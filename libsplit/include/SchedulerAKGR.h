#ifndef SCHEDULERAKGR_H
#define SCHEDULERAKGR_H

#include "SchedulerAutoAbs.h"

extern "C" {
#include <kernel_granularity.h>
}

class SchedulerAKGR : public SchedulerAutoAbs {
public:
  SchedulerAKGR(unsigned nbDevices, DeviceInfo *devInfo,
		unsigned numArgs,
		unsigned nbGlobals, KernelAnalysis *analysis);
  virtual ~SchedulerAKGR();

protected:
  virtual void initScheduler(const size_t *global_work_size,
			     const size_t *local_work_size, unsigned dim);

  virtual void getGranu();

private:
  kernel_granularity_t pl;
  double objective_time;
};

#endif /* SCHEDULERAKGR_H */
