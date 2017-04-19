#ifndef SCHEDULERENV_H
#define SCHEDULERENV_H

#include "Scheduler.h"

class SchedulerEnv : public Scheduler {
public:
  SchedulerEnv(unsigned nbDevices, DeviceInfo *devInfo,
	       unsigned numArgs,
	       unsigned nbGlobals, KernelAnalysis *analysis,
	       unsigned id);
  virtual ~SchedulerEnv();

  // Try to split the kernel, if so, update DeviceInfo structures and set
  // the split dimension.
  // Otherwise return false.
  virtual int startIter(cl_uint work_dim,
			const size_t *global_work_offset,
			const size_t *global_work_size,
			const size_t *local_work_size,
			unsigned *splitDim);

  virtual void endIter();

private:
  int mCanSplit;
  unsigned iter;
  unsigned startParams;
  unsigned id;

  double *keep_granu_dscr;
  int keep_granu_size;
};

#endif /* SCHEDULERENV_H */
