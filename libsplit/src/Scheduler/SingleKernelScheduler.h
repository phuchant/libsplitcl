#ifndef SINGLEKERNELSCHEDULER_H
#define SINGLEKERNELSCHEDULER_H

#include <Handle/KernelHandle.h>
#include <Scheduler/Scheduler.h>

namespace libsplit {
  class SingleKernelScheduler : public Scheduler {
  public:
    SingleKernelScheduler(BufferManager *buffManager, unsigned nbDevices)
      : Scheduler(buffManager, nbDevices) {}

    virtual ~SingleKernelScheduler() {}

  protected:
    virtual unsigned getKernelID(KernelHandle *k) {
      return k->getId();
    }

    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete) = 0;
    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis) = 0;
  };

};

#endif /* SINGLEKERNELSCHEDULER_H */
