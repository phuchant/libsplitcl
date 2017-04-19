#include <Debug.h>
#include "SchedulerBadBroyden.h"
#include "Algebra.h"

#include <iostream>
#include <cmath>

SchedulerBadBroyden::SchedulerBadBroyden(unsigned nbDevices, DeviceInfo *devInfo,
					 unsigned numArgs,
					 unsigned nbGlobals,
					 KernelAnalysis *analysis)
  : SchedulerAutoAbs(nbDevices, devInfo, numArgs, nbGlobals, analysis),
    iterno(0) {

  // Alloc vectors and matrix
  x = new double[nbDevices];
  y = new double[nbDevices];
  z = new double[nbDevices];
  Fx = new double[nbDevices];
  Fy = new double[nbDevices];
  Fz = new double[nbDevices];
  Gx = new double[nbDevices];
  Gy = new double[nbDevices];
  Gz = new double[nbDevices];
  J = getIdMatrix(nbDevices-1);
  deltaX = new double[nbDevices];
  deltaF = new double[nbDevices];

  JF = new double[nbDevices];
  JDF = new double[nbDevices];
  dxJ = new double[nbDevices];
  mUpdate = new double[(nbDevices) * (nbDevices)];
}

SchedulerBadBroyden::~SchedulerBadBroyden() {
  delete[] x;
  delete[] y;
  delete[] z;
  delete[] Fx;
  delete[] Fy;
  delete[] Fz;
  delete[] Gx;
  delete[] Gy;
  delete[] Gz;
  delete[] J;
  delete[] deltaX;
  delete[] deltaF;
  delete[] JF;
  delete[] JDF;
  delete[] dxJ;
  delete[] mUpdate;
}

void
SchedulerBadBroyden::initScheduler(const size_t *global_work_size,
				   const size_t *local_work_size, unsigned dim) {
  (void) global_work_size;
  (void) local_work_size;
  (void) dim;

  for (unsigned i=0; i<nbDevices; i++)
    x[i] = granu_dscr[i*3+2];
}

void
SchedulerBadBroyden::getGranu()
{
  iterno++;

  unsigned nbSplits = size_gr / 3;

  if (nbSplits < nbDevices)
    return;

  // Set real x;
  for (unsigned i=0; i<nbDevices; i++) {
    if (kernel_perf_descr[i*3] != i) {
      std::cerr << "ER\n";
      exit(0);
    }
    x[i] = kernel_perf_descr[i*3+1];
  }

  // Compute Fx
  double fi[nbDevices];
  for (unsigned i=0; i<nbDevices; i++)
    fi[i] = kernel_perf_descr[i*3+2] / x[i];

  double alpha = 0;
  for (unsigned i=0; i<nbDevices; i++)
    alpha += 1.0 / fi[i];
  alpha = 1.0 / alpha;

  for (unsigned i=0; i<nbDevices; i++)
    Fx[i] = alpha / fi[i];

  // Compute Gx
  for (unsigned i=0; i<nbDevices; i++)
    Gx[i] = Fx[i] - x[i];

  // z = x;
  std::copy(x, x + nbDevices, z);
  std::copy(Fx, Fx + nbDevices, Fz);
  std::copy(Gx, Gx + nbDevices, Gz);

  // Update Jacobian matrix
  if (iterno >= 2) {
    // Update Jacobian Matrix

    // Compute deltaX
    caxpy(-1, y, x, deltaX, nbDevices-1);

    // Compute deltaF
    caxpy(-1, Gy, Gx, deltaF, nbDevices-1);

    // Compute JDF = J * deltaF;
    gemm(nbDevices-1, nbDevices-1, 1, J, deltaF, JDF);

    double normF = norm(Gx, nbDevices-1);

    DEBUG("broyden",
	  std::cerr << "normF : " << normF << "\n";
	  );

    if (normF < 1e-12)
      return;

    // Compute deltaXt * J
    gemm(1, nbDevices-1, nbDevices-1, deltaX, J, dxJ);

    // Compute deltaX - JDF in place
    axpy(-1, JDF, deltaX, nbDevices-1);

    // Compute (deltaX - JDF) / normF
    scal((double) 1.0 / normF, deltaX, nbDevices-1);

    // compute ( (deltaX - JDF) / normF) ) * dxJ
    gemm(nbDevices-1, 1, nbDevices-1, deltaX, dxJ, mUpdate);

    // J = J + mUpdate
    axpy(1, mUpdate, J, (nbDevices-1) * (nbDevices-1));
  }

  // Update x
  // x = x - J * f(x_n))

  // Jn * f(x_n)
  gemm(nbDevices-1, nbDevices-1, 1, J, Gx, JF);

  // x_n = x_n - Jn * f(x_n)
  axpy(-1, JF, x, nbDevices-1);

  double sum = 0;
  for (unsigned i=0; i<nbDevices-1; i++) {
    x[i] = x[i] < 0 ? 0 : x[i];
    sum += x[i];
  }
  x[nbDevices-1] = 1.0 - sum;

  // Update granu dscr
  for (unsigned i=0; i<nbDevices; i++) {
    granu_dscr[i*3] = i; // dev id;
    granu_dscr[i*3+1] = 1; // nb kernels
    granu_dscr[i*3+2] = x[i];
  }

  // y = z;
  std::copy(z, z + nbDevices, y);
  std::copy(Fz, Fz + nbDevices, Fy);
  std::copy(Gz, Gz + nbDevices, Gy);
}
