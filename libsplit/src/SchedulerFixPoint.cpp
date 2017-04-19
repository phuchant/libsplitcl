#include "SchedulerFixPoint.h"

#include <iostream>


SchedulerFixPoint::SchedulerFixPoint(unsigned nbDevices, DeviceInfo *devInfo,
				     unsigned numArgs,
				     unsigned nbGlobals,
				     KernelAnalysis *analysis)
  : SchedulerAutoAbs(nbDevices, devInfo, numArgs, nbGlobals, analysis) {
  x = new double[nbDevices];
  Fx = new double[nbDevices];
}

SchedulerFixPoint::~SchedulerFixPoint() {
  delete[] x;
  delete[] Fx;
}

void
SchedulerFixPoint::initScheduler(const size_t *global_work_size,
				 const size_t *local_work_size, unsigned dim) {
  (void) global_work_size;
  (void) local_work_size;
  (void) dim;

  for (unsigned i=0; i<nbDevices; i++)
    x[i] = granu_dscr[i*3+2];
}

void
SchedulerFixPoint::getGranu() {
  unsigned nbSplits = size_gr / 3;

  // Set real x;
  for (unsigned i=0; i<nbSplits; i++)
    x[i] = kernel_perf_descr[i*3+1];

  // Compute Fx
  double fi[nbSplits];
  for (unsigned i=0; i<nbSplits; i++)
    fi[i] = kernel_perf_descr[i*3+2] / x[i];

  double alpha = 0;
  for (unsigned i=0; i<nbSplits; i++)
    alpha += 1.0 / fi[i];
  alpha = 1.0 / alpha;

  for (unsigned i=0; i<nbSplits; i++)
    Fx[i] = alpha / fi[i];

  // Update x
  for (unsigned i=0; i<nbSplits; i++)
    x[i] = Fx[i];

  // Update granu_dscr
  for (unsigned i=0; i<nbSplits; i++) {
    granu_dscr[i*3+1] = 1; // nb kernels
    granu_dscr[i*3+2] = x[i];
  }
}
