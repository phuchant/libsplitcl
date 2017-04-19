#include "KernelHandle.h"
#include "ProgramHandle.h"

#include <iostream>

// Create and return a KernelHandle object instead of a cl_kernel.
cl_kernel clCreateKernel(cl_program program, const char *kernel_name,
			 cl_int *errcode_ret)
{
  ProgramHandle *p = reinterpret_cast<ProgramHandle *>(program);
  KernelHandle *k = new KernelHandle(p, kernel_name);

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return (cl_kernel) k;
}

cl_int clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size,
		      const void *arg_value)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);

  k->setKernelArg(arg_index, arg_size, arg_value);

  return CL_SUCCESS;
}

cl_int clEnqueueNDRangeKernel(cl_command_queue command_queue,
			      cl_kernel kernel,
			      cl_uint work_dim,
			      const size_t *global_work_offset,
			      const size_t *global_work_size,
			      const size_t *local_work_size,
			      cl_uint num_events_in_wait_list,
			      const cl_event *event_wait_list,
			      cl_event *event)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);

  k->enqueueNDRange(command_queue, work_dim, global_work_offset,
		    global_work_size, local_work_size,
		    num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

cl_int clReleaseKernel(cl_kernel kernel)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);

  // // If ref counter = 0, we can delete the KernelHandle object.
  if (k->release())
    delete k;

  return CL_SUCCESS;
}

cl_int clRetainKernel(cl_kernel kernel)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);
  k->retain();

  return CL_SUCCESS;
}

cl_int clGetKernelWorkGroupInfo (cl_kernel  kernel ,
				 cl_device_id  device ,
				 cl_kernel_work_group_info  param_name ,
				 size_t  param_value_size ,
				 void  *param_value ,
				 size_t  *param_value_size_ret )
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);

  k->getKernelWorkgroupInfo(device, param_name, param_value_size, param_value,
			    param_value_size_ret);

  return CL_SUCCESS;
}

cl_int clGetKernelInfo (cl_kernel kernel,
			cl_kernel_info param_name,
			size_t param_value_size,
			void *param_value,
			size_t *param_value_size_ret) {
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);

  k->getKernelInfo(param_name, param_value_size, param_value,
		   param_value_size_ret);

  return CL_SUCCESS;
}

cl_int clCreateKernelsInProgram (cl_program  program,
				 cl_uint num_kernels,
				 cl_kernel *kernels,
				 cl_uint *num_kernels_ret)
{
  (void) program;
  (void) num_kernels;
  (void) kernels;
  (void) num_kernels_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueTask ( cl_command_queue command_queue,
		       cl_kernel kernel,
		       cl_uint num_events_in_wait_list,
		       const cl_event *event_wait_list,
		       cl_event *event)
{
  (void) command_queue;
  (void) kernel;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueNativeKernel ( cl_command_queue command_queue,
			       void (*user_func)(void *),
			       void *args,
			       size_t cb_args,
			       cl_uint num_mem_objects,
			       const cl_mem *mem_list,
			       const void **args_mem_loc,
			       cl_uint num_events_in_wait_list,
			       const cl_event *event_wait_list,
			       cl_event *event)
{
  (void) command_queue;
  (void) user_func;
  (void) args;
  (void) cb_args;
  (void) num_mem_objects;
  (void) mem_list;
  (void) args_mem_loc;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}
