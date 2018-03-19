#include <Scheduler/SchedulerFixedPoint.h>
#include <Utils/Algebra.h>
#include <Utils/Debug.h>

#include <cassert>
#include <math.h>

namespace libsplit {
  SchedulerFixedPoint::SchedulerFixedPoint(BufferManager *buffManager,
					   unsigned nbDevices)
    : SingleKernelScheduler(buffManager, nbDevices) {}

  SchedulerFixedPoint::~SchedulerFixedPoint() {
    // TODO
  }


  void
  SchedulerFixedPoint::getInitialPartition(SubKernelSchedInfo *SI,
					   unsigned kerId,
					   bool *needOtherExecToComplete) {
    (void) kerId;
    *needOtherExecToComplete = false;

    assert(kernel2MatrixMap.find(SI) == kernel2MatrixMap.end());

    FixedPointMatrix *BM = new FixedPointMatrix(nbDevices);

    for (unsigned i=0; i<nbDevices; i++) {
      SI->req_granu_dscr[i*3] = i;
      SI->req_granu_dscr[i*3+1] = 1;
      SI->req_granu_dscr[i*3+2] = 1.0 / nbDevices;
      BM->x[i] = SI->req_granu_dscr[i*3+2];
    }

    kernel2MatrixMap[SI] = BM;
  }

  void
  SchedulerFixedPoint::getNextPartition(SubKernelSchedInfo *SI,
					unsigned kerId,
					bool *needOtherExecToComplete,
					bool *needToInstantiateAnalysis) {
    (void) kerId;

    *needOtherExecToComplete = false;
    *needToInstantiateAnalysis = true;

    unsigned nbSplits = SI->real_size_gr / 3;
    FixedPointMatrix *BM = kernel2MatrixMap[SI];

    SI->updateTimers();
    SI->updatePerfDescr();

    DEBUG("timers",
	  SI->printTimers());


   // Set real x;
   for (unsigned i=0; i<nbSplits; i++) {
     BM->x[i] = SI->kernel_perf_dscr[i*3+1];
   }

   // Compute Fx
   double fi[nbDevices];
   for (unsigned i=0; i<nbSplits; i++)
     fi[i] = (this->*getSubkernelPerf)(SI, i) / BM->x[i];

   double alpha = 0;
   for (unsigned i=0; i<nbSplits; i++)
     alpha += 1.0 / fi[i];
   alpha = 1.0 / alpha;

   for (unsigned i=0; i<nbSplits; i++)
     BM->Fx[i] = alpha / fi[i];

   // Update x
   for (unsigned i=0; i<nbSplits; i++)
     BM->x[i] = BM->Fx[i];

   // Update granu dscr
   for (unsigned i=0; i<nbSplits; i++) {
     SI->req_granu_dscr[i*3+1] = 1; // nb kernels
     SI->req_granu_dscr[i*3+2] = BM->x[i];
   }
 }

};
