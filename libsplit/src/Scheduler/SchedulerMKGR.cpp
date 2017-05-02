#include <Scheduler/SchedulerMKGR.h>

#include <gsl/gsl_fit.h>

namespace libsplit {

  SchedulerMKGR::SchedulerMKGR(BufferManager *buffManager, unsigned nbDevices)
    : MultiKernelScheduler(buffManager, nbDevices) {
    if (nbDevices != 2) {
      std::cerr << "Error: MKGR works only with 2 devices.\n";
      exit(EXIT_FAILURE);
    }

    pl = mkgr_init(nbDevices);

    cycle_granu_dscr = NULL;
    cycle_granu_size = 0;

    commFunctionSampling = new std::map<double, double> *[cycleLength];
    for (unsigned i=0; i<cycleLength; i++)
      commFunctionSampling[i] = new std::map<double, double>[nbDevices];

    commConstraint = new double *[cycleLength];
    for (unsigned i=0; i<cycleLength; i++) {
      commConstraint[i] = new double[nbDevices];
      for (unsigned d=0; d<nbDevices; d++) {
	commConstraint[i][d] = 0;
      }
    }

    comm_perf_from_to = new double *[cycleLength];
    for (unsigned i=0; i<cycleLength; i++) {
      comm_perf_from_to[i] = new double[nbDevices * 6];

      // Works only with 2 devices.
      for (unsigned d=0; d<nbDevices; d++) {
	comm_perf_from_to[i][6*d + 0] = 1 - d; // from
	comm_perf_from_to[i][6*d + 1] = d; // to
	comm_perf_from_to[i][6*d + 2] = 0; // positive constraint
	comm_perf_from_to[i][6*d + 3] = 1 - d; // from
	comm_perf_from_to[i][6*d + 4] = d; // to
	comm_perf_from_to[i][6*d + 5] = 0; // negative constraint
      }
    }
  }

  SchedulerMKGR::~SchedulerMKGR() {
    free(pl);
    // TODO
  }

  void
  SchedulerMKGR::getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete) {
    *needOtherExecToComplete = false;
    std::cerr << "MKGR count=" << count << " kerId=" << kerId
	      << " iteration=" << getCycleIter() << "\n";

    for (unsigned i=0; i<nbDevices; i++) {
      SI->req_granu_dscr[i*3] = i;
      SI->req_granu_dscr[i*3+1] = 1;
      SI->req_granu_dscr[i*3+2] = 1.0 / nbDevices;
    }

    kerID2SchedInfoMap[kerId] = SI;
  }

  static void print_kernel_granularity_descr(double* granularity_descr,
					     int nb_devices) {
    printf("id_kernel : =%d \n", (int) granularity_descr[0]);
    double som = 0;
    for (int i=1 ; i < 2* nb_devices; i=i+2) {
      som +=  granularity_descr[i+1];
      printf("\t<dev: %d, gr: %g> ", (int)granularity_descr[i],
	     granularity_descr[i+1]);
    }
    //  assert(som == 1.0);
    printf("\nsom = %g\n", som);
  }

  static void print_granularity_descr(double* granularity_descr, int nb_kernels,
				      int nb_devices) {
    double *p = granularity_descr;
    for (int i_ker=0 ; i_ker < nb_kernels; i_ker++) {
      print_kernel_granularity_descr(p, nb_devices);
      p += 1 + nb_devices*2;
    }
  }

  void
  SchedulerMKGR::getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis) {
    *needOtherExecToComplete = false;
    *needToInstantiateAnalysis = false;

    std::cerr << "MKGR count=" << count << " kerId=" << kerId
	      << " iteration=" << getCycleIter() << "\n";

    if (getCycleIter() % 2 != 0)
      return;

    *needToInstantiateAnalysis = true;

    if (kerId == 0) {
      std::cerr << "measuring all for whole previous cycle and computing next "
		<< "cycle partitioning !\n";

      // Update communication constraint coefficients.
      updateCommConstraints();

      // Set communication and kernel performance to MKGR
      setCycleKernelPerformance();
      setCycleCommPerformance();

      // Get next granularities.
      mkgr_get_granularities(pl, &cycle_granu_dscr, &cycle_granu_size,
      			     &objective_time);
      print_granularity_descr(cycle_granu_dscr, cycleLength, nbDevices);

      // Update SubKernelInfo granularities.
      assert(cycle_granu_size == (int) cycleLength * 5);
      for (int i=0; i<cycle_granu_size / 5; i++) {
	unsigned k = cycle_granu_dscr[i*5];
	SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
	unsigned dev1_id = cycle_granu_dscr[i*5+1];
	double gr_dev1 = cycle_granu_dscr[i*5+2];
	unsigned dev2_id = cycle_granu_dscr[i*5+3];
	double gr_dev2 = cycle_granu_dscr[i*5+4];
	KSI->req_size_gr = 6;
	KSI->req_granu_dscr[0] = dev1_id;
	KSI->req_granu_dscr[1] = 1;
	KSI->req_granu_dscr[2] = gr_dev1;
	KSI->req_granu_dscr[3] = dev2_id;
	KSI->req_granu_dscr[4] = 1;
	KSI->req_granu_dscr[5] = gr_dev2;
      }

      free(cycle_granu_dscr);
    }

    std::cerr << "returning new granu\n";
  }

  unsigned
  SchedulerMKGR::getCycleIter() const {
      return count / cycleLength;
  }

  void
  SchedulerMKGR::updateCommConstraints() {
    for (unsigned k=0; k<cycleLength; k++) {
      unsigned kprev = (k + cycleLength - 1) % cycleLength;

      for (unsigned d=0; d<nbDevices; d++) {
	double granuKer, granuKerPrev;
	granuKer = granuKerPrev = 0;
	for (int i=0; i<kerID2SchedInfoMap[k]->size_perf_dscr/3; i++) {
	  if (i*3 == (int) d) {
	    granuKer = kerID2SchedInfoMap[k]->kernel_perf_dscr[i*3+1];
	    break;
	  }
	}
	for (int i=0; i<kerID2SchedInfoMap[kprev]->size_perf_dscr/3; i++) {
	  if (i*3 == (int) d) {
	    granuKerPrev = kerID2SchedInfoMap[kprev]->kernel_perf_dscr[i*3+1];
	    break;
	  }
	}
	// double granuKer = kerID2SchedInfoMap[k]->kernel_perf_dscr[d*3+1];
	// double granuKerPrev =
	//   kerID2SchedInfoMap[kprev]->kernel_perf_dscr[d*3+1];

	unsigned prevSamplingSize = commFunctionSampling[k][d].size();

	// Sample a new point.
	if (granuKer > granuKerPrev) {
	  double granuDiff = granuKer - granuKerPrev;
	  double timeMeasured = kerID2SchedInfoMap[k]->H2DTimes[d];
	  commFunctionSampling[k][d].
	    insert(std::pair<double, double>(granuDiff, timeMeasured));

	  std::cerr << "granuKer : " << granuKer << " granuPrev : " << granuKerPrev << "\n";
	  std::cerr << "granuDiff : " << granuDiff << "\n";
	  std::cerr << "timeMeasured : " << timeMeasured << "\n";
	}

	unsigned newSamplingSize = commFunctionSampling[k][d].size();

	// Recompute comm constraint with a linear regression if there is at
	// least two points sampled and a new one was added.
	if (newSamplingSize >= 2 && newSamplingSize > prevSamplingSize) {
	  double X[newSamplingSize], Y[newSamplingSize];
	  unsigned vecId = 0;
	  for (auto I = commFunctionSampling[k][d].begin(),
		 E = commFunctionSampling[k][d].end(); I != E; ++I) {
	    X[vecId] = I->first; Y[vecId] = I->second;
	    vecId++;
	  }
	  double c0, c1, cov00, cov01, cov11, sumsq;
	  gsl_fit_linear(X, 1, Y, 1, newSamplingSize, &c0, &c1, &cov00, &cov01,
			 &cov11, &sumsq);
	  commConstraint[k][d] = c1;
	}
      }
    }
  }

  void
  SchedulerMKGR::setCycleCommPerformance() {
    for (unsigned k=0; k<cycleLength; k++) {
      for (unsigned d=0; d<nbDevices; d++) {
	// Works only with 2 devices.
	comm_perf_from_to[k][6*d+2] = commConstraint[k][d]; // pos constr
	comm_perf_from_to[k][6*d+5] = -1.0 * commConstraint[k][1-d]; // neg constr
      }

      int kprev = (k + cycleLength - 1) % cycleLength;
      mkgr_set_comm_performance(pl,
				kprev,
				k,
				comm_perf_from_to[k], nbDevices * 6);

      std::cerr << "set commm perf <";
      for (unsigned i=0; i<nbDevices*6; i++)
	std::cerr << comm_perf_from_to[k][i] << ",";
      std::cerr << "\n";
    }
  }

  void
  SchedulerMKGR::setCycleKernelPerformance() {
    for (unsigned k=0; k<cycleLength; k++) {
      // Get SubKernelSchedInfo
      SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];

      // Set kernel performance.
      mkgr_set_kernel_performance(pl, k,
				  KSI->kernel_perf_dscr,
				  KSI->size_perf_dscr);

    }
  }

};
