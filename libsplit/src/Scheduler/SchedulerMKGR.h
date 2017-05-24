#ifndef SCHEDULERMKGR_H
#define SCHEDULERMKGR_H

#include <Scheduler/MultiKernelSolver.h>
#include <Scheduler/MultiKernelScheduler.h>


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
    MultiKernelSolver *solver;

    double *req_cycle_granu_dscr; // cycleLength * nbDevices
    double *real_cycle_granu_dscr; // cycleLength * nbDevices
    double *cycle_kernel_perf; // cycleLength * nbDevices

    std::map<unsigned, SubKernelSchedInfo *> kerID2SchedInfoMap;
    std::map<double, double> **D2HFunctionSampling; // cycleLength * nbDevices
    std::map<double, double> **H2DFunctionSampling; // cycleLength * nbDevices

    double *D2HCoef; // cycleLength * nbDevices;
    double *H2DCoef; // cycleLength * nbDevices;

    unsigned getCycleIter() const;
    void getRealCycleGranuDscr();
    void getCycleKernelPerf();
    void updateCommConstraints();
    void setCycleKernelPerformance();
    void setCycleKernelGranuDscr();
  };

};

#endif /* SCHEDULERMKGR */
