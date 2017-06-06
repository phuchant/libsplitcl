#include <Scheduler/SchedulerMKGR.h>

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

    D2HFunctionSampling = new std::map<double, double> *[cycleLength];
    H2DFunctionSampling = new std::map<double, double> *[cycleLength];
    for (unsigned i=0; i<cycleLength; i++) {
      D2HFunctionSampling[i] = new std::map<double, double>[nbDevices];
      H2DFunctionSampling[i] = new std::map<double, double>[nbDevices];
    }

    D2HCoef = new double[cycleLength * nbDevices]();
    H2DCoef = new double[cycleLength * nbDevices]();
  }

  SchedulerMKGR::~SchedulerMKGR() {
    delete solver;

    delete[] req_cycle_granu_dscr;
    delete[] real_cycle_granu_dscr;
    delete[] cycle_kernel_perf;

    for (unsigned i=0; i<cycleLength; i++) {
      delete[] D2HFunctionSampling[i];
      delete[] H2DFunctionSampling[i];
    }
    delete[] D2HFunctionSampling;
    delete[] H2DFunctionSampling;

    delete[] D2HCoef;
    delete[] H2DCoef;
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

    if (getCycleIter() % 2 != 0)
      return;

    *needToInstantiateAnalysis = true;

    if (kerId == 0) {
      // Get real cycle granu descriptor
      getRealCycleGranuDscr();
      // Get cycle kernel perf
      getCycleKernelPerf();

      // Update communication constraint coefficients and set communication
      // constraint to the solver.
      updateCommConstraints();

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
  SchedulerMKGR::updateCommConstraints() {
    // For each kernel in the cycle
    for (unsigned k=0; k<cycleLength; k++) {
      unsigned kprev = (k + cycleLength - 1) % cycleLength;

      double *prev_gr = &real_cycle_granu_dscr[kprev * nbDevices];
      double *cur_gr = &real_cycle_granu_dscr[k * nbDevices];

      double prev_prefix_gr[nbDevices]; double cur_prefix_gr[nbDevices];
      prefix_sum(prev_gr, prev_prefix_gr, nbDevices);
      prefix_sum(cur_gr, cur_prefix_gr, nbDevices);

      // D2H Constraints
      for (unsigned d=0; d<nbDevices; d++) {
	int *constraint = NULL;
	double solvConstraint[2*nbDevices];

	if (solver->getD2HConstraint(d, prev_gr, cur_gr, prev_prefix_gr,
				     cur_prefix_gr, &constraint)) {
	  // Add point to D2H Function + linear regression
	  unsigned prevSamplingSize = D2HFunctionSampling[k][d].size();

	  // Sample a new point.
	  double x = 0.0;
	  for (unsigned i=0; i<nbDevices; i++)
	    x += constraint[i] * prev_gr[i];
	  for (unsigned i=0; i<nbDevices; i++)
	    x += constraint[nbDevices + i] * cur_gr[i];

	  double y = kerID2SchedInfoMap[k]->D2HTimes[d];
	  D2HFunctionSampling[k][d].insert(std::pair<double, double>(x, y));

	  unsigned newSamplingSize = D2HFunctionSampling[k][d].size();

	  // Recompute comm constraint with a linear regression if there is at
	  // least two points sampled and a new one was added.
	  if (newSamplingSize >= 2 && newSamplingSize > prevSamplingSize) {
	    double X[newSamplingSize], Y[newSamplingSize];
	    unsigned vecId = 0;
	    for (auto I = D2HFunctionSampling[k][d].begin(),
		   E = D2HFunctionSampling[k][d].end(); I != E; ++I) {
	      X[vecId] = I->first; Y[vecId] = I->second;
	      vecId++;
	    }
	    double c0, c1, cov00, cov01, cov11, sumsq;
	    gsl_fit_linear(X, 1, Y, 1, newSamplingSize, &c0, &c1, &cov00, &cov01,
			   &cov11, &sumsq);
	    D2HCoef[k*nbDevices+d] = c1;
	  }
	}

	// Set D2H Constraint to solver
	double coef = D2HCoef[k*nbDevices+d];
	for (unsigned i=0; i<2*nbDevices; i++)
	  solvConstraint[i] = constraint[i] * coef;
	solver->setD2HConstraint(k, d, solvConstraint);
	free(constraint);
      }

      // H2D Constraints
      for (unsigned d=0; d<nbDevices; d++) {
	int *constraint = NULL;
	double solvConstraint[2*nbDevices];

	if (solver->getH2DConstraint(d, prev_gr, cur_gr, prev_prefix_gr,
				     cur_prefix_gr, &constraint)) {
	  // Add point to H2D Function + linear regression
	  unsigned prevSamplingSize = H2DFunctionSampling[k][d].size();

	  // Sample a new point.
	  double x = 0.0;
	  for (unsigned i=0; i<nbDevices; i++)
	    x += constraint[i] * prev_gr[i];
	  for (unsigned i=0; i<nbDevices; i++)
	    x += constraint[nbDevices + i] * cur_gr[i];

	  double y = kerID2SchedInfoMap[k]->H2DTimes[d];
	  H2DFunctionSampling[k][d].insert(std::pair<double, double>(x, y));

	  unsigned newSamplingSize = H2DFunctionSampling[k][d].size();

	  // Recompute comm constraint with a linear regression if there is at
	  // least two points sampled and a new one was added.
	  if (newSamplingSize >= 2 && newSamplingSize > prevSamplingSize) {
	    double X[newSamplingSize], Y[newSamplingSize];
	    unsigned vecId = 0;
	    for (auto I = H2DFunctionSampling[k][d].begin(),
		   E = H2DFunctionSampling[k][d].end(); I != E; ++I) {
	      X[vecId] = I->first; Y[vecId] = I->second;
	      vecId++;
	    }
	    double c0, c1, cov00, cov01, cov11, sumsq;
	    gsl_fit_linear(X, 1, Y, 1, newSamplingSize, &c0, &c1, &cov00, &cov01,
			   &cov11, &sumsq);
	    H2DCoef[k*nbDevices+d] = c1;
	  }
	}

	// Set H2D Constraint to solver
	double coef = H2DCoef[k*nbDevices+d];
	for (unsigned i=0; i<2*nbDevices; i++)
	  solvConstraint[i] = constraint[i] * coef;
	solver->setH2DConstraint(k, d, solvConstraint);
	free(constraint);
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
