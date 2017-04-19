#ifndef SCHEDULERAUTOABS_H
#define SCHEDULERAUTOABS_H

#include "Scheduler.h"

class SchedulerAutoAbs : public Scheduler {
public:
  SchedulerAutoAbs(unsigned nbDevices, DeviceInfo *devInfo,
		   unsigned numArgs,
		   unsigned nbGlobals, KernelAnalysis *analysis);
  virtual ~SchedulerAutoAbs();

  // Try to split the kernel, if so, update DeviceInfo structures and set
  // the split dimension.
  // Otherwise return false.
  virtual int startIter(cl_uint work_dim,
			const size_t *global_work_offset,
			const size_t *global_work_size,
			const size_t *local_work_size,
			unsigned *splitDim);

  virtual void endIter();

protected:

  virtual void initGranu();

  virtual void initScheduler(const size_t *global_work_size,
			     const size_t *local_work_size, unsigned dim) = 0;

  virtual void getGranu() = 0;

  int mCanSplit;
  bool mExecuted;

  // Kernel time on each device.
  double *kernel_time;

  // Transfer time on each device.
  double *comm_time;

  // Best granularity
  bool bestReached;
  double bestTime;
  int best_size_gr;
  double *best_granu_dscr;

  // Global and local size of the split dimension.
  int globalSize;
  int localSize;

private:
  void updatePerfDescrWithComm();
  void updatePerfDescrWithoutComm();

  bool getNextGranuFull(cl_uint work_dim,
			const size_t *global_work_offset,
			const size_t *global_work_size,
			const size_t *local_work_size,
			unsigned *splitDim);
  bool getNextGranuBest(cl_uint work_dim,
  			const size_t *global_work_offset,
  			const size_t *global_work_size,
  			const size_t *local_work_size,
  			unsigned *splitDim);
  bool getNextGranuNil(cl_uint work_dim,
  		       const size_t *global_work_offset,
  		       const size_t *global_work_size,
  		       const size_t *local_work_size,
  		       unsigned *splitDim);

  void (SchedulerAutoAbs::*updatePerfDescr)();
  bool (SchedulerAutoAbs::*getNextGranu)(cl_uint work_dim,
  					 const size_t *global_work_offset,
  					 const size_t *global_work_size,
  					 const size_t *local_work_size,
  					 unsigned *splitDim);

  bool stopBest;
};

#endif /* SCHEDULERAUTO_H */
