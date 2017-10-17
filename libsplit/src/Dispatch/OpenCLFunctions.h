#ifndef OPENCLFUNCTIONS_H
#define OPENCLFUNCTIONS_H

#include <CL/cl.h>

extern "C" {
  /* Platform API */
  extern  cl_int
  (*real_clGetPlatformIDs)(cl_uint          /* num_entries */,
			   cl_platform_id * /* platforms */,
			   cl_uint *        /* num_platforms */) ;

  extern  cl_int
  (*real_clGetPlatformInfo)(cl_platform_id   /* platform */,
			    cl_platform_info /* param_name */,
			    size_t           /* param_value_size */,
			    void *           /* param_value */,
			    size_t *         /* param_value_size_ret */) ;

  /* Device APIs */
  extern  cl_int
  (*real_clGetDeviceIDs)(cl_platform_id   /* platform */,
			 cl_device_type   /* device_type */,
			 cl_uint          /* num_entries */,
			 cl_device_id *   /* devices */,
			 cl_uint *        /* num_devices */) ;

  extern  cl_int
  (*real_clGetDeviceInfo)(cl_device_id    /* device */,
			  cl_device_info  /* param_name */,
			  size_t          /* param_value_size */,
			  void *          /* param_value */,
			  size_t *        /* param_value_size_ret */) ;

  extern  cl_int
  (*real_clCreateSubDevices)(cl_device_id                         /* in_device */,
			     const cl_device_partition_property * /* properties */,
			     cl_uint                              /* num_devices */,
			     cl_device_id *                       /* out_devices */,
			     cl_uint *                            /* num_devices_ret */) ;

  extern  cl_int
  (*real_clRetainDevice)(cl_device_id /* device */) ;

  extern  cl_int
  (*real_clReleaseDevice)(cl_device_id /* device */) ;

  /* Context APIs  */
  extern  cl_context
  (*real_clCreateContext)(const cl_context_properties * /* properties */,
			  cl_uint                 /* num_devices */,
			  const cl_device_id *    /* devices */,
			  void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
			  void *                  /* user_data */,
			  cl_int *                /* errcode_ret */) ;

  extern  cl_context
  (*real_clCreateContextFromType)(const cl_context_properties * /* properties */,
				  cl_device_type          /* device_type */,
				  void (CL_CALLBACK *     /* pfn_notify*/ )(const char *, const void *, size_t, void *),
				  void *                  /* user_data */,
				  cl_int *                /* errcode_ret */) ;

  extern  cl_int
  (*real_clRetainContext)(cl_context /* context */) ;

  extern  cl_int
  (*real_clReleaseContext)(cl_context /* context */) ;

  extern  cl_int
  (*real_clGetContextInfo)(cl_context         /* context */,
			   cl_context_info    /* param_name */,
			   size_t             /* param_value_size */,
			   void *             /* param_value */,
			   size_t *           /* param_value_size_ret */) ;

  /* Command Queue APIs */
  extern  cl_command_queue
  (*real_clCreateCommandQueue)(cl_context                     /* context */,
			       cl_device_id                   /* device */,
			       cl_command_queue_properties    /* properties */,
			       cl_int *                       /* errcode_ret */) ;

  extern  cl_int
  (*real_clRetainCommandQueue)(cl_command_queue /* command_queue */) ;

  extern  cl_int
  (*real_clReleaseCommandQueue)(cl_command_queue /* command_queue */) ;

  extern  cl_int
  (*real_clGetCommandQueueInfo)(cl_command_queue      /* command_queue */,
				cl_command_queue_info /* param_name */,
				size_t                /* param_value_size */,
				void *                /* param_value */,
				size_t *              /* param_value_size_ret */) ;

  /* Memory Object APIs */
  extern  cl_mem
  (*real_clCreateBuffer)(cl_context   /* context */,
			 cl_mem_flags /* flags */,
			 size_t       /* size */,
			 void *       /* host_ptr */,
			 cl_int *     /* errcode_ret */) ;

  extern  cl_int
  (*real_clRetainMemObject)(cl_mem /* memobj */) ;

  extern  cl_int
  (*real_clReleaseMemObject)(cl_mem /* memobj */) ;

  extern  cl_int
  clGetMemObjectInfo(cl_mem           /* memobj */,
		     cl_mem_info      /* param_name */,
		     size_t           /* param_value_size */,
		     void *           /* param_value */,
		     size_t *         /* param_value_size_ret */) ;


  /* Program Object APIs  */
  extern  cl_program
  (*real_clCreateProgramWithSource)(cl_context        /* context */,
				    cl_uint           /* count */,
				    const char **     /* strings */,
				    const size_t *    /* lengths */,
				    cl_int *          /* errcode_ret */) ;

  extern  cl_program
  (*real_clCreateProgramWithBinary)(cl_context                     /* context */,
				    cl_uint                        /* num_devices */,
				    const cl_device_id *           /* device_list */,
				    const size_t *                 /* lengths */,
				    const unsigned char **         /* binaries */,
				    cl_int *                       /* binary_status */,
				    cl_int *                       /* errcode_ret */) ;

  extern  cl_int
  (*real_clRetainProgram)(cl_program /* program */) ;

  extern  cl_int
  (*real_clReleaseProgram)(cl_program /* program */) ;

  extern  cl_int
  (*real_clBuildProgram)(cl_program           /* program */,
			 cl_uint              /* num_devices */,
			 const cl_device_id * /* device_list */,
			 const char *         /* options */,
			 void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
			 void *               /* user_data */) ;


  extern  cl_int
  (*real_clGetProgramInfo)(cl_program         /* program */,
			   cl_program_info    /* param_name */,
			   size_t             /* param_value_size */,
			   void *             /* param_value */,
			   size_t *           /* param_value_size_ret */) ;

  extern  cl_int
  (*real_clGetProgramBuildInfo)(cl_program            /* program */,
				cl_device_id          /* device */,
				cl_program_build_info /* param_name */,
				size_t                /* param_value_size */,
				void *                /* param_value */,
				size_t *              /* param_value_size_ret */) ;

  /* Kernel Object APIs */
  extern  cl_kernel
  (*real_clCreateKernel)(cl_program      /* program */,
			 const char *    /* kernel_name */,
			 cl_int *        /* errcode_ret */) ;

  extern  cl_int
  (*real_clRetainKernel)(cl_kernel    /* kernel */) ;

  extern  cl_int
  (*real_clReleaseKernel)(cl_kernel   /* kernel */) ;

  extern  cl_int
  (*real_clSetKernelArg)(cl_kernel    /* kernel */,
			 cl_uint      /* arg_index */,
			 size_t       /* arg_size */,
			 const void * /* arg_value */) ;

  extern  cl_int
  (*real_clGetKernelInfo)(cl_kernel       /* kernel */,
			  cl_kernel_info  /* param_name */,
			  size_t          /* param_value_size */,
			  void *          /* param_value */,
			  size_t *        /* param_value_size_ret */) ;

  extern  cl_int
  (*real_clGetKernelArgInfo)(cl_kernel       /* kernel */,
			     cl_uint         /* arg_indx */,
			     cl_kernel_arg_info  /* param_name */,
			     size_t          /* param_value_size */,
			     void *          /* param_value */,
			     size_t *        /* param_value_size_ret */) ;

  extern  cl_int
  (*real_clGetKernelWorkGroupInfo)(cl_kernel                  /* kernel */,
				   cl_device_id               /* device */,
				   cl_kernel_work_group_info  /* param_name */,
				   size_t                     /* param_value_size */,
				   void *                     /* param_value */,
				   size_t *                   /* param_value_size_ret */) ;

  /* Event Object APIs */
  extern  cl_int
  (*real_clWaitForEvents)(cl_uint             /* num_events */,
			  const cl_event *    /* event_list */) ;

  extern  cl_int
  (*real_clGetEventInfo)(cl_event         /* event */,
			 cl_event_info    /* param_name */,
			 size_t           /* param_value_size */,
			 void *           /* param_value */,
			 size_t *         /* param_value_size_ret */) ;

  extern  cl_event
  (*real_clCreateUserEvent)(cl_context    /* context */,
			    cl_int *      /* errcode_ret */) CL_API_SUFFIX__VERSION_1_1;

  extern  cl_int
  (*real_clRetainEvent)(cl_event /* event */) ;

  extern  cl_int
  (*real_clReleaseEvent)(cl_event /* event */) ;

  extern  cl_int
  (*real_clSetUserEventStatus)(cl_event   /* event */,
			       cl_int     /* execution_status */) CL_API_SUFFIX__VERSION_1_1;

  extern  cl_int
  (*real_clSetEventCallback)( cl_event    /* event */,
			      cl_int      /* command_exec_callback_type */,
			      void (CL_CALLBACK * /* pfn_notify */)(cl_event, cl_int, void *),
			      void *      /* user_data */) CL_API_SUFFIX__VERSION_1_1;

  /* Profiling APIs */
  extern  cl_int
  (*real_clGetEventProfilingInfo)(cl_event            /* event */,
				  cl_profiling_info   /* param_name */,
				  size_t              /* param_value_size */,
				  void *              /* param_value */,
				  size_t *            /* param_value_size_ret */) ;

  /* Flush and Finish APIs */
  extern  cl_int
  (*real_clFlush)(cl_command_queue /* command_queue */) ;

  extern  cl_int
  (*real_clFinish)(cl_command_queue /* command_queue */) ;

  /* Enqueued Commands APIs */
  extern  cl_int
  (*real_clEnqueueReadBuffer)(cl_command_queue    /* command_queue */,
			      cl_mem              /* buffer */,
			      cl_bool             /* blocking_read */,
			      size_t              /* offset */,
			      size_t              /* size */,
			      void *              /* ptr */,
			      cl_uint             /* num_events_in_wait_list */,
			      const cl_event *    /* event_wait_list */,
			      cl_event *          /* event */) ;

  extern  cl_int
  (*real_clEnqueueWriteBuffer)(cl_command_queue   /* command_queue */,
			       cl_mem             /* buffer */,
			       cl_bool            /* blocking_write */,
			       size_t             /* offset */,
			       size_t             /* size */,
			       const void *       /* ptr */,
			       cl_uint            /* num_events_in_wait_list */,
			       const cl_event *   /* event_wait_list */,
			       cl_event *         /* event */) ;

  extern  void *
  (*real_clEnqueueMapBuffer)(cl_command_queue /* command_queue */,
			     cl_mem           /* buffer */,
			     cl_bool          /* blocking_map */,
			     cl_map_flags     /* map_flags */,
			     size_t           /* offset */,
			     size_t           /* size */,
			     cl_uint          /* num_events_in_wait_list */,
			     const cl_event * /* event_wait_list */,
			     cl_event *       /* event */,
			     cl_int *         /* errcode_ret */) ;

  extern  cl_int
  (*real_clEnqueueUnmapMemObject)(cl_command_queue /* command_queue */,
				  cl_mem           /* memobj */,
				  void *           /* mapped_ptr */,
				  cl_uint          /* num_events_in_wait_list */,
				  const cl_event *  /* event_wait_list */,
				  cl_event *        /* event */) ;

  extern  cl_int
  (*real_clEnqueueFillBuffer)(cl_command_queue /* command_queue */,
			      cl_mem           /* buffer */,
			      const void *     /* pattern */,
			      size_t           /* pattern_size */,
			      size_t           /* offset */,
			      size_t           /* size */,
			      cl_uint          /* num_events_in_wait_list */,
			      const cl_event * /* event_wait_list */,
			      cl_event *       /* event */) ;

  extern  cl_int
  (*real_clEnqueueNDRangeKernel)(cl_command_queue /* command_queue */,
				 cl_kernel        /* kernel */,
				 cl_uint          /* work_dim */,
				 const size_t *   /* global_work_offset */,
				 const size_t *   /* global_work_size */,
				 const size_t *   /* local_work_size */,
				 cl_uint          /* num_events_in_wait_list */,
				 const cl_event * /* event_wait_list */,
				 cl_event *       /* event */) ;

  void getOpenCLFunctions();
}

#endif /* OPENCLFUNCTIONS_H */
