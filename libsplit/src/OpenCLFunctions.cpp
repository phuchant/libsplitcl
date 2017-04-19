#include "OpenCLFunctions.h"

#include <iostream>
#include <dlfcn.h>

/* Platform */
cl_int (*real_clGetPlatformIDs) (cl_uint num_entries,
				 cl_platform_id *platforms,
				 cl_uint *num_platforms) = NULL;

/* Memory */

cl_mem (*real_clCreateBuffer) (cl_context context,
			       cl_mem_flags flags,
			       size_t size,
			       void *host_ptr,
			       cl_int *errcode_ret) = NULL;
cl_int (*real_clReleaseMemObject) (cl_mem memobj) = NULL;

// Retain

cl_int (*real_clEnqueueWriteBuffer)
(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
 size_t offset, size_t cb, const void *ptr, cl_uint num_events_in_wait_list,
 const cl_event *event_wait_list, cl_event *event) = NULL;

cl_int (*real_clEnqueueReadBuffer)
(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
 size_t offset, size_t cb, void *ptr, cl_uint num_events_in_wait_list,
 const cl_event *event_wait_list, cl_event *event) = NULL;


/* Context */

cl_context (*real_clCreateContext)
(const cl_context_properties *,
 cl_uint,
 const cl_device_id *,
 void  (*pfn_notify) (const char *, const void *, size_t, void *),
 void *,
 cl_int *) = NULL;

cl_int (*real_clReleaseContext) (cl_context) = NULL;

//RETAIN

cl_int (*real_clGetContextInfo) (cl_context, cl_context_info, size_t,
				 void *, size_t *) = NULL;

/* Command Queue */

cl_command_queue (*real_clCreateCommandQueue)
(cl_context, cl_device_id, cl_command_queue_properties, cl_int *) = NULL;

cl_int (*real_clReleaseCommandQueue) (cl_command_queue) = NULL;

cl_int (*real_clRetainCommandQueue) (cl_command_queue command_queue) = NULL;

cl_int (*clSetCommandQueueProperty) (cl_command_queue command_queue,
				     cl_command_queue_properties properties,
				     cl_bool enable,
				     cl_command_queue_properties *old_properties) = NULL;

/* Kernel */
cl_kernel (*real_clCreateKernel)
(cl_program, const char *, cl_int *) = NULL;

cl_int (*real_clEnqueueNDRangeKernel)
(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
 const size_t *global_work_offset, const size_t *global_work_size,
 const size_t *local_work_size, cl_uint num_events_in_wait_list,
 const cl_event *event_wait_list, cl_event *event) = NULL;

cl_int (*real_clSetKernelArg)
(cl_kernel, cl_uint, size_t, const void *) = NULL;

cl_int (*real_clReleaseKernel) (cl_kernel) = NULL;

cl_int (*real_clRetainKernel) (cl_kernel) = NULL;

cl_int (*real_clGetKernelWorkGroupInfo)
(cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *,
 size_t *) = NULL;

cl_int (*real_clGetKernelInfo)
(cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size,
 void *param_value, size_t *param_value_size_ret) = NULL;

/* Program */

cl_int (*real_clBuildProgram)(cl_program, cl_uint,
			      const cl_device_id *,
			      const char *options,
			      void (*pfn_notify)(cl_program, void *),
			      void *) = NULL;
cl_int (*real_clRetainProgram) (cl_program) = NULL;

cl_int (*real_clReleaseProgram) (cl_program) = NULL;

cl_int (*real_clGetProgramInfo) (cl_program program,
				 cl_program_info param_name,
				 size_t param_value_size,
				 void *param_value,
				 size_t *param_value_size_ret) = NULL;

cl_int (*real_clGetProgramBuildInfo) (cl_program  program,
				      cl_device_id  device,
				      cl_program_build_info param_name,
				      size_t  param_value_size,
				      void  *param_value,
				      size_t  *param_value_size_ret) = NULL;

cl_program (*real_clCreateProgramWithSource) (cl_context context,
					      cl_uint count,
					      const char **strings,
					      const size_t *lengths,
					      cl_int *errcode_ret) = NULL;

cl_program (*real_clCreateProgramWithBinary)
(cl_context context,
 cl_uint num_devices,
 const cl_device_id *device_list,
 const size_t *lengths,
 const unsigned char **binaries,
 cl_int *binary_status,
 cl_int *errcode_ret) = NULL;

/* Event */

cl_event (*real_clCreateUserEvent) (cl_context context,
				    cl_int *errcode_ret) = NULL;


void getOpenCLFunctions()
{
  /* Platform */

  if (!real_clGetPlatformIDs)
    *(void **) &real_clGetPlatformIDs = dlsym(RTLD_NEXT, "clGetPlatformIDs");

  /* Memory */

  if (!real_clCreateBuffer)
    *(void **) &real_clCreateBuffer =
      dlsym(RTLD_NEXT, "clCreateBuffer");

  if (!real_clReleaseMemObject)
    *(void **) &real_clReleaseMemObject =
      dlsym(RTLD_NEXT, "clReleaseMemObject");

  if (!real_clEnqueueWriteBuffer)
    *(void **) &real_clEnqueueWriteBuffer =
      dlsym(RTLD_NEXT, "clEnqueueWriteBuffer");

  if (!real_clEnqueueReadBuffer)
    *(void **) &real_clEnqueueReadBuffer =
      dlsym(RTLD_NEXT, "clEnqueueReadBuffer");

  /* Context */

  if (!real_clCreateContext)
    *(void **) &real_clCreateContext =
      dlsym(RTLD_NEXT, "clCreateContext");

  if (!real_clReleaseContext)
    *(void **) &real_clReleaseContext =
      dlsym(RTLD_NEXT, "clReleaseContext");

  if (!real_clGetContextInfo)
    *(void **) &real_clGetContextInfo =
      dlsym(RTLD_NEXT, "clGetContextInfo");

  /* Command Queue */

  if (!real_clCreateCommandQueue)
    *(void **) &real_clCreateCommandQueue =
      dlsym(RTLD_NEXT, "clCreateCommandQueue");

  if (!real_clReleaseCommandQueue)
    *(void **) &real_clReleaseCommandQueue =
      dlsym(RTLD_NEXT, "clReleaseCommandQueue");

  if (!real_clRetainCommandQueue)
    *(void **) &real_clRetainCommandQueue =
      dlsym(RTLD_NEXT, "clRetainCommandQueue");

  /* Kernel */

  if (!real_clCreateKernel)
    *(void **) &real_clCreateKernel =
      dlsym(RTLD_NEXT, "clCreateKernel");

  if (!real_clEnqueueNDRangeKernel)
    *(void **) &real_clEnqueueNDRangeKernel =
      dlsym(RTLD_NEXT, "clEnqueueNDRangeKernel");

  if (!real_clSetKernelArg)
    *(void **) &real_clSetKernelArg =
      dlsym(RTLD_NEXT, "clSetKernelArg");

  if (!real_clReleaseKernel)
    *(void **) &real_clReleaseKernel =
      dlsym(RTLD_NEXT, "clReleaseKernel");

  if (!real_clRetainKernel)
    *(void **) &real_clRetainKernel =
      dlsym(RTLD_NEXT, "clRetainKernel");

  if (!real_clGetKernelWorkGroupInfo)
    *(void **) &real_clGetKernelWorkGroupInfo =
      dlsym(RTLD_NEXT, "clGetKernelWorkGroupInfo");

  if (!real_clGetKernelInfo)
    *(void **) &real_clGetKernelInfo =
      dlsym(RTLD_NEXT, "clGetKernelInfo");

  /* Program */

  if (!real_clBuildProgram)
    *(void **) &real_clBuildProgram =
      dlsym(RTLD_NEXT, "clBuildProgram");

  if (!real_clRetainProgram)
    *(void **) &real_clRetainProgram =
      dlsym(RTLD_NEXT, "clRetainProgram");

  if (!real_clReleaseProgram)
    *(void **) &real_clReleaseProgram =
      dlsym(RTLD_NEXT, "clReleaseProgram");

  if (!real_clGetProgramInfo)
    *(void **) &real_clGetProgramInfo =
      dlsym(RTLD_NEXT, "clGetProgramInfo");

  if (!real_clGetProgramBuildInfo)
    *(void **) &real_clGetProgramBuildInfo =
      dlsym(RTLD_NEXT, "clGetProgramBuildInfo");

  if (!real_clCreateProgramWithSource)
    *(void **) &real_clCreateProgramWithSource =
      dlsym(RTLD_NEXT, "clCreateProgramWithSource");

  if (!real_clCreateProgramWithBinary)
    *(void **) &real_clCreateProgramWithBinary =
      dlsym(RTLD_NEXT, "clCreateProgramWithBinary");

  /* Event */
  if (!real_clCreateUserEvent)
    *(void **) &real_clCreateUserEvent =
      dlsym(RTLD_NEXT, "clCreateUserEvent");
}
