#include "BufferManagerSimple.h"
#include "Utils.h"

BufferManagerSimple::BufferManagerSimple() : BufferManager() {}

BufferManagerSimple::~BufferManagerSimple() {}

MemoryHandle *
BufferManagerSimple::createMemoryHandle(ContextHandle *context,
					cl_mem_flags flags, size_t size,
					void *host_ptr) {
  return new MemoryHandle(context, flags, size, host_ptr);
}

void
BufferManagerSimple::writeSingle(DeviceInfo *devInfo, unsigned devId,
				 unsigned nbGlobals, unsigned *globalsPos,
				 MemoryHandle **memArgs) {
  cl_command_queue queue = devInfo[devId].queue;
  cl_int err;

  for (unsigned a=0; a<nbGlobals; a++) {
    unsigned pos = globalsPos[a];

    // If the buffer is RO, send just the max used
    size_t cb = memArgs[pos]->isRO ?
      memArgs[pos]->mMaxUsedSize : memArgs[pos]->mSize;

    err = real_clEnqueueWriteBuffer(queue,
  				    memArgs[pos]->mBuffers[devId],
  				    CL_FALSE,
  				    0,
  				    cb,
  				    (char *) memArgs[pos]->mLocalPtr,
  				    0, NULL, NULL);
    clCheck(err, __FILE__, __LINE__);
  }
}


void
BufferManagerSimple::readSingle(DeviceInfo *devInfo, unsigned devId,
				unsigned nbGlobals, unsigned *globalsPos,
				MemoryHandle **memArgs) {
  cl_command_queue queue = devInfo[devId].queue;
  cl_int err;

  // Read datas
  for (unsigned a=0; a<nbGlobals; a++) {
    unsigned pos = globalsPos[a];

    // Do not read RO buffers
    if (memArgs[pos]->isRO)
      continue;

    err = real_clEnqueueReadBuffer(queue,
				   memArgs[pos]->mBuffers[devId],
				   CL_TRUE,
				   0,
				   memArgs[pos]->mSize,
				   (char *) memArgs[pos]->mLocalPtr,
				   0, NULL, NULL);
    clCheck(err, __FILE__, __LINE__);
  }
}

size_t
BufferManagerSimple::computeAndReadMissing(DeviceInfo *devInfo,
					   unsigned nbDevices,
					   unsigned nbGlobals,
					   unsigned *globalsPos,
					   MemoryHandle **memArgs) {
  /* DO NOTHING */
  return 0;
}

void
BufferManagerSimple::writeDeviceNo(DeviceInfo *devInfo, unsigned d,
				   unsigned nbGlobals,
				   unsigned *globalsPos, MemoryHandle **memArgs) {
  cl_int err;
  cl_command_queue queue = devInfo[d].queue;

  // Write global arguments
  for (unsigned a=0; a<nbGlobals; a++) {
    unsigned pos = globalsPos[a];

    // If interval is undefined, send the whole buffer
    if (devInfo[d].dataRequired[a].isUndefined()) {
      devInfo[d].dataRequired[a].clearList();

      // If buffer is RO, send just the max used
      size_t cb = memArgs[pos]->isRO ?
	memArgs[pos]->mMaxUsedSize : memArgs[pos]->mSize;

      devInfo[d].dataRequired[a].add(Interval(0, cb-1));
    }

    for (unsigned i=0; i<devInfo[d].dataRequired[a].mList.size(); i++) {
      size_t offset = devInfo[d].dataRequired[a].mList[i].lb;
      size_t cb = devInfo[d].dataRequired[a].mList[i].hb - offset + 1;
      cl_event event;
      devInfo[d].commEvents.push_back(event);

      err = real_clEnqueueWriteBuffer(queue,
				      memArgs[pos]->mBuffers[d],
				      CL_FALSE,
				      offset,
				      cb,
				      (char *) memArgs[pos]->mLocalPtr +
				      offset,
				      0, NULL,
				      &devInfo[d].commEvents
				      [devInfo[d].commEvents.size()-1]);

      clCheck(err, __FILE__, __LINE__);
    }
  }
}

void
BufferManagerSimple::readDevices(DeviceInfo *devInfo, unsigned nbDevices,
				 unsigned nbGlobals, unsigned *globalsPos,
				 bool *argIsMerge,
				 MemoryHandle **memArgs) {
  // For each device read data written
  for (unsigned d=0; d<nbDevices; d++) {
    cl_int err;
    cl_command_queue queue = devInfo[d].queue;

    // read global arguments
    for (unsigned a=0; a<nbGlobals; a++) {
      unsigned pos = globalsPos[a];
      if (argIsMerge[pos])
	continue;

      for (unsigned i=0; i<devInfo[d].dataWritten[a].mList.size(); i++) {
	size_t offset = devInfo[d].dataWritten[a].mList[i].lb;
	size_t cb = devInfo[d].dataWritten[a].mList[i].hb
	  - offset + 1;
	cl_event event;
	devInfo[d].commEvents.push_back(event);

	err = real_clEnqueueReadBuffer(queue,
				       memArgs[pos]->mBuffers[d],
				       CL_TRUE,
				       offset,
				       cb,
				       (char *) memArgs[pos]->mLocalPtr +
				       offset,
				       0, NULL,
				       &devInfo[d].commEvents
				       [devInfo[d].commEvents.size()-1]);
	clCheck(err, __FILE__, __LINE__);
      }
    }
  }
}

void
BufferManagerSimple::fakeReadDevices(DeviceInfo *devInfo, unsigned nbDevices,
				     unsigned nbGlobals, unsigned *globalsPos,
				     bool *argIsMerge,
				     MemoryHandle **memArgs) {
  // For each device read data written
  for (unsigned d=0; d<nbDevices; d++) {
    cl_int err;
    cl_command_queue queue = devInfo[d].queue;

    // read global arguments
    for (unsigned a=0; a<nbGlobals; a++) {
      unsigned pos = globalsPos[a];

      if (devInfo[d].dataWritten[a].isUndefined()) {
	size_t totalSize = memArgs[pos]->mSize;
	size_t blockSize = totalSize / nbDevices;

	if (blockSize == 0) {
	  std::cerr << "error : block size = 0\n";
	  exit(EXIT_FAILURE);
	}

	size_t start = d * blockSize;
	size_t end = start + blockSize - 1;

	if (d == nbDevices-1 && end != totalSize-1)
	  end = totalSize-1;

	devInfo[d].dataWritten[a].clearList();
	devInfo[d].dataWritten[a].add(Interval(start, end));
      }

      for (unsigned i=0; i<devInfo[d].dataWritten[a].mList.size(); i++) {
	size_t offset = devInfo[d].dataWritten[a].mList[i].lb;
	size_t cb = devInfo[d].dataWritten[a].mList[i].hb
	  - offset + 1;
	cl_event event;
	devInfo[d].commEvents.push_back(event);

	err = real_clEnqueueReadBuffer(queue,
				       memArgs[pos]->mBuffers[d],
				       CL_TRUE,
				       offset,
				       cb,
				       (char *) memArgs[pos]->mLocalPtr +
				       offset,
				       0, NULL,
				       &devInfo[d].commEvents
				       [devInfo[d].commEvents.size()-1]);
	clCheck(err, __FILE__, __LINE__);
      }
    }
  }
}

void
BufferManagerSimple::readDevicesMerge(unsigned nbDevices,
				      unsigned nbWrittenArgs,
				      unsigned *writtenArgsPos,
				      MemoryHandle **memArgs) {
  (void) nbDevices;
  (void) nbWrittenArgs;
  (void) writtenArgsPos;
  (void) memArgs;

  // Do nothing.
}
