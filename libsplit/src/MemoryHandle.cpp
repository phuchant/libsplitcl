#include <Debug.h>
#include <Options.h>
#include "MemoryHandle.h"
#include "Utils.h"

#include <iostream>

#include <cstring>

static unsigned numMemoryHandle = 0;

MemoryHandle::MemoryHandle(ContextHandle *context, cl_mem_flags flags,
			   size_t size, void *host_ptr)
  : mFlags(flags), mHostPtr(host_ptr), mSize(size),
    mMaxUsedSize(1), mContext(context), id(numMemoryHandle++) {
  cl_int err;

  // Retain context
  mContext->retain();

  isRO = flags & CL_MEM_READ_ONLY;

  if (optNoMemcpy)
    NOMEMCPY = true;
  else
    NOMEMCPY = false;

  DEBUG("memcpy", std::cerr << "NOMEMCPY: " << NOMEMCPY << "\n";);

  // Remove CL_MEM_USE_HOST_PTR , CL_MEM_COPY_HOST_PTR, CL_MEM_ALLOC_HOST_PTR
  // flags
  mTransFlags= mFlags;
  mTransFlags &= ~CL_MEM_USE_HOST_PTR;
  mTransFlags &= ~CL_MEM_COPY_HOST_PTR;
  mTransFlags &= ~CL_MEM_ALLOC_HOST_PTR;

  // Create one buffer per device
  mNbBuffers = context->getNbDevices();
  mBuffers = new cl_mem[mNbBuffers];
  for (unsigned i=0; i<mNbBuffers; i++) {
    mBuffers[i] = real_clCreateBuffer(context->getContext(i), mTransFlags,
				      size, NULL, &err);
    clCheck(err, __FILE__, __LINE__);
  }

  // Handle flags
  if (mFlags & CL_MEM_ALLOC_HOST_PTR) {
    mLocalPtr = calloc(1, size);
    mHostPtr = mLocalPtr;
    mMaxUsedSize = size;
  } else if (mFlags & CL_MEM_USE_HOST_PTR) {
    mLocalPtr = mHostPtr;
    mMaxUsedSize = size;
  } else {
    mLocalPtr = calloc(1, size);
  }

  if (flags & CL_MEM_COPY_HOST_PTR) {
    if (NOMEMCPY)
      mLocalPtr = mHostPtr;
    else
      memcpy(mLocalPtr, mHostPtr, mSize);

    mMaxUsedSize = size;
  }
}

MemoryHandle::~MemoryHandle() {
  cl_int err;

  for (unsigned i=0; i<mNbBuffers; i++) {
    err = real_clReleaseMemObject(mBuffers[i]);
    clCheck(err, __FILE__, __LINE__);
  }
  delete[] mBuffers;

  if (!(mFlags & CL_MEM_ALLOC_HOST_PTR) &&
      !(mFlags & CL_MEM_USE_HOST_PTR) && !NOMEMCPY)
    free(mLocalPtr);
}

void *
MemoryHandle::map(cl_command_queue command_queue,
		  cl_map_flags map_flags,
		  size_t offset,
		  size_t cb,
		  cl_uint num_events_in_wait_list,
		  const cl_event *event_wait_list,
		  cl_event *event) {
  (void) map_flags;
  (void) cb;

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

  return (char *) mLocalPtr + offset;
}

void
MemoryHandle::write(cl_command_queue command_queue,
		    size_t offset,
		    size_t cb,
		    const void *ptr,
		    cl_uint num_events_in_wait_list,
		    const cl_event *event_wait_list,
		    cl_event *event) {
  cl_int err;

  if (event_wait_list) {
    err = clWaitForEvents(num_events_in_wait_list, event_wait_list);
    clCheck(err, __FILE__, __LINE__);
  }

  // Update max used size
  size_t total_cb = offset + cb;
  mMaxUsedSize = total_cb > mMaxUsedSize ? total_cb : mMaxUsedSize;


  if (NOMEMCPY) {
    // FIXME: Doesn't work when offset is not zero.
    mLocalPtr = (void *)ptr;
  } else {
    // Copy datas
    memcpy((char *) mLocalPtr + offset, ptr, cb);
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
}

void
MemoryHandle::read(cl_command_queue command_queue,
		   size_t offset,
		   size_t cb,
		   void *ptr,
		   cl_uint num_events_in_wait_list,
		   const cl_event *event_wait_list,
		   cl_event *event) {
  cl_int err;

  if (event_wait_list) {
    err = clWaitForEvents(num_events_in_wait_list, event_wait_list);
    clCheck(err, __FILE__, __LINE__);
  }

  if (NOMEMCPY) {
  } else {
    memcpy(ptr, (char *) mLocalPtr + offset, cb);
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
}

void
MemoryHandle::copy(cl_command_queue command_queue,
		   MemoryHandle *dst_buffer,
		   size_t src_offset,
		   size_t dst_offset,
		   size_t cb,
		   cl_uint num_events_in_wait_list,
		   const cl_event *event_wait_list,
		   cl_event *event) {
  cl_int err;

  if (event_wait_list) {
    err = clWaitForEvents(num_events_in_wait_list, event_wait_list);
    clCheck(err, __FILE__, __LINE__);
  }

  // Update max used size
  size_t total_cb = dst_offset + cb;
  size_t dst_max = dst_buffer->mMaxUsedSize;
  dst_buffer->mMaxUsedSize = total_cb > dst_max ? total_cb : dst_max;

  memcpy((char *) dst_buffer->mLocalPtr + dst_offset,
	 (char *) mLocalPtr + src_offset,
	 cb);

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
}

bool
MemoryHandle::release() {
  if (--ref_count > 0)
    return false;

  mContext->release();

  return true;
}
