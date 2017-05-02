#include <Scheduler/SchedulerBroyden.h>
#include <Utils/Algebra.h>
#include <Utils/Debug.h>

#include <cassert>
#include <math.h>

namespace libsplit {
  SchedulerBroyden::SchedulerBroyden(BufferManager *buffManager,
					   unsigned nbDevices)
    : SingleKernelScheduler(buffManager, nbDevices) {}

  SchedulerBroyden::~SchedulerBroyden() {
    // TODO
  }


  void
  SchedulerBroyden::getInitialPartition(SubKernelSchedInfo *SI,
					   unsigned kerId,
					   bool *needOtherExecToComplete) {
    (void) kerId;
    *needOtherExecToComplete = false;

    assert(kernel2MatrixMap.find(SI) == kernel2MatrixMap.end());

    BroydenMatrix *BM = new BroydenMatrix(nbDevices);

    for (unsigned i=0; i<nbDevices; i++) {
      SI->req_granu_dscr[i*3] = i;
      SI->req_granu_dscr[i*3+1] = 1;
      SI->req_granu_dscr[i*3+2] = 1.0 / nbDevices;
      BM->x[i] = SI->req_granu_dscr[i*3+2];
    }

    kernel2MatrixMap[SI] = BM;
  }

  void
  SchedulerBroyden::getNextPartition(SubKernelSchedInfo *SI,
					unsigned kerId,
					bool *needOtherExecToComplete,
					bool *needToInstantiateAnalysis) {
    (void) kerId;

    *needOtherExecToComplete = false;
    *needToInstantiateAnalysis = true;

    unsigned nbSplits = SI->real_size_gr / 3;
    BroydenMatrix *BM = kernel2MatrixMap[SI];

   if (nbSplits < nbDevices)
     return;

   // Set real x;
   for (unsigned i=0; i<nbDevices; i++) {
     if (SI->kernel_perf_dscr[i*3] != i) {
       std::cerr << "ER\n";
       exit(0);
     }
     BM->x[i] = SI->kernel_perf_dscr[i*3+1];
   }

   // Compute Fx
   double fi[nbDevices];
   for (unsigned i=0; i<nbDevices; i++)
     fi[i] = SI->kernel_perf_dscr[i*3+2] / BM->x[i];

   double alpha = 0;
   for (unsigned i=0; i<nbDevices; i++)
     alpha += 1.0 / fi[i];
   alpha = 1.0 / alpha;

   for (unsigned i=0; i<nbDevices; i++)
     BM->Fx[i] = alpha / fi[i];

   // Compute Gx
   for (unsigned i=0; i<nbDevices; i++)
     BM->Gx[i] = BM->Fx[i] - BM->x[i];

   // z = x;
   std::copy(BM->x, BM->x + nbDevices, BM->z);
   std::copy(BM->Fx, BM->Fx + nbDevices, BM->Fz);
   std::copy(BM->Gx, BM->Gx + nbDevices, BM->Gz);

   // Update Jacobian matrix
   if (SI->iterno >= 2) {
     // Update Jacobian Matrix

     // Compute deltaX
     caxpy(-1, BM->y, BM->x, BM->deltaX, nbDevices-1);

     for (unsigned i=0; i<nbDevices-1; i++) {
       if (fabs(BM->deltaX[i] > 1))
	 abort();
     }

     // Compute deltaF
     caxpy(-1, BM->Gy, BM->Gx, BM->deltaF, nbDevices-1);

     // Compute JDF = J * deltaF;
     gemm(nbDevices-1, nbDevices-1, 1, BM->J, BM->deltaF, BM->JDF);

     double dxJdF = dot(BM->deltaX, BM->JDF, nbDevices-1);

    DEBUG("broyden",
	  std::cerr << "dxJdF : " << dxJdF << "\n";
	  );

     if (dxJdF < 1e-12)
       return;

     // Compute deltaXt * J
     gemm(1, nbDevices-1, nbDevices-1, BM->deltaX, BM->J, BM->dxJ);

     // Compute deltaX - JDF in place
     axpy(-1, BM->JDF, BM->deltaX, nbDevices-1);

     // Compute (deltaX - JDF) / dxJdF
     scal((double) 1.0 / dxJdF, BM->deltaX, nbDevices-1);

     // compute ( (deltaX - JDF) / dxJdF) ) * dxJ
     gemm(nbDevices-1, 1, nbDevices-1, BM->deltaX, BM->dxJ, BM->mUpdate);

     // J = J + mUpdate
     axpy(1, BM->mUpdate, BM->J, (nbDevices-1) * (nbDevices-1));
   }

   // Update x
   // x = x - J * f(x_n))

   // Jn * f(x_n)
   gemm(nbDevices-1, nbDevices-1, 1, BM->J, BM->Gx, BM->JF);

   // x_n = x_n - Jn * f(x_n)
   axpy(-1, BM->JF, BM->x, nbDevices-1);

   double sum = 0;
   for (unsigned i=0; i<nbDevices-1; i++) {
     BM->x[i] = BM->x[i] < 0 ? 0 : BM->x[i];
     sum += BM->x[i];
   }
   BM->x[nbDevices-1] = 1.0 - sum;

   // Update granu dscr
   for (unsigned i=0; i<nbDevices; i++) {
     SI->req_granu_dscr[i*3] = i; // dev id;
     SI->req_granu_dscr[i*3+1] = 1; // nb kernels
     SI->req_granu_dscr[i*3+2] = BM->x[i];
   }

   // y = z;
   std::copy(BM->z, BM->z + nbDevices, BM->y);
   std::copy(BM->Fz, BM->Fz + nbDevices, BM->Fy);
   std::copy(BM->Gz, BM->Gz + nbDevices, BM->Gy);
 }

};
