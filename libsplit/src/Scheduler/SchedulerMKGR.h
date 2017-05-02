#ifndef SCHEDULERMKGR_H
#define SCHEDULERMKGR_H

#include <Scheduler/MultiKernelScheduler.h>

extern "C" {
#include <mkgr.h>
}

namespace libsplit {

  class SchedulerMKGR : public MultiKernelScheduler {
  public:

    SchedulerMKGR(BufferManager *buffManager, unsigned nbDevices);
    virtual ~SchedulerMKGR();

  protected:
    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete);

    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis);

  private:
    double objective_time;
    mkgr_t pl;

    double *cycle_granu_dscr; // <kerId, devId, gr, devId, gr, ..., kerId, ...>
    int cycle_granu_size;

    std::map<unsigned, SubKernelSchedInfo *> kerID2SchedInfoMap;
    std::map<double, double> **commFunctionSampling;
    double **commConstraint;
    double **comm_perf_from_to;

    unsigned getCycleIter() const;
    void updateCommConstraints();
    void setCycleCommPerformance();
    void setCycleKernelPerformance();
  };

};

#endif /* SCHEDULERMKGR */
