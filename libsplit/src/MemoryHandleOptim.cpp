#include <Debug.h>
#include "MemoryHandleOptim.h"
#include "Globals.h"
#include "Utils.h"

#include <iostream>

#include <string.h>

MemoryHandleOptim::MemoryHandleOptim(ContextHandle *context, cl_mem_flags flags,
				     size_t size, void *host_ptr) :
  MemoryHandle(context, flags, size, host_ptr) {

  localValidData = new ListInterval();
  buffersValidData = new ListInterval[mNbBuffers];

  if (flags & CL_MEM_COPY_HOST_PTR)
    localValidData->add(Interval(0, size-1));
}

MemoryHandleOptim::~MemoryHandleOptim() {
  delete localValidData;
  delete[] buffersValidData;
}

void *
MemoryHandleOptim::map(cl_command_queue command_queue,
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

  // Compute missing data.
  ListInterval missing;
  missing.add(Interval(offset, cb-offset-1));
  missing.difference(*localValidData);

  // Read from devices.
  for (unsigned d=0; d<mNbBuffers; d++) {
    ListInterval *intersection =
      ListInterval::intersection(missing, buffersValidData[d]);

    for (unsigned i=0; i<intersection->mList.size(); i++) {
      size_t myoffset = intersection->mList[i].lb;
      size_t mycb = intersection->mList[i].hb - intersection->mList[i].lb + 1;

      DEBUG("log", logger->logDtoH(d, id, intersection->mList[i]););

      err = real_clEnqueueReadBuffer(mContext->getQueueNo(d),
				     mBuffers[d],
				     CL_TRUE,
				     myoffset,
				     mycb,
				     (char *) mLocalPtr + myoffset,
				     0, NULL, NULL);
    }

    localValidData->myUnion(*intersection);
    missing.difference(*intersection);
    delete intersection;
  }

  // Unvalidate on devices.
  Interval inter(offset, offset+cb-1);
  for (unsigned d=0; d<mNbBuffers; d++)
    buffersValidData[d].remove(inter);

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
MemoryHandleOptim::write(cl_command_queue command_queue,
			 size_t offset,
			 size_t cb,
			 const void *ptr,
			 cl_uint num_events_in_wait_list,
			 const cl_event *event_wait_list,
			 cl_event *event) {
  MemoryHandle::write(command_queue, offset, cb, ptr, num_events_in_wait_list,
		      event_wait_list, event);

  // Update valid intervals
  Interval inter(offset, offset+cb-1);

  localValidData->add(inter);

  for (unsigned i=0; i<mNbBuffers; i++)
    buffersValidData[i].remove(inter);

}

void
MemoryHandleOptim::read(cl_command_queue command_queue,
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

  ListInterval needToRead;
  needToRead.add(Interval(offset, offset+cb-1));

  // Compute the intervals of data that are valid on the local buffer
  ListInterval *intersection =
    ListInterval::intersection(needToRead, *localValidData);

  // Copy them from local buffer to the host pointer
  if (NOMEMCPY && ptr == ((char *) mLocalPtr)+offset) {
    // DO NOTHING
  } else {
    for (unsigned id=0; id<intersection->mList.size(); id++) {
      size_t myoffset = intersection->mList[id].lb;
      size_t mycb = intersection->mList[id].hb - intersection->mList[id].lb + 1;
      memcpy((char *) ptr+myoffset-offset, (char *) mLocalPtr + myoffset, mycb);
    }
  }

  // Compute the intervals of data that are not valid on the local buffer
  needToRead.difference(*intersection);
  delete intersection;

  // Read them from devices buffers to the pointer pointer

  // For each device compute the intervals of data it owns
  // and read them from the device to the host pointer
  ListInterval *toReadDevice[mNbBuffers];
  size_t total = 0;

  for (unsigned d=0; d<mNbBuffers; d++) {
    toReadDevice[d] =
      ListInterval::intersection(needToRead, buffersValidData[d]);

    total += toReadDevice[d]->total();
    needToRead.difference(*toReadDevice[d]);
  }

  DEBUG("log",
	for (unsigned d=0; d<mNbBuffers; d++) {
	  for (unsigned id=0; id<toReadDevice[d]->mList.size(); id++) {
	    logger->logDtoH(d, this->id, toReadDevice[d]->mList[id]);
	  }
	}
	);

  // OMP Parallel version if enough data
  if (total > 256000) {
#pragma omp parallel for
    for (unsigned d=0; d<mNbBuffers; d++) {
      for (unsigned id=0; id<toReadDevice[d]->mList.size(); id++) {
	cl_int err;
	size_t myoffset = toReadDevice[d]->mList[id].lb;
	size_t mycb =
	  toReadDevice[d]->mList[id].hb - toReadDevice[d]->mList[id].lb + 1;


	err = real_clEnqueueReadBuffer(mContext->getQueueNo(d),
				       mBuffers[d],
				       CL_TRUE,
				       myoffset,
				       mycb,
				       (char *) ptr + myoffset - offset,
				       0, NULL, NULL);
	clCheck(err, __FILE__, __LINE__);
      }
    }
  } else {
    for (unsigned d=0; d<mNbBuffers; d++) {
      for (unsigned id=0; id<toReadDevice[d]->mList.size(); id++) {
	cl_int err;
	size_t myoffset = toReadDevice[d]->mList[id].lb;
	size_t mycb =
	  toReadDevice[d]->mList[id].hb - toReadDevice[d]->mList[id].lb + 1;

	err = real_clEnqueueReadBuffer(mContext->getQueueNo(d),
				       mBuffers[d],
				       CL_TRUE,
				       myoffset,
				       mycb,
				       (char *) ptr + myoffset - offset,
				       0, NULL, NULL);
	clCheck(err, __FILE__, __LINE__);
      }
    }
  }

  // If no memcpy and ptr == localptr, we can validate the data read on the
  // local buffer
  if (NOMEMCPY && ptr == ((char *) mLocalPtr) + offset) {
    for (unsigned d=0; d<mNbBuffers; d++) {
      localValidData->myUnion(*toReadDevice[d]);
    }
  }

  for (unsigned d=0; d<mNbBuffers; d++)
    delete toReadDevice[d];

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
MemoryHandleOptim::copy(cl_command_queue command_queue,
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


  // First ensure that local buffer contains valid data for the required
  // interval.

  ListInterval missing;
  Interval src_interval(src_offset, src_offset+cb-1);
  missing.add(src_interval);

  ListInterval *intersection =
    ListInterval::intersection(missing, *localValidData);

  missing.difference(*intersection);
  delete intersection;

  // Get needed data from devices.
  if (missing.mList.size() > 0) {
    for (unsigned d=0; d<mNbBuffers; d++) {
      intersection = ListInterval::intersection(missing,
						buffersValidData[d]);

      for (unsigned id=0; id<intersection->mList.size(); id++) {
	cl_int err;
	size_t myoffset = intersection->mList[id].lb;
	size_t mycb =
	  intersection->mList[id].hb - intersection->mList[id].lb + 1;

	DEBUG("log", logger->logDtoH(d, this->id, intersection->mList[id]););


	err = real_clEnqueueReadBuffer(mContext->getQueueNo(d),
				       mBuffers[d],
				       CL_TRUE,
				       myoffset,
				       mycb,
				       (char *) mLocalPtr + myoffset,
				       0, NULL, NULL);
	clCheck(err, __FILE__, __LINE__);
      }

      missing.difference(*intersection);
      delete intersection;
    }
  }

  localValidData->add(src_interval);

  // Then copy to dst buffer and validate the interval on local and invalidate
  // on device.
  memcpy((char *) dst_buffer->mLocalPtr + dst_offset,
	 (char *) mLocalPtr + src_offset,
	 cb);


  MemoryHandleOptim *dst = reinterpret_cast<MemoryHandleOptim *>(dst_buffer);

  Interval dstInterval(dst_offset, dst_offset+cb-1);

  dst->localValidData->add(dstInterval);

  for (unsigned d=0; d<mNbBuffers; d++)
    dst->buffersValidData[d].remove(dstInterval);

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
