#include <Driver.h>
#include <Globals.h>
#include <Handle/MemoryHandle.h>
#include <Handle/KernelHandle.h>

#include <iostream>

#include <CL/cl.h>

using namespace libsplit;

/* Enqueued Commands APIs */
cl_int
clEnqueueReadBuffer(cl_command_queue    command_queue,
		    cl_mem              buffer,
		    cl_bool             blocking_read,
		    size_t              offset,
		    size_t              size,
		    void *              ptr,
		    cl_uint             num_events_in_wait_list,
		    const cl_event *    event_wait_list,
		    cl_event *          event)
{
  (void) command_queue;

  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(buffer);
  driver->enqueueReadBuffer(command_queue, m, blocking_read, offset, size, ptr,
			    num_events_in_wait_list,
			    event_wait_list,
			    event);

  return CL_SUCCESS;
}

cl_int
clEnqueueReadBufferRect(cl_command_queue    /* command_queue */,
			cl_mem              /* buffer */,
			cl_bool             /* blocking_read */,
			const size_t *      /* buffer_offset */,
			const size_t *      /* host_offset */,
			const size_t *      /* region */,
			size_t              /* buffer_row_pitch */,
			size_t              /* buffer_slice_pitch */,
			size_t              /* host_row_pitch */,
			size_t              /* host_slice_pitch */,
			void *              /* ptr */,
			cl_uint             /* num_events_in_wait_list */,
			const cl_event *    /* event_wait_list */,
			cl_event *          /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueWriteBuffer(cl_command_queue   command_queue ,
		     cl_mem             buffer,
		     cl_bool            blocking_write,
		     size_t             offset,
		     size_t             size,
		     const void *       ptr,
		     cl_uint            num_events_in_wait_list,
		     const cl_event *   event_wait_list,
		     cl_event *         event)
{
  (void) command_queue;

  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(buffer);
  driver->enqueueWriteBuffer(command_queue, m, blocking_write, offset, size,
			     ptr, num_events_in_wait_list, event_wait_list,
			     event);

  return CL_SUCCESS;
}

cl_int
clEnqueueWriteBufferRect(cl_command_queue    /* command_queue */,
			 cl_mem              /* buffer */,
			 cl_bool             /* blocking_write */,
			 const size_t *      /* buffer_offset */,
			 const size_t *      /* host_offset */,
			 const size_t *      /* region */,
			 size_t              /* buffer_row_pitch */,
			 size_t              /* buffer_slice_pitch */,
			 size_t              /* host_row_pitch */,
			 size_t              /* host_slice_pitch */,
			 const void *        /* ptr */,
			 cl_uint             /* num_events_in_wait_list */,
			 const cl_event *    /* event_wait_list */,
			 cl_event *          /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueFillBuffer(cl_command_queue   /* command_queue */,
		    cl_mem             /* buffer */,
		    const void *       /* pattern */,
		    size_t             /* pattern_size */,
		    size_t             /* offset */,
		    size_t             /* size */,
		    cl_uint            /* num_events_in_wait_list */,
		    const cl_event *   /* event_wait_list */,
		    cl_event *         /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueCopyBuffer(cl_command_queue    command_queue,
		    cl_mem              src_buffer,
		    cl_mem              dst_buffer,
		    size_t              src_offset,
		    size_t              dst_offset,
		    size_t              size,
		    cl_uint             num_events_in_wait_list,
		    const cl_event *    event_wait_list,
		    cl_event *          event)
{
  MemoryHandle *s = reinterpret_cast<MemoryHandle *>(src_buffer);
  MemoryHandle *d = reinterpret_cast<MemoryHandle *>(dst_buffer);

  driver->enqueueCopyBuffer(command_queue, s, d, src_offset, dst_offset, size,
			    num_events_in_wait_list,
			    event_wait_list,
			    event);
  return CL_SUCCESS;
}

cl_int
clEnqueueCopyBufferRect(cl_command_queue    /* command_queue */,
			cl_mem              /* src_buffer */,
			cl_mem              /* dst_buffer */,
			const size_t *      /* src_origin */,
			const size_t *      /* dst_origin */,
			const size_t *      /* region */,
			size_t              /* src_row_pitch */,
			size_t              /* src_slice_pitch */,
			size_t              /* dst_row_pitch */,
			size_t              /* dst_slice_pitch */,
			cl_uint             /* num_events_in_wait_list */,
			const cl_event *    /* event_wait_list */,
			cl_event *          /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueReadImage(cl_command_queue     /* command_queue */,
		   cl_mem               /* image */,
		   cl_bool              /* blocking_read */,
		   const size_t *       /* origin[3] */,
		   const size_t *       /* region[3] */,
		   size_t               /* row_pitch */,
		   size_t               /* slice_pitch */,
		   void *               /* ptr */,
		   cl_uint              /* num_events_in_wait_list */,
		   const cl_event *     /* event_wait_list */,
		   cl_event *           /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueWriteImage(cl_command_queue    /* command_queue */,
		    cl_mem              /* image */,
		    cl_bool             /* blocking_write */,
		    const size_t *      /* origin[3] */,
		    const size_t *      /* region[3] */,
		    size_t              /* input_row_pitch */,
		    size_t              /* input_slice_pitch */,
		    const void *        /* ptr */,
		    cl_uint             /* num_events_in_wait_list */,
		    const cl_event *    /* event_wait_list */,
		    cl_event *          /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueFillImage(cl_command_queue   /* command_queue */,
		   cl_mem             /* image */,
		   const void *       /* fill_color */,
		   const size_t *     /* origin[3] */,
		   const size_t *     /* region[3] */,
		   cl_uint            /* num_events_in_wait_list */,
		   const cl_event *   /* event_wait_list */,
		   cl_event *         /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueCopyImage(cl_command_queue     /* command_queue */,
		   cl_mem               /* src_image */,
		   cl_mem               /* dst_image */,
		   const size_t *       /* src_origin[3] */,
		   const size_t *       /* dst_origin[3] */,
		   const size_t *       /* region[3] */,
		   cl_uint              /* num_events_in_wait_list */,
		   const cl_event *     /* event_wait_list */,
		   cl_event *           /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueCopyImageToBuffer(cl_command_queue /* command_queue */,
			   cl_mem           /* src_image */,
			   cl_mem           /* dst_buffer */,
			   const size_t *   /* src_origin[3] */,
			   const size_t *   /* region[3] */,
			   size_t           /* dst_offset */,
			   cl_uint          /* num_events_in_wait_list */,
			   const cl_event * /* event_wait_list */,
			   cl_event *       /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueCopyBufferToImage(cl_command_queue /* command_queue */,
			   cl_mem           /* src_buffer */,
			   cl_mem           /* dst_image */,
			   size_t           /* src_offset */,
			   const size_t *   /* dst_origin[3] */,
			   const size_t *   /* region[3] */,
			   cl_uint          /* num_events_in_wait_list */,
			   const cl_event * /* event_wait_list */,
			   cl_event *       /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

void *
clEnqueueMapBuffer(cl_command_queue /* command_queue */,
		   cl_mem           /* buffer */,
		   cl_bool          /* blocking_map */,
		   cl_map_flags     /* map_flags */,
		   size_t           /* offset */,
		   size_t           /* size */,
		   cl_uint          /* num_events_in_wait_list */,
		   const cl_event * /* event_wait_list */,
		   cl_event *       /* event */,
		   cl_int *         /* errcode_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

void *
clEnqueueMapImage(cl_command_queue  /* command_queue */,
		  cl_mem            /* image */,
		  cl_bool           /* blocking_map */,
		  cl_map_flags      /* map_flags */,
		  const size_t *    /* origin[3] */,
		  const size_t *    /* region[3] */,
		  size_t *          /* image_row_pitch */,
		  size_t *          /* image_slice_pitch */,
		  cl_uint           /* num_events_in_wait_list */,
		  const cl_event *  /* event_wait_list */,
		  cl_event *        /* event */,
		  cl_int *          /* errcode_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueUnmapMemObject(cl_command_queue /* command_queue */,
			cl_mem           /* memobj */,
			void *           /* mapped_ptr */,
			cl_uint          /* num_events_in_wait_list */,
			const cl_event *  /* event_wait_list */,
			cl_event *        /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueMigrateMemObjects(cl_command_queue       /* command_queue */,
			   cl_uint                /* num_mem_objects */,
			   const cl_mem *         /* mem_objects */,
			   cl_mem_migration_flags /* flags */,
			   cl_uint                /* num_events_in_wait_list */,
			   const cl_event *       /* event_wait_list */,
			   cl_event *             /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueNDRangeKernel(cl_command_queue command_queue,
		       cl_kernel        kernel,
		       cl_uint          work_dim,
		       const size_t *   global_work_offset,
		       const size_t *   global_work_size,
		       const size_t *   local_work_size,
		       cl_uint          num_events_in_wait_list,
		       const cl_event * event_wait_list,
		       cl_event *       event)
{
  KernelHandle *k = reinterpret_cast<KernelHandle *>(kernel);
  driver->enqueueNDRangeKernel(command_queue, k, work_dim, global_work_offset,
			       global_work_size, local_work_size,
			       num_events_in_wait_list,
			       event_wait_list, event);

  return CL_SUCCESS;
}

cl_int
clEnqueueTask(cl_command_queue  /* command_queue */,
	      cl_kernel         /* kernel */,
	      cl_uint           /* num_events_in_wait_list */,
	      const cl_event *  /* event_wait_list */,
	      cl_event *        /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueNativeKernel(cl_command_queue  /* command_queue */,
		      void (CL_CALLBACK * /*user_func*/)(void *),
		      void *            /* args */,
		      size_t            /* cb_args */,
		      cl_uint           /* num_mem_objects */,
		      const cl_mem *    /* mem_list */,
		      const void **     /* args_mem_loc */,
		      cl_uint           /* num_events_in_wait_list */,
		      const cl_event *  /* event_wait_list */,
		      cl_event *        /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueMarkerWithWaitList(cl_command_queue /* command_queue */,
			    cl_uint           /* num_events_in_wait_list */,
			    const cl_event *  /* event_wait_list */,
			    cl_event *        /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueBarrierWithWaitList(cl_command_queue /* command_queue */,
			     cl_uint           /* num_events_in_wait_list */,
			     const cl_event *  /* event_wait_list */,
			     cl_event *        /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueMarker(cl_command_queue    /* command_queue */,
		cl_event *          /* event */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueWaitForEvents(cl_command_queue /* command_queue */,
		       cl_uint          /* num_events */,
		       const cl_event * /* event_list */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clEnqueueBarrier(cl_command_queue /* command_queue */)
{
  // Nothing to do since we use in order queues.
  return CL_SUCCESS;
}
