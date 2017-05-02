#include <Handle/KernelHandle.h>

#include <iostream>

#include <CL/cl.h>

using namespace libsplit;

/* Kernel Object APIs */
cl_kernel
clCreateKernel(cl_program      program,
	       const char *    kernel_name,
	       cl_int *        errcode_ret)
{
  ProgramHandle *p = reinterpret_cast<ProgramHandle *>(program);
  KernelHandle *k = new KernelHandle(p, kernel_name);

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return (cl_kernel) k;
}

cl_int
clCreateKernelsInProgram(cl_program     /* program */,
			 cl_uint        /* num_kernels */,
			 cl_kernel *    /* kernels */,
			 cl_uint *      /* num_kernels_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int
clRetainKernel(cl_kernel    kernel)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);
  k->retain();

  return CL_SUCCESS;
}

cl_int
clReleaseKernel(cl_kernel   kernel)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);
  k->release();

  return CL_SUCCESS;
}

cl_int
clSetKernelArg(cl_kernel    kernel,
	       cl_uint      arg_index,
	       size_t       arg_size,
	       const void * arg_value)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);

  k->setKernelArg(arg_index, arg_size, arg_value);

  return CL_SUCCESS;
}

cl_int
clGetKernelInfo(cl_kernel       kernel,
		cl_kernel_info  param_name,
		size_t          param_value_size,
		void *          param_value,
		size_t *        param_value_size_ret)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);

  k->getKernelInfo(param_name, param_value_size, param_value,
		   param_value_size_ret);

  return CL_SUCCESS;
}

cl_int
clGetKernelArgInfo(cl_kernel       /* kernel */,
		   cl_uint         /* arg_indx */,
		   cl_kernel_arg_info  /* param_name */,
		   size_t          /* param_value_size */,
		   void *          /* param_value */,
		   size_t *        /* param_value_size_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int
clGetKernelWorkGroupInfo(cl_kernel                  kernel,
			 cl_device_id               device,
			 cl_kernel_work_group_info  param_name,
			 size_t                     param_value_size,
			 void *                     param_value,
			 size_t *                   param_value_size_ret)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);

  k->getKernelWorkgroupInfo(device, param_name, param_value_size, param_value,
			    param_value_size_ret);

  return CL_SUCCESS;
}
