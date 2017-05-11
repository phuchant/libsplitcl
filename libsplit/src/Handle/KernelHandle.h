#ifndef KERNELHANDLE_H
#define KERNELHANDLE_H

#include <Handle/MemoryHandle.h>
#include <Handle/ProgramHandle.h>
#include <Utils/Retainable.h>

#include <KernelAnalysis.h>

#include <CL/opencl.h>

#include <map>

namespace libsplit {

  struct KernelArg {
    KernelArg(size_t size, bool local, const void *value);

    KernelArg(const KernelArg &o);

    KernelArg & operator=(const KernelArg &o);

    ~KernelArg();

    size_t size;
    bool local;
    char value[256];
  };

  typedef std::map<cl_uint, KernelArg> KernelArgs;

  void updateKernelArg(KernelArgs &args, cl_uint idx, KernelArg &a);


  class KernelHandle : public Retainable {
  public:
    KernelHandle(ProgramHandle *p, const char *kernel_name);
    ~KernelHandle();

    void setKernelArg(cl_uint arg_index, size_t arg_size,
		      const void *arg_value);

    void setNumgroupsArg(unsigned dev, int numgroups);
    void setSplitdimArg(unsigned dev, int splitdim);

    KernelArgs getKernelArgsForDevice(unsigned i);

    void getKernelWorkgroupInfo(cl_device_id device,
				cl_kernel_work_group_info param_name,
				size_t param_value_size, void *param_value,
				size_t *param_value_size_ret);
    void getKernelInfo(cl_kernel_info param_name,
		       size_t param_value_size,
		       void *param_value,
		       size_t *param_value_size_ret);

    unsigned getId() const;

    ContextHandle *getContext();

    cl_kernel getDeviceKernel(unsigned d);

    KernelAnalysis *getAnalysis();

    std::vector<int> getArgsValuesAsInt();
    MemoryHandle *getGlobalArgHandle(unsigned i);

    const char *getName() const;

  private:
    // Kernel name
    char *mName;

    static unsigned numKernels;
    unsigned id;

    // Sub-Kernels
    unsigned mNbSubKernels;
    cl_kernel *mSubKernels;

    // Execution
    bool mExecuted;
    unsigned mNbSkipIter;
    unsigned mIteration;

    // Program
    ProgramHandle *mProgram;

    // Context
    ContextHandle *mContext;

    // Arguments
    unsigned mNumArgs; // Number of arguments before transformation
    int *argsValuesAsInt;
    KernelArgs *subkernelArgs;

    // Global arguments
    unsigned mNbGlobalArgs;
    std::map<unsigned, bool> argIsGlobalMap;
    std::map<unsigned, unsigned> globalArg2PosMap;
    std::map<unsigned, unsigned> argPos2GlobalPosMap;
    std::map<unsigned, MemoryHandle *> globalArg2MemHandleMap;

    // Kernel Analysis
    KernelAnalysis *mAnalysis;

    // Split infos
    bool mDontSplit;
    unsigned mSingleDeviceID;

    // Launch KernelAnalysis pass over the spir binary.
    void launchAnalysis();

    // Parse the environment variable SPLITPARAMS to get the split parameters.
    // This function initialize the attributes mNbSplits, mDenominator,
    // mNominators and mSplitDevices.
    // Parse the environment variable SINGLEID to get the device id for the single
    // kernel.
    // Parse the environment variable DONTSPLIT to initalize the attribute
    // mDontSplit.
    void getEnvOptions();
  };

}

#endif /* KERNELHANDLE_H */
