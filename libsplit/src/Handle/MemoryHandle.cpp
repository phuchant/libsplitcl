#include <Handle/MemoryHandle.h>
#include <Utils/Debug.h>
#include <Utils/Utils.h>
#include <Options.h>
#include <Queue/DeviceQueue.h>

#include <cassert>
#include <cstring>
#include <iostream>

namespace libsplit {

  static unsigned numMemoryHandle = 0;

  MemoryHandle::MemoryHandle(ContextHandle *context, cl_mem_flags flags,
			     size_t size, void *host_ptr)
    : mFlags(flags), mSize(size), mMaxUsedSize(1), id(numMemoryHandle++),
      mHostPtr(host_ptr), mContext(context) {
    cl_int err;

    // Retain context
    mContext->retain();

    lastWriter = -1;

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
      mLocalBuffer = calloc(1, size);
      mHostPtr = mLocalBuffer;
      mMaxUsedSize = size;
    } else if (mFlags & CL_MEM_USE_HOST_PTR) {
      mLocalBuffer = mHostPtr;
      mMaxUsedSize = size;
    } else {
      if (optPinnedMem) {
	int firstGpuID = -1;

	for (unsigned i=0; i<context->getNbDevices(); i++) {
	  cl_device_id dev = context->getDevice(i);
	  cl_device_type dev_type;
	  cl_int err = real_clGetDeviceInfo(dev, CL_DEVICE_TYPE,
					    sizeof(dev_type),
					    &dev_type,
					    NULL);
	  clCheck(err, __FILE__, __LINE__);
	  if (dev_type == CL_DEVICE_TYPE_GPU) {
	    firstGpuID = i;
	    break;
	  }
	}
	assert(firstGpuID != -1);

	cl_mem dummyBuffer
	  = real_clCreateBuffer(context->getContext(firstGpuID),
				CL_MEM_ALLOC_HOST_PTR, size, NULL,
				&err);
	clCheck(err, __FILE__, __LINE__);
	mLocalBuffer = real_clEnqueueMapBuffer(context->getQueueNo(firstGpuID)
					       ->cl_queue,
					       dummyBuffer,
					       CL_TRUE,
					       CL_MAP_WRITE_INVALIDATE_REGION,
					       0, size, 0, NULL, NULL, &err);
	clCheck(err, __FILE__, __LINE__);
      } else {
	mLocalBuffer = calloc(1, size);
      }
    }

    if (flags & CL_MEM_COPY_HOST_PTR) {
      if (NOMEMCPY)
	mLocalBuffer = mHostPtr;
      else
	memcpy(mLocalBuffer, mHostPtr, mSize);

      mMaxUsedSize = size;
    }

    // Create devices regions.
    devicesValidData = new ListInterval[mNbBuffers];

    if (flags & CL_MEM_COPY_HOST_PTR)
      hostValidData.add(Interval(0, size-1));
  }

  MemoryHandle::~MemoryHandle() {
    cl_int err;

    for (unsigned i=0; i<mNbBuffers; i++) {
      err = real_clReleaseMemObject(mBuffers[i]);
      clCheck(err, __FILE__, __LINE__);
    }
    delete[] mBuffers;

    if (!(mFlags & CL_MEM_ALLOC_HOST_PTR) &&
	!(mFlags & CL_MEM_USE_HOST_PTR) && !NOMEMCPY) {
      if (!optPinnedMem)
	free(mLocalBuffer);
    }

    delete[] devicesValidData;

    mContext->release();
  }

  void
  MemoryHandle::dumpMemoryState() {
    std::cerr << "Memory Handle size : " << mSize << "\n";
    std::cerr << "max size used : " << mMaxUsedSize << "\n";
    std::cerr << "host valid data : ";
    hostValidData.debug();
    std::cerr << "\n";
    for (unsigned i=0; i<mNbBuffers; i++) {
      std::cerr << "device " << i << " valid data : ";
      devicesValidData[i].debug();
      std::cerr << "\n";
    }
  }

};
