#ifndef SCHEDULERSAMPLE_H
#define SCHEDULERSAMPLE_H

#include <Scheduler/MultiKernelScheduler.h>

namespace libsplit {

  class SchedulerSample : public MultiKernelScheduler {
  public:

    SchedulerSample(BufferManager *buffManager, unsigned nbDevices);
    virtual ~SchedulerSample();

  protected:
    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete);

    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis);

  private:
    std::map<unsigned, SubKernelSchedInfo *> kerID2SchedInfoMap;

    unsigned getCycleIter() const;
    void simulateOneCycleTransfers();
    void computeOneCycleDataTransfers(size_t *D2HPerKernel,
				      size_t *H2DPerKernel);
    void dumpHeader();
    void dumpCycleResults(size_t *D2HPerKernel, size_t *H2DPerKernel,
			  double *cycleGranus);
    void instantiateCycleAnalysis(double *cycleGranus);
  };

};

#endif /* SCHEDULERSAMPLE */
