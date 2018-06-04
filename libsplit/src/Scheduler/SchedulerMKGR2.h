#ifndef SCHEDULERMKGR2_H
#define SCHEDULERMKGR2_H

#include <Scheduler/MultiKernelSolver.h>
#include <Scheduler/MultiKernelScheduler.h>

#include <set>

namespace libsplit {

  class SchedulerMKGR2 : public MultiKernelScheduler {
  public:

    SchedulerMKGR2(BufferManager *buffManager, unsigned nbDevices);
    virtual ~SchedulerMKGR2();

    void plotD2HPoints() const;
    void plotH2DPoints() const;

  protected:
    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete);

    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis);

  private:
    const double RESIDUAL_TRESHOLD = 0.6;

    MultiKernelSolver *solver;

    double *req_cycle_granu_dscr; // cycleLength * nbDevices
    double *real_cycle_granu_dscr; // cycleLength * nbDevices
    double *cycle_kernel_perf; // cycleLength * nbDevices

    std::map<unsigned, SubKernelSchedInfo *> kerID2SchedInfoMap;

    // Transfer sampling in bytes
    std::map<unsigned, std::map<unsigned, std::map<unsigned,
      std::vector<std::pair<double, double> > > > > dst2SrcBufferH2DSampling;
    std::map<unsigned, std::map<unsigned, std::map<unsigned,
      std::vector<std::pair<double, double> > > > > dst2SrcBufferD2HSampling;

    // Kernel dependencies
    std::map<unsigned, std::set<unsigned> >
    D2HDependencies; // dst to src
    std::map<unsigned, std::set<unsigned> >
    H2DDependencies; // dst to src

    // Transfer coefficients
    std::map<unsigned, std::map<unsigned, std::map<unsigned, double> > >
    D2HCoefsPerBuffer; // dst to src to buffer to coef
    std::map<unsigned, std::map<unsigned, std::map<unsigned, double> > >
    H2DCoefsPerBuffer; // dst to src to buffer to coef

    int *isD2HCoefUpdated; // cycleLength * cycleLength (dst to src)
    int *isH2DCoefUpdated; // cycleLength * cycleLength (dst to src)

    // Constraint coefficients
    double *kernelsD2HCoefs; // cycleLength * cycleLength * nbDevices (dst to src)
    double *kernelsH2DCoefs; // cycleLength * cycleLength * nbDevices (dst to src)

    // Transfer throughput coef per device
    double *H2DThroughputCoefs;
    double *D2HThroughputCoefs;
    bool *isDeviceH2DThroughputSampled;
    bool *isDeviceD2HThroughputSampled;


    unsigned getCycleIter() const;
    void getRealCycleGranuDscr();
    void getCycleKernelPerf();
    void setCycleKernelPerformance();
    void setCycleKernelGranuDscr();

    void sampleDevicesThroughput();

    void sampleTransfers();
    void sampleKernelTransfers(unsigned kerId);
    void sampleKernelTransfersForBuffer(unsigned kerId, MemoryHandle *m);

    void updateKernelsCommConstraints();
  };

};

#endif /* SCHEDULERMKGR2 */
