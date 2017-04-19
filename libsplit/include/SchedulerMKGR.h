#ifndef SCHEDULERMKGR_H
#define SCHEDULERMKGR_H

#include "SchedulerAutoAbs.h"

extern "C" {
#include <kernel_granularity.h>
}

class SchedulerMKGR : public SchedulerAutoAbs {
public:
  SchedulerMKGR(unsigned nbDevices, DeviceInfo *devInfo,
		unsigned numArgs,
		unsigned nbGlobals, KernelAnalysis *analysis);
  virtual ~SchedulerMKGR();

  virtual void endIter();

protected:
  virtual void initScheduler(const size_t *global_work_size,
			     const size_t *local_work_size, unsigned dim);

  virtual void getGranu();

private:
  double objective_time;
  double *kernel_perfs;
};

#endif /* SCHEDULERMKGR_H */
