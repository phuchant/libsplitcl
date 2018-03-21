#include <Scheduler/SchedulerMKGR.h>
#include <Utils/Debug.h>

#include <cstring>

extern "C" {
#include <gsl/gsl_fit.h>
}

namespace libsplit {

  SchedulerMKGR::SchedulerMKGR(BufferManager *buffManager, unsigned nbDevices)
    : MultiKernelScheduler(buffManager, nbDevices) {

    solver = new MultiKernelSolver(nbDevices, cycleLength);

    req_cycle_granu_dscr = new double[cycleLength * nbDevices];
    for (unsigned i=0; i<cycleLength*nbDevices; i++)
      req_cycle_granu_dscr[i] = 1.0 / nbDevices;

    real_cycle_granu_dscr = new double[cycleLength * nbDevices];
    cycle_kernel_perf = new double[cycleLength * nbDevices];

    kernelsD2HCoefs = new double[cycleLength * cycleLength * nbDevices]();
    kernelsH2DCoefs = new double[cycleLength * cycleLength * nbDevices]();
    kernelsD2HSampling =
      new std::map<double, double>[cycleLength * cycleLength * nbDevices];
    kernelsH2DSampling =
      new std::map<double, double>[cycleLength * cycleLength * nbDevices];
  }

  SchedulerMKGR::~SchedulerMKGR() {
    delete solver;

    delete[] req_cycle_granu_dscr;
    delete[] real_cycle_granu_dscr;
    delete[] cycle_kernel_perf;

    delete[] kernelsD2HCoefs;
    delete[] kernelsH2DCoefs;
    delete[] kernelsD2HSampling;
    delete[] kernelsH2DSampling;
  }

  void
  SchedulerMKGR::getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete) {
    *needOtherExecToComplete = false;

    for (unsigned i=0; i<nbDevices; i++) {
      SI->req_granu_dscr[i*3] = i;
      SI->req_granu_dscr[i*3+1] = 1;
      SI->req_granu_dscr[i*3+2] = req_cycle_granu_dscr[kerId*nbDevices+i];
    }

    kerID2SchedInfoMap[kerId] = SI;
  }

  void
  SchedulerMKGR::getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis) {
    (void) SI;
    *needOtherExecToComplete = false;
    *needToInstantiateAnalysis = false;

    if (getCycleIter() % 2 != 0) {
      if (kerId == 0) {
	// Clear timers
	for (unsigned k=0; k<cycleLength; k++) {
	  SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
	  KSI->clearEvents();
	  for (unsigned d=0; d<nbDevices; d++) {
	    KSI->src2H2DTimes[d].clear();
	    KSI->src2D2HTimes[d].clear();
	  }
	}
      }

      return;
    }

    *needToInstantiateAnalysis = true;

    if (kerId == 0) {
      for (unsigned k=0; k<cycleLength; k++) {
	SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
	KSI->updateTimers();
	KSI->updatePerfDescr();

	DEBUG("timers",
	      KSI->printTimers(););
      }


      // Get real cycle granu descriptor
      getRealCycleGranuDscr();
      // Get cycle kernel perf
      getCycleKernelPerf();

      // Update communication constraint coefficients and set communication
      // constraint to the solver.
      updateKernelsCommConstraints();

      // Clear timers
      for (unsigned k=0; k<cycleLength; k++) {
      	SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
      	for (unsigned d=0; d<nbDevices; d++) {
      	  KSI->src2H2DTimes[d].clear();
      	  KSI->src2D2HTimes[d].clear();
      	}
      }


      // Set kernel performance and granularities to the solver.
      setCycleKernelPerformance();
      setCycleKernelGranuDscr();

      // Get next granularities.
      delete[] req_cycle_granu_dscr;
      req_cycle_granu_dscr = solver->getGranularities();
      if (req_cycle_granu_dscr == NULL) {
	*needToInstantiateAnalysis = false;
	return;
      }

      // Update SubKernelInfo granularities.
      for (unsigned k=0; k<cycleLength; k++) {
	SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
	KSI->req_size_gr = 3 * nbDevices;

	for (unsigned d=0; d<nbDevices; d++) {
	  KSI->req_granu_dscr[d*3+0] = d;
	  KSI->req_granu_dscr[d*3+1] = 1.0;
	  KSI->req_granu_dscr[d*3+2] =
	    req_cycle_granu_dscr[k*nbDevices+d];
	}

      }
    }
  }

  unsigned
  SchedulerMKGR::getCycleIter() const {
      return count / cycleLength;
  }

  void
  SchedulerMKGR::getRealCycleGranuDscr() {
    memset(real_cycle_granu_dscr, 0.0, cycleLength*nbDevices*sizeof(double));

    for (unsigned k=0; k<cycleLength; k++) {
      for (int i=0; i<kerID2SchedInfoMap[k]->real_size_gr/3; i++) {
	unsigned dev = kerID2SchedInfoMap[k]->real_granu_dscr[i*3];
	double gr = kerID2SchedInfoMap[k]->real_granu_dscr[i*3+2];
	real_cycle_granu_dscr[k*nbDevices+dev] = gr;
      }
    }
  }

  void SchedulerMKGR::getCycleKernelPerf() {
    memset(cycle_kernel_perf, 0.0, cycleLength*nbDevices*sizeof(double));

    for (unsigned k=0; k<cycleLength; k++) {
      for (int i=0; i<kerID2SchedInfoMap[k]->size_perf_dscr/3; i++) {
	unsigned dev = kerID2SchedInfoMap[k]->kernel_perf_dscr[i*3];
	double perf = kerID2SchedInfoMap[k]->kernel_perf_dscr[i*3+2];
	cycle_kernel_perf[k*nbDevices+dev] = perf;
      }
    }
  }

  static void prefix_sum(double *array, double *prefix, int size)
  {
    prefix[0] = 0.0;
    for (int i=1; i<size; i++) {
      prefix[i] = prefix[i-1] + array[i-1];
    }
  }

  void
  SchedulerMKGR::updateKernelsCommConstraints() {
    // Update Sampling

    // For each kernel in the cycle
    for (unsigned k=0; k<cycleLength; k++) {
      SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];

      // For each D2H comm measured
      for (unsigned d=0; d<nbDevices; d++) {
	for (auto IT : KSI->src2D2HTimes[d]) {
	  int srcId = IT.first;
	  if (srcId == -1 || srcId == (int) k)
	    continue;

	  double *prev_gr = &real_cycle_granu_dscr[srcId * nbDevices];
	  double *cur_gr = &real_cycle_granu_dscr[k * nbDevices];

	  double prev_prefix_gr[nbDevices]; double cur_prefix_gr[nbDevices];
	  prefix_sum(prev_gr, prev_prefix_gr, nbDevices);
	  prefix_sum(cur_gr, cur_prefix_gr, nbDevices);

	  int *constraint = NULL;

	  if (solver->getD2HConstraint(d, prev_gr, cur_gr, prev_prefix_gr,
				       cur_prefix_gr, &constraint)) {
	    int samplingID = k * (cycleLength * nbDevices) +
	      srcId * nbDevices + d;

	    // Add point to D2H Function + linear regression
	    unsigned prevSamplingSize = kernelsD2HSampling[samplingID].size();

	    // Sample a new point.
	    double x = 0.0;
	    for (unsigned i=0; i<nbDevices; i++)
	      x += constraint[i] * prev_gr[i];
	    for (unsigned i=0; i<nbDevices; i++)
	      x += constraint[nbDevices + i] * cur_gr[i];

	    double y = IT.second;

	    kernelsD2HSampling[samplingID].
	      insert(std::pair<double, double>(x, y));

	    unsigned newSamplingSize = kernelsD2HSampling[samplingID].size();

	    // Recompute comm constraint with a linear regression if there is at
	    // least two points sampled and a new one was added.
	    if (newSamplingSize >= 2 && newSamplingSize > prevSamplingSize) {
	      double X[newSamplingSize], Y[newSamplingSize];
	      unsigned vecId = 0;
	      for (auto I = kernelsD2HSampling[samplingID].begin(),
		     E = kernelsD2HSampling[samplingID].end(); I != E; ++I) {
		X[vecId] = I->first; Y[vecId] = I->second;
		vecId++;
	      }
	      double c0, c1, cov00, cov01, cov11, sumsq;
	      gsl_fit_linear(X, 1, Y, 1, newSamplingSize, &c0, &c1, &cov00, &cov01,
			     &cov11, &sumsq);
	      kernelsD2HCoefs[samplingID] = c1;

	      D2HDependencies[k][srcId].insert(d);
	    }
	  }

	  free(constraint);
	}
      }

      // For each H2D comm measured
      for (unsigned d=0; d<nbDevices; d++) {
	for (auto IT : KSI->src2H2DTimes[d]) {
	  int srcId = IT.first;
	  if (srcId == -1 || srcId == (int) k)
	    continue;

	  double *prev_gr = &real_cycle_granu_dscr[srcId * nbDevices];
	  double *cur_gr = &real_cycle_granu_dscr[k * nbDevices];

	  double prev_prefix_gr[nbDevices]; double cur_prefix_gr[nbDevices];
	  prefix_sum(prev_gr, prev_prefix_gr, nbDevices);
	  prefix_sum(cur_gr, cur_prefix_gr, nbDevices);

	  int *constraint = NULL;

	  if (solver->getH2DConstraint(d, prev_gr, cur_gr, prev_prefix_gr,
				       cur_prefix_gr, &constraint)) {
	    int samplingID = k * (cycleLength * nbDevices) +
	      srcId * nbDevices + d;

	    // Add point to H2D Function + linear regression
	    unsigned prevSamplingSize = kernelsH2DSampling[samplingID].size();

	    // Sample a new point.
	    double x = 0.0;
	    for (unsigned i=0; i<nbDevices; i++)
	      x += constraint[i] * prev_gr[i];
	    for (unsigned i=0; i<nbDevices; i++)
	      x += constraint[nbDevices + i] * cur_gr[i];

	    double y = IT.second;

	    kernelsH2DSampling[samplingID].
	      insert(std::pair<double, double>(x, y));

	    unsigned newSamplingSize = kernelsH2DSampling[samplingID].size();

	    // Recompute comm constraint with a linear regression if there is at
	    // least two points sampled and a new one was added.
	    if (newSamplingSize >= 2 && newSamplingSize > prevSamplingSize) {
	      double X[newSamplingSize], Y[newSamplingSize];
	      unsigned vecId = 0;
	      for (auto I = kernelsH2DSampling[samplingID].begin(),
		     E = kernelsH2DSampling[samplingID].end(); I != E; ++I) {
		X[vecId] = I->first; Y[vecId] = I->second;
		vecId++;
	      }
	      double c0, c1, cov00, cov01, cov11, sumsq;
	      gsl_fit_linear(X, 1, Y, 1, newSamplingSize, &c0, &c1, &cov00, &cov01,
			     &cov11, &sumsq);
	      kernelsH2DCoefs[samplingID] = c1;

	      H2DDependencies[k][srcId].insert(d);
	    }

	    free(constraint);
	  }
	}

      }
    }

    for (auto IT : D2HDependencies) {
      for (auto IT2 : IT.second) {
	for (unsigned d : IT2.second) {
	  int samplingId = IT.first * (cycleLength * nbDevices) +
	    IT2.first * nbDevices + d;
	  int dst = IT.first;
	  int src = IT2.first;

	  double *prev_gr = &real_cycle_granu_dscr[src * nbDevices];
	  double *cur_gr = &real_cycle_granu_dscr[dst * nbDevices];

	  double prev_prefix_gr[nbDevices]; double cur_prefix_gr[nbDevices];
	  prefix_sum(prev_gr, prev_prefix_gr, nbDevices);
	  prefix_sum(cur_gr, cur_prefix_gr, nbDevices);

	  int *constraint = NULL;
	  double solvConstraint[2*nbDevices];

	  solver->getD2HConstraint(d, prev_gr, cur_gr, prev_prefix_gr,
				   cur_prefix_gr, &constraint);

	  double coef = kernelsD2HCoefs[samplingId];
	  for (unsigned i=0; i<2*nbDevices; i++)
	    solvConstraint[i] = constraint[i] * coef;
	  solver->setKernelsD2HConstraint(dst, src, d, solvConstraint);
	  free(constraint);
	}
      }
    }
    for (auto IT : H2DDependencies) {
      for (auto IT2 : IT.second) {
	for (unsigned d : IT2.second) {
	  int samplingId = IT.first * (cycleLength * nbDevices) +
	    IT2.first * nbDevices + d;

	  int dst = IT.first;
	  int src = IT2.first;

	  double *prev_gr = &real_cycle_granu_dscr[src * nbDevices];
	  double *cur_gr = &real_cycle_granu_dscr[dst * nbDevices];

	  double prev_prefix_gr[nbDevices]; double cur_prefix_gr[nbDevices];
	  prefix_sum(prev_gr, prev_prefix_gr, nbDevices);
	  prefix_sum(cur_gr, cur_prefix_gr, nbDevices);

	  int *constraint = NULL;
	  double solvConstraint[2*nbDevices];

	  solver->getD2HConstraint(d, prev_gr, cur_gr, prev_prefix_gr,
				   cur_prefix_gr, &constraint);

	  double coef = kernelsD2HCoefs[samplingId];
	  for (unsigned i=0; i<2*nbDevices; i++)
	    solvConstraint[i] = constraint[i] * coef;
	  solver->setKernelsD2HConstraint(dst, src, d, solvConstraint);
	  free(constraint);
	}
      }
    }

  }

  void
  SchedulerMKGR::setCycleKernelPerformance() {
    for (unsigned k=0; k<cycleLength; k++)
      solver->setKernelPerf(k, &cycle_kernel_perf[k*nbDevices]);
  }

  void
  SchedulerMKGR::setCycleKernelGranuDscr() {
    for (unsigned k=0; k<cycleLength; k++)
      solver->setKernelGr(k, &real_cycle_granu_dscr[k*nbDevices]);
  }
};
