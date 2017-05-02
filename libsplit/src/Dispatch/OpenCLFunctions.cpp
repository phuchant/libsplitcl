#include "OpenCLFunctions.h"

#include <iostream>
#include <dlfcn.h>
/* Platform API */
cl_int
(*real_clGetPlatformIDs)(cl_uint          /* num_entries */,
			 cl_platform_id * /* platforms */,
			 cl_uint *        /* num_platforms */) = NULL;

cl_int
(*real_clGetPlatformInfo)(cl_platform_id   /* platform */,
			  cl_platform_info /* param_name */,
			  size_t           /* param_value_size */,
			  void *           /* param_value */,
			  size_t *         /* param_value_size_ret */) = NULL;

/* Device APIs */
cl_int
(*real_clGetDeviceIDs)(cl_platform_id   /* platform */,
		       cl_device_type   /* device_type */,
		       cl_uint          /* num_entries */,
		       cl_device_id *   /* devices */,
		       cl_uint *        /* num_devices */) = NULL;

cl_int
(*real_clGetDeviceInfo)(cl_device_id    /* device */,
			cl_device_info  /* param_name */,
			size_t          /* param_value_size */,
			void *          /* param_value */,
			size_t *        /* param_value_size_ret */) = NULL;

cl_int
(*real_clCreateSubDevices)(cl_device_id                         /* in_device */,
			   const cl_device_partition_property * /* properties */,
			   cl_uint                              /* num_devices */,
			   cl_device_id *                       /* out_devices */,
			   cl_uint *                            /* num_devices_ret */) = NULL;

cl_int
(*real_clRetainDevice)(cl_device_id /* device */) = NULL;

cl_int
(*real_clReleaseDevice)(cl_device_id /* device */) = NULL;

/* Context APIs  */
cl_context
(*real_clCreateContext)(const cl_context_properties * /* properties */,
			cl_uint                 /* num_devices */,
			const cl_device_id *    /* devices */,
			void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
			void *                  /* user_data */,
			cl_int *                /* errcode_ret */) = NULL;

cl_context
(*real_clCreateContextFromType)(const cl_context_properties * /* properties */,
				cl_device_type          /* device_type */,
				void (CL_CALLBACK *     /* pfn_notify*/ )(const char *, const void *, size_t, void *),
				void *                  /* user_data */,
				cl_int *                /* errcode_ret */) = NULL;

cl_int
(*real_clRetainContext)(cl_context /* context */) = NULL;

cl_int
(*real_clReleaseContext)(cl_context /* context */) = NULL;

cl_int
(*real_clGetContextInfo)(cl_context         /* context */,
			 cl_context_info    /* param_name */,
			 size_t             /* param_value_size */,
			 void *             /* param_value */,
			 size_t *           /* param_value_size_ret */) = NULL;

/* Command Queue APIs */
cl_command_queue
(*real_clCreateCommandQueue)(cl_context                     /* context */,
			     cl_device_id                   /* device */,
			     cl_command_queue_properties    /* properties */,
			     cl_int *                       /* errcode_ret */) = NULL;

cl_int
(*real_clRetainCommandQueue)(cl_command_queue /* command_queue */) = NULL;

cl_int
(*real_clReleaseCommandQueue)(cl_command_queue /* command_queue */) = NULL;

cl_int
(*real_clGetCommandQueueInfo)(cl_command_queue      /* command_queue */,
			      cl_command_queue_info /* param_name */,
			      size_t                /* param_value_size */,
			      void *                /* param_value */,
			      size_t *              /* param_value_size_ret */) = NULL;

/* Memory Object APIs */
cl_mem
(*real_clCreateBuffer)(cl_context   /* context */,
		       cl_mem_flags /* flags */,
		       size_t       /* size */,
		       void *       /* host_ptr */,
		       cl_int *     /* errcode_ret */) = NULL;

cl_int
(*real_clRetainMemObject)(cl_mem /* memobj */) = NULL;

cl_int
(*real_clReleaseMemObject)(cl_mem /* memobj */) = NULL;

cl_int
(*real_clGetMemObjectInfo) (cl_mem           /* memobj */,
			    cl_mem_info      /* param_name */,
			    size_t           /* param_value_size */,
			    void *           /* param_value */,
			    size_t *         /* param_value_size_ret */) = NULL;


/* Program Object APIs  */
cl_program
(*real_clCreateProgramWithSource)(cl_context        /* context */,
				  cl_uint           /* count */,
				  const char **     /* strings */,
				  const size_t *    /* lengths */,
				  cl_int *          /* errcode_ret */) = NULL;

cl_program
(*real_clCreateProgramWithBinary)(cl_context                     /* context */,
				  cl_uint                        /* num_devices */,
				  const cl_device_id *           /* device_list */,
				  const size_t *                 /* lengths */,
				  const unsigned char **         /* binaries */,
				  cl_int *                       /* binary_status */,
				  cl_int *                       /* errcode_ret */) = NULL;

cl_int
(*real_clRetainProgram)(cl_program /* program */) = NULL;

cl_int
(*real_clReleaseProgram)(cl_program /* program */) = NULL;

cl_int
(*real_clBuildProgram)(cl_program           /* program */,
		       cl_uint              /* num_devices */,
		       const cl_device_id * /* device_list */,
		       const char *         /* options */,
		       void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
		       void *               /* user_data */) = NULL;


cl_int
(*real_clGetProgramInfo)(cl_program         /* program */,
			 cl_program_info    /* param_name */,
			 size_t             /* param_value_size */,
			 void *             /* param_value */,
			 size_t *           /* param_value_size_ret */) = NULL;

cl_int
(*real_clGetProgramBuildInfo)(cl_program            /* program */,
			      cl_device_id          /* device */,
			      cl_program_build_info /* param_name */,
			      size_t                /* param_value_size */,
			      void *                /* param_value */,
			      size_t *              /* param_value_size_ret */) = NULL;

/* Kernel Object APIs */
cl_kernel
(*real_clCreateKernel)(cl_program      /* program */,
		       const char *    /* kernel_name */,
		       cl_int *        /* errcode_ret */) = NULL;

cl_int
(*real_clRetainKernel)(cl_kernel    /* kernel */) = NULL;

cl_int
(*real_clReleaseKernel)(cl_kernel   /* kernel */) = NULL;

cl_int
(*real_clSetKernelArg)(cl_kernel    /* kernel */,
		       cl_uint      /* arg_index */,
		       size_t       /* arg_size */,
		       const void * /* arg_value */) = NULL;

cl_int
(*real_clGetKernelInfo)(cl_kernel       /* kernel */,
			cl_kernel_info  /* param_name */,
			size_t          /* param_value_size */,
			void *          /* param_value */,
			size_t *        /* param_value_size_ret */) = NULL;

cl_int
(*real_clGetKernelArgInfo)(cl_kernel       /* kernel */,
			   cl_uint         /* arg_indx */,
			   cl_kernel_arg_info  /* param_name */,
			   size_t          /* param_value_size */,
			   void *          /* param_value */,
			   size_t *        /* param_value_size_ret */) = NULL;

cl_int
(*real_clGetKernelWorkGroupInfo)(cl_kernel                  /* kernel */,
				 cl_device_id               /* device */,
				 cl_kernel_work_group_info  /* param_name */,
				 size_t                     /* param_value_size */,
				 void *                     /* param_value */,
				 size_t *                   /* param_value_size_ret */) = NULL;

/* Event Object APIs */
cl_int
(*real_clWaitForEvents)(cl_uint             /* num_events */,
			const cl_event *    /* event_list */) = NULL;

cl_int
(*real_clGetEventInfo)(cl_event         /* event */,
		       cl_event_info    /* param_name */,
		       size_t           /* param_value_size */,
		       void *           /* param_value */,
		       size_t *         /* param_value_size_ret */) = NULL;

cl_event
(*real_clCreateUserEvent)(cl_context    /* context */,
			  cl_int *      /* errcode_ret */) = NULL;

cl_int
(*real_clRetainEvent)(cl_event /* event */) = NULL;

cl_int
(*real_clReleaseEvent)(cl_event /* event */) = NULL;

cl_int
(*real_clSetUserEventStatus)(cl_event   /* event */,
			     cl_int     /* execution_status */) = NULL;

cl_int
(*real_clSetEventCallback)( cl_event    /* event */,
			    cl_int      /* command_exec_callback_type */,
			    void (CL_CALLBACK * /* pfn_notify */)(cl_event, cl_int, void *),
			    void *      /* user_data */) = NULL;

/* Profiling APIs */
cl_int
(*real_clGetEventProfilingInfo)(cl_event            /* event */,
				cl_profiling_info   /* param_name */,
				size_t              /* param_value_size */,
				void *              /* param_value */,
				size_t *            /* param_value_size_ret */) = NULL;

/* Flush and Finish APIs */
cl_int
(*real_clFlush)(cl_command_queue /* command_queue */) = NULL;

cl_int
(*real_clFinish)(cl_command_queue /* command_queue */) = NULL;

/* Enqueued Commands APIs */
cl_int
(*real_clEnqueueReadBuffer)(cl_command_queue    /* command_queue */,
			    cl_mem              /* buffer */,
			    cl_bool             /* blocking_read */,
			    size_t              /* offset */,
			    size_t              /* size */,
			    void *              /* ptr */,
			    cl_uint             /* num_events_in_wait_list */,
			    const cl_event *    /* event_wait_list */,
			    cl_event *          /* event */) = NULL;

cl_int
(*real_clEnqueueWriteBuffer)(cl_command_queue   /* command_queue */,
			     cl_mem             /* buffer */,
			     cl_bool            /* blocking_write */,
			     size_t             /* offset */,
			     size_t             /* size */,
			     const void *       /* ptr */,
			     cl_uint            /* num_events_in_wait_list */,
			     const cl_event *   /* event_wait_list */,
			     cl_event *         /* event */) = NULL;

void *
(*real_clEnqueueMapBuffer)(cl_command_queue /* command_queue */,
			   cl_mem           /* buffer */,
			   cl_bool          /* blocking_map */,
			   cl_map_flags     /* map_flags */,
			   size_t           /* offset */,
			   size_t           /* size */,
			   cl_uint          /* num_events_in_wait_list */,
			   const cl_event * /* event_wait_list */,
			   cl_event *       /* event */,
			   cl_int *         /* errcode_ret */) = NULL;

cl_int
(*real_clEnqueueUnmapMemObject)(cl_command_queue /* command_queue */,
				cl_mem           /* memobj */,
				void *           /* mapped_ptr */,
				cl_uint          /* num_events_in_wait_list */,
				const cl_event *  /* event_wait_list */,
				cl_event *        /* event */) = NULL;

cl_int
(*real_clEnqueueNDRangeKernel)(cl_command_queue /* command_queue */,
			       cl_kernel        /* kernel */,
			       cl_uint          /* work_dim */,
			       const size_t *   /* global_work_offset */,
			       const size_t *   /* global_work_size */,
			       const size_t *   /* local_work_size */,
			       cl_uint          /* num_events_in_wait_list */,
			       const cl_event * /* event_wait_list */,
			       cl_event *       /* event */) = NULL;

void getOpenCLFunctions()
{
  /* Platform API */
  if (!real_clGetPlatformIDs)
    *(void **) &real_clGetPlatformIDs = dlsym(RTLD_NEXT, "clGetPlatformIDs");

  if (!real_clGetPlatformInfo)
    *(void **) &real_clGetPlatformInfo = dlsym(RTLD_NEXT, "clGetPlatformInfo");

  /* Device APIs */
  if (!real_clGetDeviceIDs)
    *(void **) &real_clGetDeviceIDs = dlsym(RTLD_NEXT, "clGetDeviceIDs");

  if (!real_clGetDeviceInfo)
    *(void **) &real_clGetDeviceInfo = dlsym(RTLD_NEXT, "clGetDeviceInfo");

  if (!real_clCreateSubDevices)
    *(void **) &real_clCreateSubDevices = dlsym(RTLD_NEXT, "clCreateSubDevices");

  if (!real_clRetainDevice)
    *(void **) &real_clRetainDevice = dlsym(RTLD_NEXT, "clRetainDevice");

  if (!real_clReleaseDevice)
    *(void **) &real_clReleaseDevice = dlsym(RTLD_NEXT, "clReleaseDevice");

  /* Context APIs  */
  if (!real_clCreateContext)
    *(void **) &real_clCreateContext = dlsym(RTLD_NEXT, "clCreateContext");

  if (!real_clCreateContextFromType)
    *(void **) &real_clCreateContextFromType = dlsym(RTLD_NEXT, "clCreateContextFromType");

  if (!real_clRetainContext)
    *(void **) &real_clRetainContext = dlsym(RTLD_NEXT, "clRetainContext");

  if (!real_clReleaseContext)
    *(void **) &real_clReleaseContext = dlsym(RTLD_NEXT, "clReleaseContext");

  if (!real_clGetContextInfo)
    *(void **) &real_clGetContextInfo = dlsym(RTLD_NEXT, "clGetContextInfo");

  /* Command Queue APIs */
  if (!real_clCreateCommandQueue)
    *(void **) &real_clCreateCommandQueue = dlsym(RTLD_NEXT, "clCreateCommandQueue");

  if (!real_clRetainCommandQueue)
    *(void **) &real_clRetainCommandQueue = dlsym(RTLD_NEXT, "clRetainCommandQueue");

  if (!real_clReleaseCommandQueue)
    *(void **) &real_clReleaseCommandQueue = dlsym(RTLD_NEXT, "clReleaseCommandQueue");

  if (!real_clGetCommandQueueInfo)
    *(void **) &real_clGetCommandQueueInfo = dlsym(RTLD_NEXT, "clGetCommandQueueInfo");

  /* Memory Object APIs */
  if (!real_clCreateBuffer)
    *(void **) &real_clCreateBuffer = dlsym(RTLD_NEXT, "clCreateBuffer");

  if (!real_clRetainMemObject)
    *(void **) &real_clRetainMemObject = dlsym(RTLD_NEXT, "clRetainMemObject");

  if (!real_clReleaseMemObject)
    *(void **) &real_clReleaseMemObject = dlsym(RTLD_NEXT, "clReleaseMemObject");

  if (!real_clGetMemObjectInfo)
    *(void **) &real_clGetMemObjectInfo = dlsym(RTLD_NEXT, "clGetMemObjectInfo");

  /* Program Object APIs  */
  if (!real_clCreateProgramWithSource)
    *(void **) &real_clCreateProgramWithSource = dlsym(RTLD_NEXT, "clCreateProgramWithSource");

  if (!real_clCreateProgramWithBinary)
    *(void **) &real_clCreateProgramWithBinary = dlsym(RTLD_NEXT, "clCreateProgramWithBinary");

  if (!real_clRetainProgram)
    *(void **) &real_clRetainProgram = dlsym(RTLD_NEXT, "clRetainProgram");

  if (!real_clReleaseProgram)
    *(void **) &real_clReleaseProgram = dlsym(RTLD_NEXT, "clReleaseProgram");

  if (!real_clBuildProgram)
    *(void **) &real_clBuildProgram = dlsym(RTLD_NEXT, "clBuildProgram");


  if (!real_clGetProgramInfo)
    *(void **) &real_clGetProgramInfo = dlsym(RTLD_NEXT, "clGetProgramInfo");

  if (!real_clGetProgramBuildInfo)
    *(void **) &real_clGetProgramBuildInfo = dlsym(RTLD_NEXT, "clGetProgramBuildInfo");

  /* Kernel Object APIs */
  if (!real_clCreateKernel)
    *(void **) &real_clCreateKernel = dlsym(RTLD_NEXT, "clCreateKernel");

  if (!real_clRetainKernel)
    *(void **) &real_clRetainKernel = dlsym(RTLD_NEXT, "clRetainKernel");

  if (!real_clReleaseKernel)
    *(void **) &real_clReleaseKernel = dlsym(RTLD_NEXT, "clReleaseKernel");

  if (!real_clSetKernelArg)
    *(void **) &real_clSetKernelArg = dlsym(RTLD_NEXT, "clSetKernelArg");

  if (!real_clGetKernelInfo)
    *(void **) &real_clGetKernelInfo = dlsym(RTLD_NEXT, "clGetKernelInfo");

  if (!real_clGetKernelArgInfo)
    *(void **) &real_clGetKernelArgInfo = dlsym(RTLD_NEXT, "clGetKernelArgInfo");

  if (!real_clGetKernelWorkGroupInfo)
    *(void **) &real_clGetKernelWorkGroupInfo = dlsym(RTLD_NEXT, "clGetKernelWorkGroupInfo");

  /* Event Object APIs */
  if (!real_clWaitForEvents)
    *(void **) &real_clWaitForEvents = dlsym(RTLD_NEXT, "clWaitForEvents");

  if (!real_clGetEventInfo)
    *(void **) &real_clGetEventInfo = dlsym(RTLD_NEXT, "clGetEventInfo");

  if (!real_clCreateUserEvent)
    *(void **) &real_clCreateUserEvent = dlsym(RTLD_NEXT, "clCreateUserEvent");

  if (!real_clRetainEvent)
    *(void **) &real_clRetainEvent = dlsym(RTLD_NEXT, "clRetainEvent");

  if (!real_clReleaseEvent)
    *(void **) &real_clReleaseEvent = dlsym(RTLD_NEXT, "clReleaseEvent");

  if (!real_clSetUserEventStatus)
    *(void **) &real_clSetUserEventStatus = dlsym(RTLD_NEXT, "clSetUserEventStatus");

  if (!real_clSetEventCallback)
    *(void **) &real_clSetEventCallback = dlsym(RTLD_NEXT, "clSetEventCallback");

  /* Profiling APIs */
  if (!real_clGetEventProfilingInfo)
    *(void **) &real_clGetEventProfilingInfo = dlsym(RTLD_NEXT, "clGetEventProfilingInfo");

  /* Flush and Finish APIs */
  if (!real_clFlush)
    *(void **) &real_clFlush = dlsym(RTLD_NEXT, "clFlush");

  if (!real_clFinish)
    *(void **) &real_clFinish = dlsym(RTLD_NEXT, "clFinish");

  /* Enqueued Commands APIs */
  if (!real_clEnqueueReadBuffer)
    *(void **) &real_clEnqueueReadBuffer = dlsym(RTLD_NEXT, "clEnqueueReadBuffer");

  if (!real_clEnqueueWriteBuffer)
    *(void **) &real_clEnqueueWriteBuffer = dlsym(RTLD_NEXT, "clEnqueueWriteBuffer");

  if (!real_clEnqueueMapBuffer)
    *(void **) &real_clEnqueueMapBuffer = dlsym(RTLD_NEXT, "clEnqueueMapBuffer");

  if (!real_clEnqueueUnmapMemObject)
    *(void **) &real_clEnqueueUnmapMemObject = dlsym(RTLD_NEXT, "clEnqueueUnmapMemObject");

  if (!real_clEnqueueNDRangeKernel)
    *(void **) &real_clEnqueueNDRangeKernel = dlsym(RTLD_NEXT, "clEnqueueNDRangeKernel");
}

