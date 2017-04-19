#ifndef MEMORYHANDLEOPTIM_H
#define MEMORYHANDLEOPTIM_H

#include "ContextHandle.h"
#include "ListInterval.h"
#include "MemoryHandle.h"

#include <CL/opencl.h>

class MemoryHandleOptim : public MemoryHandle {
public:
  MemoryHandleOptim(ContextHandle *context, cl_mem_flags flags, size_t size,
		    void *host_ptr);
  virtual ~MemoryHandleOptim();

  virtual void *map(cl_command_queue command_queue,
		    cl_map_flags map_flags,
		    size_t offset,
		    size_t cb,
		    cl_uint num_events_in_wait_list,
		    const cl_event *event_wait_list,
		    cl_event *event);

  /* void clEnqueueUnmapMemObject(cl_command_queue command_queue, */
  /* 			       cl_uint num_events_in_wait_list, */
  /* 			       const cl_event *event_wait_list, */
  /* 			       cl_event *event) */

  virtual void write(cl_command_queue command_queue,
		     size_t offset,
		     size_t cb,
		     const void *ptr,
		     cl_uint num_events_in_wait_list,
		     const cl_event *event_wait_list,
		     cl_event *event);

  virtual void read(cl_command_queue command_queue,
		    size_t offset,
		    size_t cb,
		    void *ptr,
		    cl_uint num_events_in_wait_list,
		    const cl_event *event_wait_list,
		    cl_event *event);

  virtual void copy(cl_command_queue command_queue,
		    MemoryHandle *dst_buffer,
		    size_t src_offset,
		    size_t dst_offset,
		    size_t cb,
		    cl_uint num_events_in_wait_list,
		    const cl_event *event_wait_list,
		    cl_event *event);

  // Valid intervals for the local buffer and for each device.
  ListInterval *buffersValidData;
  ListInterval *localValidData;

};

#endif /* MEMORYHANDLEOPTIM_H */
