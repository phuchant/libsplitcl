#ifndef SCHEDULERBADBROYDEN_H
#define SCHEDULERBADBROYDEN_H

#include "SchedulerAutoAbs.h"

class SchedulerBadBroyden : public SchedulerAutoAbs {
public:
  SchedulerBadBroyden(unsigned nbDevices, DeviceInfo *devInfo,
		      unsigned numArgs,
		      unsigned nbGlobals, KernelAnalysis *analysis);
  virtual ~SchedulerBadBroyden();

protected:
  virtual void initScheduler(const size_t *global_work_size,
			     const size_t *local_work_size, unsigned dim);

  virtual void getGranu();

private:
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
  unsigned iterno;
};

#endif /* SCHEDULERBADBROYDEN_H */
