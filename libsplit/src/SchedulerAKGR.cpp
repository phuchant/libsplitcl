#include "SchedulerAKGR.h"

#include <iostream>

SchedulerAKGR::SchedulerAKGR(unsigned nbDevices, DeviceInfo *devInfo,
			     unsigned numArgs, unsigned nbGlobals,
			     KernelAnalysis *analysis)
  : SchedulerAutoAbs(nbDevices, devInfo, numArgs, nbGlobals, analysis),
    pl(NULL) {}

SchedulerAKGR::~SchedulerAKGR() {
  if (pl)
    free(pl);
}

void
SchedulerAKGR::initScheduler(const size_t *global_work_size,
			     const size_t *local_work_size, unsigned dim) {
  (void) local_work_size;
  cl_uint totalWorkSize = global_work_size[dim];

  if (pl)
    free(pl);

  pl = akgr_init(nbDevices, totalWorkSize);
}

void
SchedulerAKGR::getGranu() {
  double *akgr_granu_dscr = NULL;

  akgr_set_kernel_performances_get_granularities(pl,
						 kernel_perf_descr,
						 size_perf_descr,
						 &akgr_granu_dscr,
						 &size_gr,
						 &objective_time);

  std::copy(akgr_granu_dscr, akgr_granu_dscr + size_gr, granu_dscr);

  free(akgr_granu_dscr);
}
