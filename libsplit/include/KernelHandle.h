#ifndef KERNELHANDLE_H
#define KERNELHANDLE_H

#include "DeviceInfo.h"
#include "MemoryHandle.h"
#include "ProgramHandle.h"
#include "Retainable.h"
#include "Scheduler.h"

#include <KernelAnalysis.h>

#include <CL/opencl.h>

class KernelHandle : public Retainable {
public:
  KernelHandle(ProgramHandle *p, const char *kernel_name);
  ~KernelHandle();

  void setKernelArg(cl_uint arg_index, size_t arg_size,
		    const void *arg_value);
  void enqueueNDRange(cl_command_queue command_queue,
		      cl_uint work_dim,
		      const size_t *global_work_offset,
		      const size_t *global_work_size,
		      const size_t *local_work_size,
		      cl_uint num_events_in_wait_list,
		      const cl_event *event_wait_list,
		      cl_event *event);
  void getKernelWorkgroupInfo(cl_device_id device,
			      cl_kernel_work_group_info param_name,
			      size_t param_value_size, void *param_value,
			      size_t *param_value_size_ret);
  void getKernelInfo(cl_kernel_info param_name,
		     size_t param_value_size,
		     void *param_value,
		     size_t *param_value_size_ret);

  virtual bool release();

private:
  double NDRangeTime;
  double computeReadTime;
  double writeDeviceTime;
  double subkernelsTime;
  double readDevicesTime;

  // Kernel name
  char *mName;

  static unsigned numKernels;
  unsigned id;

  // Kernels
  unsigned mNbKernels;
  cl_kernel *mKernels;

  // Execution
  bool mExecuted;
  int mCanSplit;
  unsigned mNbSkipIter;
  unsigned mIteration;

  // Scheduler;
  Scheduler *sched;

  // Program
  ProgramHandle *mProgram;

  // Context
  ContextHandle *mContext;

  // Devices
  unsigned mNbDevices;
  DeviceInfo *mDevicesInfo;
  unsigned mergeDeviceId;

  // Arguments
  unsigned mNumArgs; // Number of arguments before transformation
  /* int *mArgs; // args values as integer */
  unsigned *mGlobalArgsPos; // position of global args in the kernel
  KernelAnalysis *mAnalysis; // analysis
  MemoryHandle **mMemArgs; // Array of size numargs containing the
  // associated MemoryHandle object for each argument.
  // Index is the real position in the kernel.

  unsigned mNbGlobalArgs;
  bool *mArgIsGlobal; // Index is position in the kernel.
  unsigned mNbWrittenArgs;
  unsigned *mWrittenArgsPos;
  unsigned mNbMergeArgs;
  unsigned *mMergeArgsPos;
  bool *mArgIsMerge;

  // Split infos
  unsigned mSplitDim;
  bool mDontSplit;
  unsigned mSingleDeviceID;

  // Launch KernelAnalysis pass over the spir binary.
  void launchAnalysis();

  // Merge Kernels
  cl_kernel getMergeKernel(unsigned dev, unsigned nbsplits);

  // This function enqueues a single kernel on the device with ID
  // mSingleDeviceID.
  void enqueueSingleKernel(cl_uint work_dim,
			   const size_t *global_work_offset,
			   const size_t *global_work_size,
			   const size_t *local_work_size);

  // This functions enqueues the sub-kernels on their devices using
  // the mDeviceInfo objects and the sub-kernels NDRanges.
  void enqueueSubKernels(cl_uint work_dim,
			 const size_t *local_work_size);

  // This functions enqueues the sub-kernels on their devices using
  // the mDeviceInfo objects and the sub-kernels NDRanges.
  void enqueueFakeSubKernels(cl_uint work_dim,
			     const size_t *local_work_size);

  // This functions enqueues the sub-kernels on their devices using
  // the mDeviceInfo objects and the sub-kernels NDRanges.
  void enqueueSubKernelsOMP(cl_uint work_dim,
			    const size_t *local_work_size);
  // This functions enqueues the sub-kernels on their devices using
  // the mDeviceInfo objects and the sub-kernels NDRanges.
  void enqueueSubKernelsNOOMP(cl_uint work_dim,
			      const size_t *local_work_size);
  // Create an output buffer and input buffers for each written argument
  // and set kernel arguments.
  // Input buffers are only created if the subkernel doesn't belong the
  // the mergedevice's context.
  void createMergeBuffersAndSetArgs(unsigned nbSplits,
				    unsigned *splitDevIdx,
				    cl_kernel mergeKernel,
				    cl_context mergeContext,
				    cl_mem *mergeBuffers,
				    cl_mem *devBuffers,
				    int *maxLen);

  // Read the necessary data from devices and host and write then
  // to the merge context.
  void prepareMergeBuffers(unsigned nbSplits,
			   unsigned *splitDevIdx,
			   cl_context mergeCtxt,
			   cl_mem *devBuffers,
			   cl_mem *mergeBuffers);

  // Read output merged buffers back to the host.
  void readMergeBuffers(DeviceInfo *devInfo,
			cl_mem *mergeBuffers);

  // Compute the number of argument to merge and their position.
  void computeMergeArguments();

  // Enqueue subkernels then merge output.
  void enqueueMergeKernels(cl_uint work_dim,
			   const size_t *local_work_size);

  // Parse the environment variable SPLITPARAMS to get the split parameters.
  // This function initialize the attributes mNbSplits, mDenominator,
  // mNominators and mSplitDevices.
  // Parse the environment variable SINGLEID to get the device id for the single
  // kernel.
  // Parse the environment variable DONTSPLIT to initalize the attribute
  // mDontSplit.
  void getEnvOptions();
};

#endif /* KERNELHANDLE_H */
