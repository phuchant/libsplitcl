#ifndef SCHEDULERBADBROYDEN_H
#define SCHEDULERBADBROYDEN_H

#include <Scheduler/SingleKernelScheduler.h>
#include <Utils/Algebra.h>

namespace libsplit {

  class SchedulerBadBroyden : public SingleKernelScheduler {
  public:

    SchedulerBadBroyden(BufferManager *buffManager, unsigned nbDevices);
    virtual ~SchedulerBadBroyden();

  protected:
    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete);
    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis);

  private:
    struct BroydenMatrix;
    std::map<SubKernelSchedInfo *, BroydenMatrix *> kernel2MatrixMap;

    struct BroydenMatrix {
      BroydenMatrix(unsigned nbDevices) {
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

      ~BroydenMatrix() {
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

      double *x, *y, *z; // granus
      double *Fx, *Fy, *Fz; // f(x)
      double *Gx, *Gy, *Gz; // F(x) = f(x) - x

      double *J; // Jacobian Matrix (size nbDevice-1)
      double *deltaX; // x_n - x_{n-1}
      double *deltaF; // F_n - F_{n-1}

      double *JF; // J_n * F_n
      double *JDF; // J_n * delta_f
      double *dxJ; // delta_x * J

      double *mUpdate;
    };

  };

};

#endif /* SCHEDULERBADBROYDEN_H */
