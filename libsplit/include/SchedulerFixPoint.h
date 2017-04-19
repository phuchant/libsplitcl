#ifndef SCHEDULERFIXPOINT_H
#define SCHEDULERFIXPOINT_H

#include "SchedulerAutoAbs.h"

class SchedulerFixPoint : public SchedulerAutoAbs {
public:
  SchedulerFixPoint(unsigned nbDevices, DeviceInfo *devInfo,
		    unsigned numArgs,
		    unsigned nbGlobals, KernelAnalysis *analysis);
  virtual ~SchedulerFixPoint();

protected:
  virtual void initScheduler(const size_t *global_work_size,
			     const size_t *local_work_size, unsigned dim);

  virtual void getGranu();

private:
  double *x; // granus
  double *Fx;
};

#endif /* SCHEDULERFIXPOINT_H */
