#include <Debug.h>
#include "Define.h"
#include "ProgramHandle.h"
#include "Utils.h"

#include <fstream>

#include <cassert>
#include <cstdio>
#include <cstring>

int ProgramHandle::idxCount = -1;

void
ProgramHandle::init(ContextHandle *context) {
  mContext = context;
  hasBeenBuilt = false;
  mIdx = ++idxCount;

  mNbPrograms = mContext->getNbDevices();
  mPrograms = new cl_program[mNbPrograms];

  mMergeKernels = new cl_kernel *[mNbPrograms];
  for (unsigned i=0; i<mNbPrograms; i++)
    mMergeKernels[i] = new cl_kernel[MAXMERGEARGS * mContext->getNbDevices()];
}

ProgramHandle::ProgramHandle(ContextHandle *context, cl_uint count,
			     const char **strings, const size_t *lengths)
  : mCount(count), mLengths(lengths), mBinary(NULL), mBinaryLength(0),
    mIsBinary(false) {
  init(context);

  mStrings = new char *[count];
  for(unsigned i=0; i<count; ++i) {

    if (lengths && lengths[i]) {
      mStrings[i] = new char[lengths[i] + 1];
      memcpy(mStrings[i], strings[i], lengths[i]);
      mStrings[i][lengths[i]] = '\0';
    } else {
      mStrings[i] = new char[strlen(strings[i]) + 1];
      memcpy(mStrings[i], strings[i], strlen(strings[i]));
      mStrings[i][strlen(strings[i])] = '\0';
    }
  }
}

ProgramHandle::ProgramHandle(ContextHandle *context,
			     const unsigned char *binary,
			     size_t length)
  : mCount(0), mStrings(NULL), mLengths(NULL), mBinaryLength(length),
    mIsBinary(true) {
  init(context);

  mBinary = new unsigned char[mBinaryLength];
  memcpy(mBinary, binary, mBinaryLength);
}

ProgramHandle::~ProgramHandle() {
  delete mBinary;

  for (unsigned i=0; i<mCount; ++i)
    delete[] mStrings[i];
  delete[] mStrings;
  delete[] mPrograms;

  for (unsigned i=0; i<mNbPrograms; i++)
    delete[] mMergeKernels[i];
  delete[] mMergeKernels;
}

void
ProgramHandle::build(const char *options, void (*pfn_notify)
		     (cl_program, void *user_data), void *user_data) {
  // Make transformations and create program
  if (mIsBinary)
    createProgramsWithBinary();
  else
    createProgramsWithSource(options);

  // Build programs
  for (unsigned i=0; i<mNbPrograms; i++) {
    cl_int err = real_clBuildProgram(mPrograms[i], 0, NULL, options,
				     pfn_notify, user_data);
    if (err != CL_SUCCESS) {
      size_t len;
      real_clGetProgramBuildInfo(mPrograms[i],
				 mContext->getDevice(i),
				 CL_PROGRAM_BUILD_LOG,
				 0,
				 NULL,
				 &len);

      char *log = new char[len];

      real_clGetProgramBuildInfo(mPrograms[i],
				 mContext->getDevice(i),
				 CL_PROGRAM_BUILD_LOG,
				 len,
				 log,
				 NULL);

      std::cerr << "build error :\n" << log << "\n";
    }
    clCheck(err, __FILE__, __LINE__);
  }

  // If it is not a binary, generate SPIR.
  if (!mIsBinary) {
    char src_filename[64];
    sprintf(src_filename, ".source%d.cl", mIdx);

    // Generate inline sources
    char inline_filename[64];
    sprintf(inline_filename, ".source-inline%d.cl", mIdx);
    {
      char command[1024];
      sprintf(command, "%s %s %s %s > %s 2> /dev/null", CLINLINEPATH,
	      options ? options : "",
	      "-Dcl_khr_fp64", src_filename, inline_filename);
      // cl_khr_fp64 is defined to avoid Clang errors when using doubles

      DEBUG("programhandle",
	    std::cerr << "inline command : " << command << "\n";
	    );

      system(command);
    }

    // Generate spir
    char bc_filename[64];
    sprintf(bc_filename, ".spir%d.bc", mIdx);

    // Generate spir with no optimization
    {
      char command[1024];
      sprintf(command, "%s %s %s %s -o %s",
	      CLANGPATH,
	      SPIRFLAGS,
	      options ? options : "",
	      inline_filename,
	      bc_filename);

      DEBUG("programhandle",
	    std::cerr << "spir command : " << command << "\n";
	    );

      system(command);
    }

    // Opt passes
    {
      char command[2048];
      sprintf(command, "%s %s -o %s < %s",
	      OPTPATH,
	      OPTPASSES,
	      bc_filename,
	      bc_filename);

      DEBUG("programhandle",
	    std::cerr << "opt command : " << command << "\n";
	    );

      system(command);
    }
  }

  hasBeenBuilt = true;

  // Merge kernels handled only with source for the moment.
  if (!mIsBinary) {
    // Create merge kernels
    for (unsigned p=0; p<mNbPrograms; p++) {
      for (unsigned d=1; d<mContext->getNbDevices(); d++) {
	for (unsigned a=0; a<MAXMERGEARGS; a++) {
	  cl_int err;
	  char kernel_name[64];
	  sprintf(kernel_name, "merge%u%u", d+1, a+1);
	  mMergeKernels[p][d*MAXMERGEARGS+a] =
	    real_clCreateKernel(mPrograms[p],
				kernel_name,
				&err);
	  clCheck(err, __FILE__, __LINE__);
	}
      }
    }
  }
}

bool
ProgramHandle::release() {
  if (--ref_count > 0)
    return false;

  cl_int err = CL_SUCCESS;

  if (!hasBeenBuilt)
    return true;

  for (unsigned i=0; i<mNbPrograms; i++)
    err |= real_clReleaseProgram(mPrograms[i]);
  clCheck(err, __FILE__, __LINE__);

  return true;
}

void
ProgramHandle::getProgramInfo(cl_program_info param_name,
			      size_t param_value_size,
			      void *param_value, size_t *param_value_size_ret) {
  if (!hasBeenBuilt) {
    std::cerr << "Error: getProgramBuildInfo called but proram has not been built"
	      << " yet!\n";
    exit(EXIT_FAILURE);
  }

  cl_int err;

  err =  real_clGetProgramInfo(mPrograms[0], param_name, param_value_size,
			       param_value, param_value_size_ret);
  clCheck(err, __FILE__, __LINE__);
}

void
ProgramHandle::getProgramBuildInfo(cl_device_id device,
				   cl_program_info param_name,
				   size_t param_value_size, void *param_value,
				   size_t *param_value_size_ret) {
  if (!hasBeenBuilt) {
    std::cerr << "Error: getProgramBuildInfo called but proram has not been built"
	      << " yet!\n";
    exit(EXIT_FAILURE);
  }

  cl_int err;
  err =  real_clGetProgramBuildInfo(mPrograms[0], mContext->getDevice(0),
				    param_name,
				    param_value_size, param_value,
				    param_value_size_ret);
  clCheck(err, __FILE__, __LINE__);
}

void
ProgramHandle::createProgramsWithSource(const char *options) {
  cl_int err;

  // Write source
  char src_filename[64];
  sprintf(src_filename, ".source%d.cl", mIdx);
  FILE *fd = fopen(src_filename, "w");
  for (unsigned i = 0; i < mCount; ++i)
    fprintf(fd, "%s", mStrings[i]);
  fclose(fd);

  // Transform sources with ClTransform
  char trans_filename[64];
  sprintf(trans_filename, ".trans%d.cl", mIdx);
  {
    char command[1024];
    sprintf(command, "%s %s %s %s > %s 2> /dev/null", CLTRANSFORMPATH,
	    options ? options : "",
	    "-Dcl_khr_fp64", src_filename, trans_filename);
    // cl_khr_fp64 is defined to avoid Clang errors when using doubles

    system(command);
  }

  // Add merge kernels
  {
    char command[1024];
    const char *type = STR(MERGEKERNELS_TYPE);
    sprintf(command, "cat %s/mergekernels-%s.cl >> %s",
    	    MERGEKERNELS_PATH, type, trans_filename);
    system(command);
  }

  // Load transformed source
  char *trans_source = hookcl_file_load(trans_filename);

  // Create split programs
  for (unsigned i=0; i<mNbPrograms; i++) {
    mPrograms[i] = real_clCreateProgramWithSource(mContext->getContext(i),
						  1,
						  (const char **) &trans_source,
						  NULL, &err);
    clCheck(err, __FILE__, __LINE__);
  }

  free(trans_source);
}

void
ProgramHandle::createProgramsWithBinary() {
  cl_int err;

  // Write binary
  char bin_filename[64];
  sprintf(bin_filename, ".spir%d.bc", mIdx);

  std::ofstream ofile(bin_filename, std::ios::out | std::ios::trunc
		      | std::ios::binary);

  ofile.write((const char *) mBinary, mBinaryLength);
  ofile.close();

  // Transform sources with ClTransform
  char trans_filename[64];
  sprintf(trans_filename, ".trans%d.bc", mIdx);
  {
    char command[1024];
    sprintf(command, "%s -load %s %s %s > %s 2> /dev/null",
	    OPTPATH, SPIRTRANSFORMPATH, SPIRPASS, bin_filename, trans_filename);

    system(command);
  }

  // Add merge kernels
  /* TODO */

  // Load transformed binary
  size_t transBinarySize;
  unsigned char *transBinary = hookcl_binaryload(trans_filename,
						 &transBinarySize);

  assert(mContext->getNbContexts() == mContext->getNbDevices());

  // Create split programs
  for (unsigned i=0; i<mNbPrograms; i++) {
    cl_context context = mContext->getContext(i);
    cl_device_id device = mContext->getDevice(i);
    mPrograms[i] = real_clCreateProgramWithBinary(context,
						  1,
						  &device,
						  &transBinarySize,
						  (const unsigned char **)
						  &transBinary,
						  NULL, &err);
    clCheck(err, __FILE__, __LINE__);
  }

  free(transBinary);
}

cl_program
ProgramHandle::getProgram(unsigned n) {
  return mPrograms[n];
}

ContextHandle *
ProgramHandle::getContext() {
  return mContext;
}

int
ProgramHandle::getId() {
  return mIdx;
}

cl_kernel
ProgramHandle::getMergeKernel(unsigned dev, unsigned nbsplits, unsigned nbArgs)
{
  return mMergeKernels[dev][(nbsplits-1)*MAXMERGEARGS + nbArgs-1];
}
