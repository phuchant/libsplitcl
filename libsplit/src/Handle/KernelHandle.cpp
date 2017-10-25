#include <Define.h>
#include <Handle/KernelHandle.h>
#include <Options.h>
#include <Utils/Debug.h>
#include <Utils/Utils.h>
#include <IndexExpr/IndexExprValue.h>

#include <cassert>
#include <iostream>
#include <fcntl.h>
#include <malloc.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

namespace libsplit {

  unsigned KernelHandle::numKernels = 0;

  KernelArg::KernelArg(size_t size, bool local, const void *value)
    : size(size), local(local) {
    if (!local)
      memcpy((void *) this->value, value, size);
  }

  KernelArg::KernelArg(const KernelArg &o)
    : size(o.size), local(o.local) {
    if (!o.local)
      memcpy(value, o.value, o.size);
  }

  KernelArg &
  KernelArg::operator=(const KernelArg &o) {
    size = o.size;
    local = o.local;
    if (!local)
      memcpy(value, o.value, o.size);

    return *this;
  }

  KernelArg::~KernelArg() {}

  void updateKernelArg(KernelArgs &args, cl_uint idx, KernelArg &a)
  {
    KernelArgs::iterator it = args.find(idx);
    if( it == args.end())
      args.insert(std::pair<cl_uint, KernelArg>(idx, a));
    else
      it->second = a;
  }


  KernelHandle::KernelHandle(ProgramHandle *p, const char *kernel_name)
    : mExecuted(false), mNbSkipIter(0), mIteration(0),
      mProgram(p), mSingleDeviceID(0)
  {
    // Retain program
    p->retain();

    // Get name
    mName = strdup(kernel_name);

    id = ++numKernels;

    DEBUG("kernelstats",
	  std::cerr << "kernel " << mName << " id " << id << "\n";
	  );

    // Get context
    mContext = mProgram->getContext();

    // Create one cl_kernel per device
    cl_int err;

    mNbSubKernels = mContext->getNbDevices();
    mSubKernels = new cl_kernel[mNbSubKernels];
    for (unsigned i=0; i<mNbSubKernels; i++) {
      mSubKernels[i] = real_clCreateKernel(p->getProgram(i), kernel_name, &err);
      clCheck(err, __FILE__, __LINE__);
    }

    // Arguments
    err = real_clGetKernelInfo(mSubKernels[0], CL_KERNEL_NUM_ARGS,
			       sizeof(mNumArgs), &mNumArgs, NULL);
    clCheck(err, __FILE__, __LINE__);
    mNumArgs -= 2;

    mAnalysis = NULL;
    mNbGlobalArgs = 0;
    subkernelArgs = new KernelArgs[mNbSubKernels];

    // Launch analysis
    launchAnalysis();

    for (unsigned i=0; i<mNumArgs; i++)
      argsValues.push_back(nullptr);

    // Get environment variables SINGLEID, DONTSPLIT and SCHED and instantiate
    // the scheduler.
    getEnvOptions();

    if (//!strcmp(mName, "create_seq") || // bug SCEV MAX (if k2 < NUM_KEYS) fixed ?
	false)
      mDontSplit = true;

    if (!strcmp(mName, "reduce_kernel") ||
	!strcmp(mName, "normalize_weights_kernel") ||
	false)
      mDontSplit = true;

    // // SOTL
    // if (!strcmp(mName, "scan") ||
    //     !strcmp(mName, "scan_down_step") ||
    //     !strcmp(mName, "box_sort") ||
    //     !strcmp(mName, "reset_int_buffer") ||
    //     !strcmp(mName, "box_count"))
    //   mDontSplit = true;

  }



  KernelHandle::~KernelHandle() {
    for (unsigned i=0; i<mNbSubKernels; i++) {
      cl_int err = real_clReleaseKernel(mSubKernels[i]);
      clCheck(err, __FILE__, __LINE__);
    }

    for (unsigned i=0; i<mNumArgs; i++)
      delete argsValues[i];

    mProgram->release();

    delete[] mSubKernels;
    delete[] subkernelArgs;

    delete mAnalysis;

    free(mName);
  }

  void
  KernelHandle::setKernelArg(cl_uint arg_index, size_t arg_size,
			     const void *arg_value) {
    assert(arg_index < mNumArgs);

    // Store scalar argument value
    if (mAnalysis->argIsScalar(arg_index)) {
      delete argsValues[arg_index];
      switch (mAnalysis->getScalarArgType(arg_index)) {
      case ArgumentAnalysis::BOOL:
	{
	  bool value;
	  value = *((bool *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::UCHAR:
	{
	  unsigned char value;
	  value = *((unsigned char *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::CHAR:
	{
	  char value;
	  value = *((char *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::SHORT:
	{
	  short value;
	  value = *((short *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::USHORT:
	{
	  unsigned short value;
	  value = *((unsigned short *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::INT:
	{
	  int value;
	  value = *((int *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::UINT:
	{
	  unsigned int value;
	  value = *((unsigned int *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::LONG:
	{
	  long value;
	  value = *((long *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::ULONG:
	{
	  unsigned long value;
	  value = *((unsigned long *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createLong((long) value);
	  break;
	}
      case ArgumentAnalysis::FLOAT:
	{
	  float value;
	  value = *((float *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createFloat(value);
	  break;
	}
      case ArgumentAnalysis::DOUBLE:
	{
	  double value;
	  value = *((double *) arg_value);
	  argsValues[arg_index] = IndexExprValue::createDouble(value);
	  break;
	}
      case ArgumentAnalysis::UNKNOWN:
	std::cerr << "Error: unknown scalar type !\n";
	exit(EXIT_FAILURE);
      };
    }

    // Buffers are considered as long
    else {
      long value = (long) *((MemoryHandle **) arg_value);
      argsValues[arg_index] = IndexExprValue::createLong(value);
    }

    // If it is a global argument or a constant argument,
    // set different membuffer for each split kernel
    if (argIsGlobalMap[arg_index]) {
      MemoryHandle *mem = *((MemoryHandle **) arg_value);
      globalArg2MemHandleMap[argPos2GlobalPosMap[arg_index]] = mem;

      for (unsigned i=0; i<mNbSubKernels; i++) {
	KernelArg a = KernelArg(arg_size, false, &mem->mBuffers[i]);
	updateKernelArg(subkernelArgs[i], arg_index, a);
      }
    } else { // Not global address space nor constant
      for (unsigned i=0; i<mNbSubKernels; i++) {
	KernelArg a = KernelArg(arg_size, arg_value == NULL ? true : false,
				arg_value);
	updateKernelArg(subkernelArgs[i], arg_index, a);
      }
    }
  }

  void
  KernelHandle::setNumgroupsArg(unsigned dev, int numgroups) {
    KernelArg a = KernelArg(sizeof(int), false, (void *) &numgroups);
    updateKernelArg(subkernelArgs[dev], mNumArgs, a);
  }

  void
  KernelHandle::setSplitdimArg(unsigned dev, int splitdim) {
    KernelArg a = KernelArg(sizeof(int), false, (void *) &splitdim);
    updateKernelArg(subkernelArgs[dev], mNumArgs+1, a);
  }

  KernelArgs
  KernelHandle::getKernelArgsForDevice(unsigned i) {
    return subkernelArgs[i];
  }

  unsigned
  KernelHandle::getId() const {
    return id;
  }

  void
  KernelHandle::getKernelWorkgroupInfo(cl_device_id device,
				       cl_kernel_work_group_info param_name,
				       size_t param_value_size,
				       void *param_value,
				       size_t *param_value_size_ret) {
    (void) device;
    cl_int err;

    err =  real_clGetKernelWorkGroupInfo(mSubKernels[0],
					 mContext->getDevice(0),
					 param_name,
					 param_value_size, param_value,
					 param_value_size_ret);
    clCheck(err, __FILE__, __LINE__);
  }

  void
  KernelHandle::getKernelInfo(cl_kernel_info param_name,
			      size_t param_value_size,
			      void *param_value,
			      size_t *param_value_size_ret) {
    cl_int err;

    err =  real_clGetKernelInfo(mSubKernels[0], param_name, param_value_size,
				param_value, param_value_size_ret);
    clCheck(err, __FILE__, __LINE__);
  }

  ContextHandle *
  KernelHandle::getContext() {
    return mContext;
  }

  cl_kernel
  KernelHandle::getDeviceKernel(unsigned d) {
    return mSubKernels[d];
  }

  KernelAnalysis *
  KernelHandle::getAnalysis() {
    return mAnalysis;
  }

  const std::vector<IndexExprValue *> &
  KernelHandle::getArgsValues() const {
    return argsValues;
  }

  MemoryHandle *
  KernelHandle::getGlobalArgHandle(unsigned i) {
    return globalArg2MemHandleMap[i];
  }

  ArgumentAnalysis::TYPE
  KernelHandle::getBufferType(MemoryHandle *m) {
    bool found = false;
    unsigned i=0;
    for (auto I : globalArg2MemHandleMap) {
      if (I.second == m) {
	i = I.first;
	found = true;
	break;
      }
    }

    assert(found);

    return getAnalysis()->getGlobalArgType(i);
  }

  void
  KernelHandle::launchAnalysis() {
    // Launch KernelAnalysis Pass over the LLVM IR binary
    char opt_command[1024];
    sprintf(opt_command, "%s -load %s %s -kernelname=%s .spir%d.bc > /dev/null",
	    OPTPATH, KERNELANALYSIS_SO_PATH, PASSARG, mName, mProgram->getId());

    DEBUG("kernelhandle",
	  std::cerr << "opt command: " << opt_command << "\n");
    int err = system(opt_command);
    assert(err == 0);

    // Get results from analysis using mmap
    int fd;
    char *data;

    if ((fd = open("analysis.txt", O_RDONLY)) == -1) {
      perror("libhook open");
      exit(EXIT_FAILURE);
    }

    size_t len = 64 * getpagesize();
    data = (char *) mmap((caddr_t) 0, len, PROT_READ, MAP_SHARED, fd, 0);
    if (data == (char *) (caddr_t) -1) {
      perror("libhook mmap");
      exit(EXIT_FAILURE);
    }

    close(fd);

    std::string str(data, len);
    std::stringstream ss;
    ss << str;

    mAnalysis = KernelAnalysis::open(ss);
    DEBUG("analysis",
	  mAnalysis->debug(););

    assert(!strcmp(mAnalysis->getName(), mName));

    // Fill argIsGlobalMap
    for (unsigned i=0; i<mNumArgs; i++) {
      argIsGlobalMap[i] = mAnalysis->argIsGlobal(i);
      if (!argIsGlobalMap[i])
	continue;

      argPos2GlobalPosMap[i] = mAnalysis->getGlobalArgId(i);
    }

    // Fill globalArg2PosMap position
    mNbGlobalArgs = mAnalysis->getNbGlobalArguments();
    for (unsigned i=0; i<mNbGlobalArgs; i++)
      globalArg2PosMap[i] = mAnalysis->getGlobalArgPos(i);
  }

  void
  KernelHandle::getEnvOptions() {
    // Get single device ID
    mSingleDeviceID = optSingleDeviceID;

    // Get DONTSPLIT option
    mDontSplit = optDontSplit;

    // Get NBSKIPITER option
    mNbSkipIter = optNbSkipIter;
  }

  const char *
  KernelHandle::getName() const {
    return mName;
  }

};
