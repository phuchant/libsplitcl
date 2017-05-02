#ifndef SCHEDULERFIXEDPOINT_H
#define SCHEDULERFIXEDPOINT_H

#include <Scheduler/SingleKernelScheduler.h>
#include <Utils/Algebra.h>

namespace libsplit {

  class SchedulerFixedPoint : public SingleKernelScheduler {
  public:

    SchedulerFixedPoint(BufferManager *buffManager, unsigned nbDevices);
    virtual ~SchedulerFixedPoint();

  protected:
    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete);
    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis);

  private:
    struct FixedPointMatrix;
    std::map<SubKernelSchedInfo *, FixedPointMatrix *> kernel2MatrixMap;

    struct FixedPointMatrix {
      FixedPointMatrix(unsigned nbDevices) {
	// Alloc vectors and matrix
	x = new double[nbDevices];
	Fx = new double[nbDevices];
      }

      ~FixedPointMatrix() {
	delete[] x;
	delete[] Fx;
      }

      double *x;
      double *Fx;
    };

  };

};

#endif /* SCHEDULERFIXEDPOINT_H */
