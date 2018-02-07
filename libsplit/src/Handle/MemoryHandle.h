#ifndef MEMORYHANDLE_H
#define MEMORYHANDLE_H

#include <Handle/ContextHandle.h>
#include <Utils/Retainable.h>
#include <ListInterval.h>

#include <CL/opencl.h>

namespace libsplit {

  class ContextHandle;

  class MemoryHandle : public Retainable {
  public:
    MemoryHandle(ContextHandle *context, cl_mem_flags flags, size_t size,
		 void *host_ptr);
    virtual ~MemoryHandle();

    void dumpMemoryState();

    cl_mem_flags mFlags;
    cl_mem_flags mTransFlags;
    size_t mSize; // original size
    size_t mMaxUsedSize;
    bool isRO;
    bool NOMEMCPY;
    const unsigned id;
    void *mHostPtr;

    // OpenCL buffers
    unsigned mNbBuffers;
    cl_mem *mBuffers;

    // Host buffer
    void *mLocalBuffer;

    // Context
    ContextHandle *mContext;

    // Valid regions for each devices and for the host buffer.
    ListInterval *devicesValidData;
    ListInterval hostValidData;

    int lastWriter;
  };

};

#endif /* MEMORYHANDLE_H */
