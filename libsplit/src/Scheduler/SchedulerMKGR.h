#ifndef SCHEDULERMKGR_H
#define SCHEDULERMKGR_H

#include <Scheduler/MultiKernelSolver.h>
#include <Scheduler/MultiKernelScheduler.h>

#include <set>

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

    // Constraint coefficients
    double *kernelsD2HCoefs; // cycleLength * cycleLength * nbDevices (dst to src to dev)
    double *kernelsH2DCoefs; // cycleLength * cycleLength * nbDevices (dst to src to dev)

    // Constraint function sampling
    std::map<double, double> *kernelsD2HSampling; // cycleLength * cycleLength * nbDevice (dst to src to dev)
    std::map<double, double> *kernelsH2DSampling; // cycleLength * cycleLength * nbDevices (dst to src to dev)

    // Kernel dependencies
    std::map<unsigned, std::map<unsigned, std::set<unsigned> > > D2HDependencies; // dst to src to dev
    std::map<unsigned, std::map<unsigned, std::set<unsigned> > > H2DDependencies; // dst to src to dev

    unsigned getCycleIter() const;
    void getRealCycleGranuDscr();
    void getCycleKernelPerf();
    void setCycleKernelPerformance();
    void setCycleKernelGranuDscr();
    void updateKernelsCommConstraints();
  };

};

#endif /* SCHEDULERMKGR */
