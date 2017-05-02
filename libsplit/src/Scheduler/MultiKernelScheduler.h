#ifndef MULTIKERNELSCHEDULER_H
#define MULTIKERNELSCHEDULER_H

#include <Handle/KernelHandle.h>
#include <Scheduler/Scheduler.h>
#include <Options.h>

#include <cassert>

namespace libsplit {

  class MultiKernelScheduler : public Scheduler {
  public:
    MultiKernelScheduler(BufferManager *buffManager, unsigned nbDevices)
      : Scheduler(buffManager, nbDevices) {
      cycleLength = optCycleLength;
      assert(cycleLength > 0);
    }

    virtual ~MultiKernelScheduler() {}

  protected:
    unsigned cycleLength;
    std::map<unsigned, KernelHandle *> kerID2HandleMap;

    virtual unsigned getKernelID(KernelHandle *k) {
      unsigned kerId = count % cycleLength;
      if (count < cycleLength)
	kerID2HandleMap[kerId] = k;
      else
	assert(kerID2HandleMap.find(kerId) != kerID2HandleMap.end() &&
	       kerID2HandleMap[kerId] == k);

      return kerId;
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

#endif /* MULTIKERNELSCHEDULER_H */
