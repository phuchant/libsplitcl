#ifndef MEMORYHANDLE_H
#define MEMORYHANDLE_H

#include "ContextHandle.h"
#include "Retainable.h"

#include <CL/opencl.h>

class MemoryHandle : public Retainable {
public:
  MemoryHandle(ContextHandle *context, cl_mem_flags flags, size_t size,
	       void *host_ptr);
  virtual ~MemoryHandle();

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

  virtual bool release();

  // private:

  cl_mem_flags mFlags;
  cl_mem_flags mTransFlags;

  unsigned mNbBuffers;
  cl_mem *mBuffers;

  void *mHostPtr;

  void *mLocalPtr;

  size_t mSize; // original size

  size_t mMaxUsedSize;

  bool isRO;

  bool NOMEMCPY;

  ContextHandle *mContext;

  const unsigned id;
};

#endif /* MEMORYHANDLE_H */
