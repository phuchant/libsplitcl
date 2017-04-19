#ifndef PROGRAMINFO_H
#define PROGRAMINFO_H

#include "ContextHandle.h"
#include "Retainable.h"

#include <CL/opencl.h>

class ProgramHandle : public Retainable {
private:
  void init(ContextHandle *context);

public:
  ProgramHandle(ContextHandle *context, cl_uint count, const char **strings,
		const size_t *lengths);
  ProgramHandle(ContextHandle *context, const unsigned char *binary,
		size_t length);
  ~ProgramHandle();

  void build(const char *options, void (*pfn_notify)
	     (cl_program, void *user_data), void *user_data);
  void getProgramInfo(cl_program_info param_name, size_t param_value_size,
		      void *param_value, size_t *param_value_size_ret);
  void getProgramBuildInfo(cl_device_id device, cl_program_info param_name,
			   size_t param_value_size, void *param_value,
			   size_t *param_value_size_ret);

  // Getters
  cl_program getProgram(unsigned n);
  ContextHandle *getContext();
  cl_kernel getMergeKernel(unsigned dev, unsigned nbsplits, unsigned nbArgs);

  virtual bool release();

  int getId();

private:
  void createProgramsWithSource(const char *options);
  void createProgramsWithBinary();

  cl_uint mCount;
  char **mStrings;
  const size_t *mLengths;

  unsigned char *mBinary;
  size_t mBinaryLength;
  bool mIsBinary;

  ContextHandle *mContext;

  bool hasBeenBuilt;
  unsigned mNbPrograms;
  cl_program *mPrograms;

  cl_kernel **mMergeKernels;

  int mIdx;

  static int idxCount;
};

#endif /* PROGRAMINFO_H */
