#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "DeviceInfo.h"

#include <KernelAnalysis.h>

class Scheduler {
public:

  enum TYPE {
    AKGR,
    MKGR,
    BADBROYDEN,
    BROYDEN,
    FIXPOINT,
    ENV
  };

  enum STATUS {
    SPLIT, // Can split
    MERGE, // Enough work groups but analysis failed
    FAIL // Not enough work groups
  };

  Scheduler(unsigned nbDevices, DeviceInfo *devInfo,
	    unsigned numArgs,
	    unsigned nbGlobals, KernelAnalysis *analysis);
  virtual ~Scheduler();

  // Try to split the kernel, if so, update DeviceInfo structures.
  // Otherwise return false.
  virtual int startIter(cl_uint work_dim,
			const size_t *global_work_offset,
			const size_t *global_work_size,
			const size_t *local_work_size,
			unsigned *splitDim) = 0;

  virtual void endIter() = 0;

  // Store arg value as integer. The value is then used to instantiate the
  // analysis.
  void storeArgValue(unsigned argIndex, int value);

protected:
  unsigned nbDevices;
  DeviceInfo *devInfo;

  // Granularity and performance descriptor
  int size_gr;
  double *granu_dscr; // <dev, nb, granu, ... >
  int size_perf_descr;
  double *kernel_perf_descr; // dev, granu, perf

  unsigned numArgs;
  int *argValues;

  unsigned nbGlobals;
  KernelAnalysis *mAnalysis;

  bool mNeedToInstantiate;

  // Instantiate analysis with split parameters and original ndrange.
  // This function determines if the kernel can be split according to the
  // spit parameters. It updates the sub-kernels NDRanges
  // (mGlobalWorkSize, mGlobalWorkOffset) in DeviceInfo.
  bool instantiateAnalysis(cl_uint work_dim,
			   const size_t *global_work_offset,
			   const size_t *global_work_size,
			   const size_t *local_work_size,
			   unsigned splitDim);

  bool instantiateMergeAnalysis(cl_uint work_dim,
				const size_t *global_work_offset,
				const size_t *global_work_size,
				const size_t *local_work_size,
				unsigned splitDim);

  // Compute the array of dimension id from the one with
  // the maximum number of splits.
  void getSortedDim(cl_uint work_dim,
		    const size_t *global_work_size,
		    const size_t *local_work_size,
		    unsigned *order);

  void updateDeviceInfo();

  unsigned iterno;
  unsigned id;

};

#endif /* SCHEDULER_H */
