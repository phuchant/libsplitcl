#include "ContextHandle.h"
#include "Globals.h"
#include "MemoryHandle.h"

#include "Utils.h"

#include <cstring>

#include <iostream>
#include <sstream>
#include <vector>

cl_mem clCreateBuffer (cl_context context,
		       cl_mem_flags flags,
		       size_t size,
		       void *host_ptr,
		       cl_int *errcode_ret)
{
  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);
  MemoryHandle *ret = bufferMgr->createMemoryHandle(c, flags, size, host_ptr);

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return (cl_mem) ret;
}

void *clEnqueueMapBuffer(cl_command_queue command_queue,
			 cl_mem buffer,
			 cl_bool blocking_map,
			 cl_map_flags map_flags,
			 size_t offset,
			 size_t cb,
			 cl_uint num_events_in_wait_list,
			 const cl_event *event_wait_list,
			 cl_event *event,
			 cl_int *errcode_ret)
{
  (void) blocking_map;
  (void) cb;
  (void) map_flags;

  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(buffer);

  void *ret = m->map(command_queue, map_flags, offset, cb,
		     num_events_in_wait_list, event_wait_list, event);

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return ret;
}

cl_int clEnqueueUnmapMemObject(cl_command_queue command_queue,
			       cl_mem memobj,
			       void *mapped_ptr,
			       cl_uint num_events_in_wait_list,
			       const cl_event *event_wait_list,
			       cl_event *event) {
  (void) mapped_ptr;
  (void) memobj;

  cl_int err;

  if (event_wait_list) {
    err = clWaitForEvents(num_events_in_wait_list, event_wait_list);
    clCheck(err, __FILE__, __LINE__);
  }

  if (event) {
    cl_context context;
    err = clGetCommandQueueInfo(command_queue, CL_QUEUE_CONTEXT,
				sizeof(context), &context, NULL);
    clCheck(err, __FILE__, __LINE__);
    *event = real_clCreateUserEvent(context, &err);
    clCheck(err, __FILE__, __LINE__);
    err = clSetUserEventStatus(*event, CL_COMPLETE);
    clCheck(err, __FILE__, __LINE__);
  }

  return CL_SUCCESS;
}

cl_int clEnqueueWriteBuffer (cl_command_queue command_queue,
			     cl_mem buffer,
			     cl_bool blocking_write,
			     size_t offset,
			     size_t cb,
			     const void *ptr,
			     cl_uint num_events_in_wait_list,
			     const cl_event *event_wait_list,
			     cl_event *event)
{
  (void) blocking_write;

  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(buffer);

  m->write(command_queue, offset, cb, ptr, num_events_in_wait_list,
	   event_wait_list, event);

  return CL_SUCCESS;
}

cl_int clEnqueueReadBuffer (cl_command_queue command_queue,
			    cl_mem buffer,
			    cl_bool blocking_read,
			    size_t offset,
			    size_t cb,
			    void *ptr,
			    cl_uint num_events_in_wait_list,
			    const cl_event *event_wait_list,
			    cl_event *event)
{
  (void) blocking_read;

  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(buffer);

  m->read(command_queue, offset, cb, ptr, num_events_in_wait_list,
	  event_wait_list, event);

  return CL_SUCCESS;
}

cl_int clEnqueueCopyBuffer (cl_command_queue command_queue,
			    cl_mem src_buffer,
			    cl_mem dst_buffer,
			    size_t src_offset,
			    size_t dst_offset,
			    size_t cb,
			    cl_uint num_events_in_wait_list,
			    const cl_event *event_wait_list,
			    cl_event *event) {
  MemoryHandle *src = reinterpret_cast<MemoryHandle *>(src_buffer);
  MemoryHandle *dst = reinterpret_cast<MemoryHandle *>(dst_buffer);

  src->copy(command_queue, dst, src_offset, dst_offset, cb,
	    num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

cl_int clRetainMemObject(cl_mem memobj)
{
  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(memobj);

  m->retain();

  return CL_SUCCESS;
}

cl_int clReleaseMemObject(cl_mem memobj)
{
  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(memobj);

  if (m->release())
    delete m;

  return CL_SUCCESS;
}

cl_mem clCreateSubBuffer(cl_mem buffer,
			 cl_mem_flags flags,
			 cl_buffer_create_type buffer_create_type,
			 const void *buffer_create_info,
			 cl_int *errcode_ret)
{
  (void) buffer;
  (void) flags;
  (void) buffer_create_type;
  (void) buffer_create_info;
  (void) errcode_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueReadBufferRect(cl_command_queue command_queue,
			       cl_mem buffer,
			       cl_bool blocking_read,
			       const size_t buffer_origin[3],
			       const size_t host_origin[3],
			       const size_t region[3],
			       size_t buffer_row_pitch,
			       size_t buffer_slice_pitch,
			       size_t host_row_pitch,
			       size_t host_slice_pitch,
			       void *ptr,
			       cl_uint num_events_in_wait_list,
			       const cl_event *event_wait_list,
			       cl_event *event)
{
  (void) command_queue;
  (void) buffer;
  (void) blocking_read;
  (void) buffer_origin;
  (void) host_origin;
  (void) region;
  (void) buffer_row_pitch;
  (void) buffer_slice_pitch;
  (void) host_row_pitch;
  (void) host_slice_pitch;
  (void) ptr;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueWriteBufferRect(cl_command_queue command_queue,
				cl_mem buffer,
				cl_bool blocking_write,
				const size_t buffer_origin[3],
				const size_t host_origin[3],
				const size_t region[3],
				size_t buffer_row_pitch,
				size_t buffer_slice_pitch,
				size_t host_row_pitch,
				size_t host_slice_pitch,
				void *ptr,
				cl_uint num_events_in_wait_list,
				const cl_event *event_wait_list,
				cl_event *event)
{
  (void) command_queue;
  (void) buffer;
  (void) blocking_write;
  (void) buffer_origin;
  (void) host_origin;
  (void) region;
  (void) buffer_row_pitch;
  (void) buffer_slice_pitch;
  (void) host_row_pitch;
  (void) host_slice_pitch;
  (void) ptr;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clSetMemObjectDestructorCallback (cl_mem memobj,
					 void (CL_CALLBACK  *pfn_notify)
					 (cl_mem memobj, void *user_data),
					 void *user_data)
{
  (void) memobj;
  (void) pfn_notify;
  (void) user_data;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_mem clCreateImage2D(cl_context context,
		       cl_mem_flags flags,
		       const cl_image_format *image_format,
		       size_t image_width,
		       size_t image_height,
		       size_t image_row_pitch,
		       void *host_ptr,
		       cl_int *errcode_ret)
{
  (void) context;
  (void) flags;
  (void) image_format;
  (void) image_width;
  (void) image_height;
  (void) image_row_pitch;
  (void) host_ptr;
  (void) errcode_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_mem clCreateImage3D(cl_context context,
		       cl_mem_flags flags,
		       const cl_image_format *image_format,
		       size_t image_width,
		       size_t image_height,
		       size_t image_depth,
		       size_t image_row_pitch,
		       size_t image_slice_pitch,
		       void *host_ptr,
		       cl_int *errcode_ret)
{
  (void) context;
  (void) flags;
  (void) image_format;
  (void) image_width;
  (void) image_height;
  (void) image_depth;
  (void) image_row_pitch;
  (void) image_slice_pitch;
  (void) host_ptr;
  (void) errcode_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clGetSupportedImageFormats(cl_context context,
				  cl_mem_flags flags,
				  cl_mem_object_type image_type,
				  cl_uint num_entries,
				  cl_image_format *image_formats,
				  cl_uint *num_image_formats)
{
  (void) context;
  (void) flags;
  (void) image_type;
  (void) num_entries;
  (void) image_formats;
  (void) num_image_formats;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueReadImage ( cl_command_queue command_queue,
			    cl_mem image,
			    cl_bool blocking_read,
			    const size_t origin[3],
			    const size_t region[3],
			    size_t row_pitch,
			    size_t slice_pitch,
			    void *ptr,
			    cl_uint num_events_in_wait_list,
			    const cl_event *event_wait_list,
			    cl_event *event)
{
  (void) command_queue;
  (void) image;
  (void) blocking_read;
  (void) origin;
  (void) region;
  (void) row_pitch;
  (void) slice_pitch;
  (void) ptr;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueWriteImage ( cl_command_queue command_queue,
			     cl_mem image,
			     cl_bool blocking_write,
			     const size_t origin[3],
			     const size_t region[3],
			     size_t input_row_pitch,
			     size_t input_slice_pitch,
			     const void * ptr,
			     cl_uint num_events_in_wait_list,
			     const cl_event *event_wait_list,
			     cl_event *event)
{
  (void) command_queue;
  (void) image;
  (void) blocking_write;
  (void) origin;
  (void) region;
  (void) input_row_pitch;
  (void) input_slice_pitch;
  (void) ptr;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}


cl_int clEnqueueCopyImage ( cl_command_queue command_queue,
			    cl_mem src_image,
			    cl_mem dst_image,
			    const size_t src_origin[3],
			    const size_t dst_origin[3],
			    const size_t region[3],
			    cl_uint num_events_in_wait_list,
			    const cl_event *event_wait_list,
			    cl_event *event)
{
  (void) command_queue;
  (void) src_image;
  (void) dst_image;
  (void) src_origin;
  (void) dst_origin;
  (void) region;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueCopyImageToBuffer ( cl_command_queue command_queue,
				    cl_mem src_image,
				    cl_mem  dst_buffer,
				    const size_t src_origin[3],
				    const size_t region[3],
				    size_t dst_offset,
				    cl_uint num_events_in_wait_list,
				    const cl_event *event_wait_list,
				    cl_event *event)
{
  (void) command_queue;
  (void) src_image;
  (void) dst_buffer;
  (void) src_origin;
  (void) region;
  (void) dst_offset;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueCopyBufferRect ( cl_command_queue command_queue,
				 cl_mem src_buffer,
				 cl_mem dst_buffer,
				 const size_t src_origin[3],
				 const size_t dst_origin[3],
				 const size_t region[3],
				 size_t src_row_pitch,
				 size_t src_slice_pitch,
				 size_t dst_row_pitch,
				 size_t dst_slice_pitch,
				 cl_uint num_events_in_wait_list,
				 const cl_event *event_wait_list,
				 cl_event *event)
{
  (void) command_queue;
  (void) src_buffer;
  (void) dst_buffer;
  (void) src_origin;
  (void) dst_origin;
  (void) region;
  (void) src_row_pitch;
  (void) src_slice_pitch;
  (void) dst_row_pitch;
  (void) dst_slice_pitch;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clEnqueueCopyBufferToImage ( cl_command_queue command_queue,
				    cl_mem src_buffer,
				    cl_mem  dst_image,
				    size_t src_offset,
				    const size_t dst_origin[3],
				    const size_t region[3],
				    cl_uint num_events_in_wait_list,
				    const cl_event *event_wait_list,
				    cl_event *event)
{
  (void) command_queue;
  (void) src_buffer;
  (void) dst_image;
  (void) src_offset;
  (void) dst_origin;
  (void) region;
  (void) num_events_in_wait_list;
  (void) event_wait_list;
  (void) event;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clGetMemObjectInfo ( cl_mem memobj,
			    cl_mem_info param_name,
			    size_t param_value_size,
			    void *param_value,
			    size_t *param_value_size_ret)
{
  (void) memobj;
  (void) param_name;
  (void) param_value_size;
  (void) param_value;
  (void) param_value_size_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clGetImageInfo ( cl_mem image,
			cl_image_info param_name,
			size_t param_value_size,
			void *param_value,
			size_t *param_value_size_ret)
{
  (void) image;
  (void) param_name;
  (void) param_value_size;
  (void) param_value;
  (void) param_value_size_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

