#ifndef SCHEDULERMKStatic_H
#define SCHEDULERMKStatic_H

#include <Scheduler/MultiKernelScheduler.h>

namespace libsplit {

  class SchedulerMKStatic : public MultiKernelScheduler {
  public:

    SchedulerMKStatic(BufferManager *buffManager, unsigned nbDevices);
    virtual ~SchedulerMKStatic();

  protected:
    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete);

    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis);

  private:

    double *req_cycle_granu_dscr; // cycleLength * nbDevices
    double *real_cycle_granu_dscr; // cycleLength * nbDevices
    double *cycle_kernel_perf; // cycleLength * nbDevices

    std::map<unsigned, SubKernelSchedInfo *> kerID2SchedInfoMap;

    unsigned getCycleIter() const;
  };

};

#endif /* SCHEDULERMKStatic */
