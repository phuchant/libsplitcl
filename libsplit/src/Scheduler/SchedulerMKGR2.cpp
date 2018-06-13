#include <EventFactory.h>
#include <Queue/DeviceQueue.h>
#include <Globals.h>
#include <Scheduler/SchedulerMKGR2.h>
#include <Utils/Debug.h>

#include <iostream>
#include <fstream>

#include <cstring>

extern "C" {
#include <gsl/gsl_fit.h>
}

#define EPSILON 1e-6

namespace libsplit {

  SchedulerMKGR2::SchedulerMKGR2(BufferManager *buffManager, unsigned nbDevices)
    : MultiKernelScheduler(buffManager, nbDevices) {

    solver = new MultiKernelSolver(nbDevices, cycleLength);

    req_cycle_granu_dscr = new double[cycleLength * nbDevices];
    for (unsigned i=0; i<cycleLength*nbDevices; i++)
      req_cycle_granu_dscr[i] = 1.0 / nbDevices;

    real_cycle_granu_dscr = new double[cycleLength * nbDevices];
    cycle_kernel_perf = new double[cycleLength * nbDevices];

    kernelsD2HCoefs = new double[cycleLength * cycleLength * nbDevices]();
    kernelsH2DCoefs = new double[cycleLength * cycleLength * nbDevices]();

    H2DThroughputCoefs = new double[nbDevices]();
    D2HThroughputCoefs = new double[nbDevices]();
    isDeviceH2DThroughputSampled = new bool[nbDevices];
    isDeviceD2HThroughputSampled = new bool[nbDevices];
    for (unsigned d=0; d<nbDevices; d++) {
      isDeviceH2DThroughputSampled[d] = false;
      isDeviceD2HThroughputSampled[d] = false;
    }

    isD2HCoefUpdated = new int[cycleLength * cycleLength];
    isH2DCoefUpdated = new int[cycleLength * cycleLength];
  }

  SchedulerMKGR2::~SchedulerMKGR2() {
    delete solver;

    delete[] req_cycle_granu_dscr;
    delete[] real_cycle_granu_dscr;
    delete[] cycle_kernel_perf;

    delete[] kernelsD2HCoefs;
    delete[] kernelsH2DCoefs;
    delete[] H2DThroughputCoefs;
    delete[] D2HThroughputCoefs;
    delete[] isH2DCoefUpdated;
    delete[] isD2HCoefUpdated;
    delete[] isDeviceH2DThroughputSampled;
    delete[] isDeviceD2HThroughputSampled;
  }

  void
  SchedulerMKGR2::getInitialPartition(SubKernelSchedInfo *SI,
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
  SchedulerMKGR2::getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis) {
    (void) SI;
    *needOtherExecToComplete = false;
    *needToInstantiateAnalysis = false;

    if (getCycleIter() % 2 != 0) {
      if (kerId == 0) {
	double totalCyclePerDevice[nbDevices] = {0};

	// Clear timers
	for (unsigned k=0; k<cycleLength; k++) {
	  SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
	  DEBUG("timers",
		KSI->updateTimers();
		KSI->updatePerfDescr();
		KSI->printTimers(k);
		for (unsigned d=0; d<nbDevices; d++) {
		  totalCyclePerDevice[d] += KSI->D2HTimes[d] + KSI->H2DTimes[d] +
		    KSI->kernelTimes[d];
		}
		);


	  KSI->clearEvents();
	  for (unsigned d=0; d<nbDevices; d++) {
	    KSI->src2H2DTimes[d].clear();
	    KSI->src2D2HTimes[d].clear();
	  }

	  KSI->buffersRequired.clear();
	}
        eventFactory->freeEvents();
	DEBUG("timers",
	    for (unsigned d=0; d<nbDevices; d++) {
	      std::cerr << "total iter time on device " << d << ": "
			<< totalCyclePerDevice[d] << "\n";
	    }
	    );
      }

      return;
    }

    *needToInstantiateAnalysis = true;

    if (kerId == 0) {
      double totalCyclePerDevice[nbDevices] = {0};

      for (unsigned k=0; k<cycleLength; k++) {
	SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
	KSI->updateTimers();
	KSI->updatePerfDescr();

	DEBUG("timers",
	      for (unsigned d=0; d<nbDevices; d++) {
		totalCyclePerDevice[d] += KSI->D2HTimes[d] + KSI->H2DTimes[d] +
		  KSI->kernelTimes[d];
	      }
	      KSI->printTimers(k);
	      );
      }
      eventFactory->freeEvents();
      DEBUG("timers",
	    for (unsigned d=0; d<nbDevices; d++) {
	      std::cerr << "total iter time on device " << d << ": "
			<< totalCyclePerDevice[d] << "\n";
	    }
	    );


      // Get real cycle granu descriptor
      getRealCycleGranuDscr();

      // Get cycle kernel perf
      getCycleKernelPerf();

      // Sample devices throughput
      sampleDevicesThroughput();

      // Sample transfers
      sampleTransfers();

      // Update kernels comm constraints
      updateKernelsCommConstraints();

      // Clear timers
      for (unsigned k=0; k<cycleLength; k++) {
      	SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
      	for (unsigned d=0; d<nbDevices; d++) {
      	  KSI->src2H2DTimes[d].clear();
      	  KSI->src2D2HTimes[d].clear();
      	}

	KSI->buffersRequired.clear();
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

	double total = 0;

	for (unsigned d=0; d<nbDevices; d++) {
	  KSI->req_granu_dscr[d*3+0] = d;
	  KSI->req_granu_dscr[d*3+1] = 1.0;
	  KSI->req_granu_dscr[d*3+2] =
	    req_cycle_granu_dscr[k*nbDevices+d];
	  total += KSI->req_granu_dscr[d*3+2];
	}
	if (total < 0.9)
	  solver->dumpProb();
	assert(total > 0.9);
      }
    }
  }

  unsigned
  SchedulerMKGR2::getCycleIter() const {
      return count / cycleLength;
  }

  void
  SchedulerMKGR2::getRealCycleGranuDscr() {
    memset(real_cycle_granu_dscr, 0.0, cycleLength*nbDevices*sizeof(double));

    for (unsigned k=0; k<cycleLength; k++) {
      for (int i=0; i<kerID2SchedInfoMap[k]->real_size_gr/3; i++) {
	unsigned dev = kerID2SchedInfoMap[k]->real_granu_dscr[i*3];
	double gr = kerID2SchedInfoMap[k]->real_granu_dscr[i*3+2];
	real_cycle_granu_dscr[k*nbDevices+dev] = gr;
      }
    }
  }

  void SchedulerMKGR2::getCycleKernelPerf() {
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
  SchedulerMKGR2::updateKernelsCommConstraints() {
    // D2H Constraints
    for (auto IT : D2HDependencies) {
      for (unsigned src : IT.second) {
	unsigned dst = IT.first;
	for (unsigned d=0; d<nbDevices; d++) {
	  int samplingId = dst * (cycleLength * nbDevices) +
	    src * nbDevices + d;
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
	  solver->set_keri_from_kerj_Dk2H_coef(dst, src, d, coef);
	  free(constraint);
	}
      }
    }

    // H2D Constraints
    for (auto IT : H2DDependencies) {
      for (unsigned src : IT.second) {
	unsigned dst = IT.first;
	for (unsigned d=0; d<nbDevices; d++) {
	  int samplingId = dst * (cycleLength * nbDevices) +
	    src * nbDevices + d;
	  double *prev_gr = &real_cycle_granu_dscr[src * nbDevices];
	  double *cur_gr = &real_cycle_granu_dscr[dst * nbDevices];

	  double prev_prefix_gr[nbDevices]; double cur_prefix_gr[nbDevices];
	  prefix_sum(prev_gr, prev_prefix_gr, nbDevices);
	  prefix_sum(cur_gr, cur_prefix_gr, nbDevices);

	  int *constraint = NULL;
	  double solvConstraint[2*nbDevices];

	  solver->getH2DConstraint(d, prev_gr, cur_gr, prev_prefix_gr,
				   cur_prefix_gr, &constraint);

	  double coef = kernelsH2DCoefs[samplingId];
	  assert(isfinite(coef));
	  for (unsigned i=0; i<2*nbDevices; i++)
	    solvConstraint[i] = constraint[i] * coef;
	  solver->setKernelsH2DConstraint(dst, src, d, solvConstraint);
	  solver->set_keri_from_kerj_H2Dk_coef(dst, src, d, coef);
	  free(constraint);
	}
      }
    }
  }

  void
  SchedulerMKGR2::setCycleKernelPerformance() {
    for (unsigned k=0; k<cycleLength; k++)
      solver->setKernelPerf(k, &cycle_kernel_perf[k*nbDevices]);
  }

  void
  SchedulerMKGR2::setCycleKernelGranuDscr() {
    for (unsigned k=0; k<cycleLength; k++)
      solver->setKernelGr(k, &real_cycle_granu_dscr[k*nbDevices]);
  }

  static
  double computeSquaredResidual(double *X, double *Y, unsigned n, double a,
				double b) {
    // See: https://en.wikipedia.org/wiki/Coefficient_of_determination

    double mean = 0;
    for (unsigned i=0; i<n; i++)
      mean += Y[i];
    mean /= n;

    double SSres = 0;
    for (unsigned i=0; i<n; i++) {
      double res = Y[i] - (a * X[i] + b);
      SSres += res * res;
    }

    double SStot = 0;
    for (unsigned i=0; i<n; i++) {
      double elem = Y[i] - mean;
      SStot += elem * elem;
    }

    return 1.0 - SSres / SStot;
  }

  void
  SchedulerMKGR2::sampleDevicesThroughput() {
    // H2D Throughputs
    for (unsigned d=0; d<nbDevices; d++) {
      unsigned nbSamples = H2DThroughputSamplingPerDevice[d].size();
      if (nbSamples < 2)
	continue;
      if (nbSamples > 20 && isDeviceH2DThroughputSampled[d])
	continue;

      double H2D_X[nbSamples]; double H2D_Y[nbSamples];
      for (unsigned i=0; i<nbSamples; i++) {
	H2D_X[i] = H2DThroughputSamplingPerDevice[d][i].first;
	H2D_Y[i] = H2DThroughputSamplingPerDevice[d][i].second;
      }

      double c0; double c1; double cov00; double cov01; double cov11;
      double sumsq;

      int err = gsl_fit_linear(H2D_X, 1, H2D_Y, 1, nbSamples, &c0, &c1,
			       &cov00, &cov01, &cov11, &sumsq);
      assert(err == 0);
      if (isfinite(c1) && c1 > 0) {
	H2DThroughputCoefs[d] = c1;
	isDeviceH2DThroughputSampled[d] = true;
      }
    }

    // D2H Throughput
    for (unsigned d=0; d<nbDevices; d++) {
      unsigned nbSamples = D2HThroughputSamplingPerDevice[d].size();
      if (nbSamples < 2)
	continue;
      if (nbSamples > 20 && isDeviceD2HThroughputSampled[d])
	continue;

      double D2H_X[nbSamples]; double D2H_Y[nbSamples];
      for (unsigned i=0; i<nbSamples; i++) {
	D2H_X[i] = D2HThroughputSamplingPerDevice[d][i].first;
	D2H_Y[i] = D2HThroughputSamplingPerDevice[d][i].second;
      }

      double c0; double c1; double cov00; double cov01; double cov11;
      double sumsq;

      int err = gsl_fit_linear(D2H_X, 1, D2H_Y, 1, nbSamples, &c0, &c1,
			       &cov00, &cov01, &cov11, &sumsq);
      assert(err == 0);
      if (isfinite(c1) && c1 > 0) {
	D2HThroughputCoefs[d] = c1;
	isDeviceD2HThroughputSampled[d] = true;
      }
    }
  }

  void
  SchedulerMKGR2::sampleTransfers() {
    memset(isD2HCoefUpdated, 0, cycleLength * cycleLength * sizeof(int));
    memset(isH2DCoefUpdated, 0, cycleLength * cycleLength * sizeof(int));

    for (unsigned k=0; k<cycleLength; k++) {
      sampleKernelTransfers(k);
    }

    // Recompute kernels D2H coefs if needed
    for (unsigned to=0; to<cycleLength; to++) {
      for (unsigned from=0; from<cycleLength; from++) {
	if (isD2HCoefUpdated[to * cycleLength + from] == 0)
	  continue;

	D2HDependencies[to].insert(from);

	double newCoef = 0;
	for (auto I : D2HCoefsPerBuffer[to][from])
	  newCoef += I.second;

	for (unsigned d=0; d<nbDevices; d++) {
	  kernelsD2HCoefs[to * (cycleLength * nbDevices) +
			  from * nbDevices +
			  d] = newCoef * D2HThroughputCoefs[d];
	}
      }
    }

    // Recompute kernels H2D coefs if needed
    for (unsigned to=0; to<cycleLength; to++) {
      for (unsigned from=0; from<cycleLength; from++) {
	if (isH2DCoefUpdated[to * cycleLength + from] == 0)
	  continue;

	H2DDependencies[to].insert(from);

	double newCoef = 0;
	for (auto I : H2DCoefsPerBuffer[to][from])
	  newCoef += I.second;

	for (unsigned d=0; d<nbDevices; d++) {
	  kernelsH2DCoefs[to * (cycleLength * nbDevices) +
			  from * nbDevices +
			  d] = newCoef * H2DThroughputCoefs[d];
	}
      }
    }
  }

  void
  SchedulerMKGR2::sampleKernelTransfers(unsigned kerId) {
    SubKernelSchedInfo *KSI = kerID2SchedInfoMap[kerId];
    for (MemoryHandle *bufferRequired : KSI->buffersRequired) {
      sampleKernelTransfersForBuffer(kerId, bufferRequired);
    }
  }

  void
  SchedulerMKGR2::sampleKernelTransfersForBuffer(unsigned kerId,
						MemoryHandle *m) {
    ListInterval requiredByDevice[nbDevices];
    for (unsigned d=0; d<nbDevices; d++)
      requiredByDevice[d].myUnion(m->ker2Dev2ReadRegion[kerId][d]);
    unsigned total= 0;
    for (unsigned d=0; d<nbDevices; d++)
      total += requiredByDevice[d].total();
    if (total == 0)
      return;

    std::vector<unsigned> kernelsFrom;
    for (int i = ((int) kerId) -1; i >= 0; i--)
      kernelsFrom.push_back(i);
    for (unsigned i=cycleLength-1; i >kerId; i--)
      kernelsFrom.push_back(i);

    for (unsigned i=0; i<kernelsFrom.size(); i++) {
      unsigned from = kernelsFrom[i];

      unsigned H2D_X[nbDevices];
      double H2D_Y[nbDevices];
      unsigned D2H_X[nbDevices];
      double D2H_Y[nbDevices];

      ListInterval writtenAllDevices;
      for (unsigned d=0; d<nbDevices; d++)
	writtenAllDevices.myUnion(m->ker2Dev2WrittenRegion[from][d]);
      if (writtenAllDevices.total() == 0)
	continue;

      bool hasData = false;
      for (unsigned d=0; d<nbDevices; d++) {
	ListInterval *intersection =
	  ListInterval::intersection(requiredByDevice[d],
				     writtenAllDevices);
	if (intersection->total() > 0) {
	  hasData = true;
	  delete intersection;
	  break;
	}

	delete intersection;
      }
      if (!hasData)
	continue;

      ListInterval alreadyPresent[nbDevices];
      for (unsigned d = 0; d<nbDevices; d++) {
	ListInterval *intersection =
	  ListInterval::intersection(requiredByDevice[d],
				     m->ker2Dev2WrittenRegion[from][d]);
	alreadyPresent[d].myUnion(*intersection);
	delete intersection;
      }

      ListInterval requiredByDeviceMinusAlreadyPresent[nbDevices];
      for (unsigned d=0; d<nbDevices; d++) {
	requiredByDeviceMinusAlreadyPresent[d].myUnion(requiredByDevice[d]);
	requiredByDeviceMinusAlreadyPresent[d].difference(alreadyPresent[d]);
      }

      ListInterval requiredMinPresentAllDevices;
      for (unsigned d=0; d<nbDevices; d++)
	requiredMinPresentAllDevices.myUnion(requiredByDeviceMinusAlreadyPresent[d]);


      ListInterval granuFrom[nbDevices];
      ListInterval granuTo[nbDevices];

      // H2D:
      // X = partition_d_to \ partition_d_from
      // Y =  (required-min-already-present_d) inter written_kerfrom_all_devices
      for (unsigned d=0; d<nbDevices; d++) {
	ListInterval *inter =
	  ListInterval::intersection(requiredByDeviceMinusAlreadyPresent[d],
				     writtenAllDevices);
	H2D_Y[d] = inter->total();
	delete inter;
	ListInterval *interX =
	  ListInterval::difference(kerID2SchedInfoMap[kerId]->granu_intervals[d],
				   kerID2SchedInfoMap[from]->granu_intervals[d]);
	H2D_X[d] = interX->total();
	delete interX;
      }

      // D2H:
      // X = partition_d_from \ partition_d_to
      // Y = U_{required-min-present_others} inter written_d
      for (unsigned d=0; d<nbDevices; d++) {
	ListInterval *requiredMinPresentOther = ListInterval::difference(requiredMinPresentAllDevices,
									 requiredByDevice[d]);
	ListInterval *inter =
	  ListInterval::intersection(*requiredMinPresentOther,
				     m->ker2Dev2WrittenRegion[from][d]);
	delete requiredMinPresentOther;
	D2H_Y[d] = inter->total();
	delete inter;
	ListInterval *interX =
	  ListInterval::difference(kerID2SchedInfoMap[from]->granu_intervals[d],
				   kerID2SchedInfoMap[kerId]->granu_intervals[d]);
	D2H_X[d] = interX->total();
	delete interX;
      }

      bool D2HPointAdded = false;
      bool H2DPointAdded = false;
      for (unsigned d=0; d<nbDevices; d++) {
    	if (H2D_X[d] > 0) {
    	  H2DPointAdded = true;
	  double x_point = (double) H2D_X[d] / GRANU2INTFACTOR;
	  double y_point = H2D_Y[d];
	  for (unsigned i=0; i<dst2SrcBufferH2DSampling[kerId][from][m->id].size(); i++) {
	    if (dst2SrcBufferH2DSampling[kerId][from][m->id][i].first == x_point &&
		dst2SrcBufferH2DSampling[kerId][from][m->id][i].second == y_point) {
	      H2DPointAdded = false;
	      break;
	    }
	  }

	  if (H2DPointAdded) {
    	  assert(H2D_X[d] < 2000000);
	       dst2SrcBufferH2DSampling[kerId][from][m->id].
    	    push_back(std::make_pair((double) H2D_X[d] /
    				     GRANU2INTFACTOR, H2D_Y[d]));
	  }
    	}

    	if (D2H_X[d] > 0) {
	  D2HPointAdded = true;
	  double x_point = (double) D2H_X[d] / GRANU2INTFACTOR;
	  double y_point = D2H_Y[d];

	  for (unsigned i=0; i<dst2SrcBufferD2HSampling[kerId][from][m->id].size(); i++) {
	    if (dst2SrcBufferD2HSampling[kerId][from][m->id][i].first == x_point &&
		dst2SrcBufferD2HSampling[kerId][from][m->id][i].second == y_point) {
	      D2HPointAdded = false;
	      break;
	    }
	  }

	  if (D2HPointAdded) {
	    assert(D2H_X[d] < 2000000);
	    dst2SrcBufferD2HSampling[kerId][from][m->id].
	      push_back(std::make_pair((double) D2H_X[d] /
				       GRANU2INTFACTOR, D2H_Y[d]));
	  }
    	}
      }


      // Update D2H coef
      if (H2DPointAdded) {
      	unsigned samplingSize =
      	  dst2SrcBufferH2DSampling[kerId][from][m->id].size();
      	if (samplingSize > 2) {
      	  double X[samplingSize]; double Y[samplingSize];
      	  for (unsigned i=0; i<samplingSize; i++) {
      	    X[i] = dst2SrcBufferH2DSampling[kerId][from][m->id][i].first;
      	    Y[i] = dst2SrcBufferH2DSampling[kerId][from][m->id][i].second;
      	  }

      	  double c0; double c1; double cov00; double cov01; double cov11;
      	  double sumsq;

      	  int err = gsl_fit_linear(X, 1, Y, 1, samplingSize, &c0, &c1,
      				   &cov00, &cov01, &cov11, &sumsq);
      	  if (err == 0 && isfinite(c1)) {
      	    if (computeSquaredResidual(X, Y, samplingSize, c1, c0) >
      		RESIDUAL_TRESHOLD && c1 > 0) {
      	      H2DCoefsPerBuffer[kerId][from][m->id] = c1;
      	      assert(isfinite(c1));
      	    } else {
      	      H2DCoefsPerBuffer[kerId][from][m->id] = 0;
      	    }
      	    isH2DCoefUpdated[kerId * cycleLength + from] = 1;
      	  }

      	}
      }

      if (D2HPointAdded) {
      	unsigned samplingSize =
      	  dst2SrcBufferD2HSampling[kerId][from][m->id].size();
      	if (samplingSize > 2) {
      	  double X[samplingSize]; double Y[samplingSize];
      	  for (unsigned i=0; i<samplingSize; i++) {
      	    X[i] = dst2SrcBufferD2HSampling[kerId][from][m->id][i].first;
      	    Y[i] = dst2SrcBufferD2HSampling[kerId][from][m->id][i].second;
      	  }

      	  double c0; double c1; double cov00; double cov01; double cov11;
      	  double sumsq;

      	  int err = gsl_fit_linear(X, 1, Y, 1, samplingSize, &c0, &c1,
      				   &cov00, &cov01, &cov11, &sumsq);

      	  if (err == 0 && isfinite(c1)) {
      	    if (computeSquaredResidual(X, Y, samplingSize, c1, c0) >
      		RESIDUAL_TRESHOLD && c1 > 0) {
      	      D2HCoefsPerBuffer[kerId][from][m->id] = c1;
      	    } else {
      	      assert(isfinite(c1));
      	      D2HCoefsPerBuffer[kerId][from][m->id] = 0;
      	    }
      	    isD2HCoefUpdated[kerId * cycleLength + from] = 1;
      	  }

      	}
      }

      // Update data required
      total = 0;
      for (unsigned d=0; d<nbDevices; d++) {
	requiredByDevice[d].difference(writtenAllDevices);
	total += requiredByDevice[d].total();
      }

      return;
      if (total == 0)
	return;
    }
  }

  void
  SchedulerMKGR2::plotD2HPoints() const {
    std::string plotFilename = std::string("plot-D2H-points.gnu");
    std::string dataFilename = std::string("data-D2H-points.dat");
    std::string pdfFilename = std::string("D2H-points.pdf");
    std::ofstream plotFile; plotFile.open(plotFilename);
    std::ofstream dataFile; dataFile.open(dataFilename);
    unsigned dataFileRowOffset = 1;

    // Plot file
    plotFile << "set terminal pdf\n"
	     << "set output \"" << pdfFilename << "\"\n"
	     << "unset key\n";

    for (auto kertoIT : dst2SrcBufferD2HSampling) {
      for (auto kerfromIT : kertoIT.second) {
	for (auto bufferIT : kerfromIT.second) {
	  unsigned from = kerfromIT.first;
	  unsigned to = kertoIT.first;
	  unsigned buffer = bufferIT.first;
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << "\"\n";

	  unsigned startRow = dataFileRowOffset;

	  double samplesX[bufferIT.second.size()];
	  double samplesY[bufferIT.second.size()];
	  unsigned sampleId=0;

	  for (auto pointIT : bufferIT.second) {
	    double x = pointIT.first;
	    double y = pointIT.second;
	    samplesX[sampleId] = x;
	    samplesY[sampleId] = y;
	    sampleId++;

	    dataFile << x << "\t" << y << "\n";
	    dataFileRowOffset++;

	  }

	  double c0, c1, cov00, cov01, cov11, sumsq, error;
	  if (sampleId > 0) {
	    gsl_fit_linear(samplesX, 1, samplesY, 1, sampleId, &c0, &c1, &cov00,
			   &cov01, &cov11, &sumsq);

	    // Compute Error
	    error = computeSquaredResidual(samplesX, samplesY, sampleId, c1,
					   c0);
	  }

	  unsigned endRow = dataFileRowOffset-1;

	  if (endRow - startRow > 0) {
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << " R2 " << error << "\"\n";

	    plotFile << "f(x) = "<< c1 << " * x + " << c0 << "\n";
	    // plotFile << "fit f(x) \"<(sed -n '"<<startRow<<"," <<endRow<<"p' "
	    // 	     << dataFilename << ")\" using 1 : 2 via m,b\n";
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints, \\\n"
		     << "f(x)\n";
	  } else {
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints\n";
	  }
	}
      }
    }
  }

  void
  SchedulerMKGR2::plotH2DPoints() const {
    std::string plotFilename = std::string("plot-H2D-points.gnu");
    std::string dataFilename = std::string("data-H2D-points.dat");
    std::string pdfFilename = std::string("H2D-points.pdf");
    std::ofstream plotFile; plotFile.open(plotFilename);
    std::ofstream dataFile; dataFile.open(dataFilename);
    unsigned dataFileRowOffset = 1;

    // Plot file
    plotFile << "set terminal pdf\n"
	     << "set output \"" << pdfFilename << "\"\n"
	     << "unset key\n";

    for (auto kertoIT : dst2SrcBufferH2DSampling) {
      for (auto kerfromIT : kertoIT.second) {
	for (auto bufferIT : kerfromIT.second) {
	  unsigned from = kerfromIT.first;
	  unsigned to = kertoIT.first;
	  unsigned buffer = bufferIT.first;
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << "\"\n";

	  unsigned startRow = dataFileRowOffset;

	  double samplesX[bufferIT.second.size()];
	  double samplesY[bufferIT.second.size()];
	  unsigned sampleId=0;

	  for (auto pointIT : bufferIT.second) {
	    double x = pointIT.first;
	    double y = pointIT.second;
	    samplesX[sampleId] = x;
	    samplesY[sampleId] = y;
	    sampleId++;

	    dataFile << x << "\t" << y << "\n";
	    dataFileRowOffset++;

	  }

	  double c0, c1, cov00, cov01, cov11, sumsq, error;
	  if (sampleId > 0) {
	    gsl_fit_linear(samplesX, 1, samplesY, 1, sampleId, &c0, &c1, &cov00,
			   &cov01, &cov11, &sumsq);

	    // Compute Error
	    error = computeSquaredResidual(samplesX, samplesY, sampleId, c1,
					   c0);
	  }

	  unsigned endRow = dataFileRowOffset-1;

	  if (endRow - startRow > 0) {
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << " R2 " << error << "\"\n";

	    plotFile << "f(x) = "<< c1 << " * x + " << c0 << "\n";
	    // plotFile << "fit f(x) \"<(sed -n '"<<startRow<<"," <<endRow<<"p' "
	    // 	     << dataFilename << ")\" using 1 : 2 via m,b\n";
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints, \\\n"
		     << "f(x)\n";
	  } else {
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints\n";
	  }
	}
      }
    }
  }
};
