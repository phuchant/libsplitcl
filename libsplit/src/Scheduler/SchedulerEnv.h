#ifndef SCHEDULERENV_H
#define SCHEDULERENV_H

#include <Scheduler/SingleKernelScheduler.h>

#include <map>

namespace libsplit {

  class SchedulerEnv : public SingleKernelScheduler {
  public:
    SchedulerEnv(BufferManager *buffManager, unsigned nbDevices);
    virtual ~SchedulerEnv();

  protected:
    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete);
    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis);
  };

};

#endif /* SCHEDULERENV_H */
