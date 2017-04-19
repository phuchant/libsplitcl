#ifndef OPENCLFUNCTIONS_H
#define OPENCLFUNCTIONS_H

#include <CL/cl.h>



extern "C" {

  /* Platform */

  extern cl_int (*real_clGetPlatformIDs) (cl_uint num_entries,
					  cl_platform_id *platforms,
					  cl_uint *num_platforms);


  /* Memory */

  extern cl_mem (*real_clCreateBuffer) (cl_context context,
					cl_mem_flags flags,
					size_t size,
					void *host_ptr,
					cl_int *errcode_ret);

  extern cl_int (*real_clReleaseMemObject) (cl_mem memobj);

  // Retain

  extern cl_int (*real_clEnqueueWriteBuffer)
  (cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
   size_t offset, size_t cb, const void *ptr, cl_uint num_events_in_wait_list,
   const cl_event *event_wait_list, cl_event *event);

  extern cl_int (*real_clEnqueueReadBuffer)
  (cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
   size_t offset, size_t cb, void *ptr, cl_uint num_events_in_wait_list,
   const cl_event *event_wait_list, cl_event *event);


  /* Context */

  extern cl_context (*real_clCreateContext)
  (const cl_context_properties *,
   cl_uint,
   const cl_device_id *,
   void  (*pfn_notify) (const char *, const void *, size_t, void *),
   void *,
   cl_int *);

  extern cl_int (*real_clReleaseContext) (cl_context);

  //RETAIN

  extern cl_int (*real_clGetContextInfo) (cl_context, cl_context_info, size_t,
					  void *, size_t *);

  /* Command Queue */

  extern cl_command_queue (*real_clCreateCommandQueue)
  (cl_context, cl_device_id, cl_command_queue_properties, cl_int *);

  extern cl_int (*real_clReleaseCommandQueue) (cl_command_queue);

  extern cl_int (*real_clRetainCommandQueue) (cl_command_queue command_queue);

  extern cl_int (*clSetCommandQueueProperty) (cl_command_queue command_queue,
					      cl_command_queue_properties properties,
					      cl_bool enable,
					      cl_command_queue_properties *old_properties);

  /* Kernel */
  extern cl_kernel (*real_clCreateKernel)
  (cl_program, const char *, cl_int *);

  extern cl_int (*real_clEnqueueNDRangeKernel)
  (cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
   const size_t *global_work_offset, const size_t *global_work_size,
   const size_t *local_work_size, cl_uint num_events_in_wait_list,
   const cl_event *event_wait_list, cl_event *event);

  extern cl_int (*real_clSetKernelArg)
  (cl_kernel, cl_uint, size_t, const void *);

  extern cl_int (*real_clReleaseKernel) (cl_kernel);

  extern cl_int (*real_clRetainKernel) (cl_kernel);

  extern cl_int (*real_clGetKernelWorkGroupInfo)
  (cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *,
   size_t *);

  extern cl_int (*real_clGetKernelInfo)
  (cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size,
   void *param_value, size_t *param_value_size_ret);

  /* Program */

  extern cl_int (*real_clBuildProgram)(cl_program, cl_uint,
				       const cl_device_id *,
				       const char *options,
				       void (*pfn_notify)(cl_program, void *),
				       void *);
  extern cl_int (*real_clRetainProgram) (cl_program);

  extern cl_int (*real_clReleaseProgram) (cl_program);

  extern cl_int (*real_clGetProgramInfo) (cl_program program,
					  cl_program_info param_name,
					  size_t param_value_size,
					  void *param_value,
					  size_t *param_value_size_ret);

  extern cl_int (*real_clGetProgramBuildInfo) (cl_program  program,
					       cl_device_id  device,
					       cl_program_build_info param_name,
					       size_t  param_value_size,
					       void  *param_value,
					       size_t  *param_value_size_ret);

  extern cl_program (*real_clCreateProgramWithSource) (cl_context context,
						       cl_uint count,
						       const char **strings,
						       const size_t *lengths,
						       cl_int *errcode_ret);

  extern cl_program (*real_clCreateProgramWithBinary)
  (cl_context context,
   cl_uint num_devices,
   const cl_device_id *device_list,
   const size_t *lengths,
   const unsigned char **binaries,
   cl_int *binary_status,
   cl_int *errcode_ret);

  /* Event */

  extern cl_event (*real_clCreateUserEvent) (cl_context context,
					     cl_int *errcode_ret);

  void getOpenCLFunctions();

}

#endif /* OPENCLFUNCTIONS_H */
