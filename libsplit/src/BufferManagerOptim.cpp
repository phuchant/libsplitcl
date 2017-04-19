#include "BufferManagerOptim.h"
#include <Debug.h>
#include "Globals.h"
#include "Utils.h"

#include <cassert>

void
BufferManagerOptim::readLocalMissing(DeviceInfo *devInfo, unsigned nbDevices,
				     MemoryHandleOptim *memHandle,
				     ListInterval *required) {
  // Compute the intervals of data we need to write and we don't have in
  // the local buffer (needTowrite \ localValidDatas)
  ListInterval *needToRead =
    ListInterval::difference(*required,
			     *memHandle->localValidData);

  // Read the data we don't have from others devices to the local buffer
  // So for each device :
  for (unsigned d=0; d<nbDevices; d++) {
    // Compute the intervals of data it has
    ListInterval *intersection =
      ListInterval::intersection(*needToRead,
				 memHandle->buffersValidData[d]);


    // Read them to the local buffer
    for (unsigned i=0; i<intersection->mList.size(); i++) {
      cl_int err;
      size_t offset = intersection->mList[i].lb;
      size_t cb = intersection->mList[i].hb -
	intersection->mList[i].lb + 1;
      // cl_event event;
      // devInfo[devId].commEvents.push_back(event);


      DEBUG("log", logger->logDtoH(d, memHandle->id, intersection->mList[i]); );

      err = real_clEnqueueReadBuffer(devInfo[d].queue,
				     memHandle->mBuffers[d],
				     CL_TRUE,
				     offset,
				     cb,
				     (char *) memHandle->mLocalPtr + offset,
				     0, NULL,
				     NULL);

      clCheck(err, __FILE__, __LINE__);

      // Validate the data read on the local buffer
      memHandle->localValidData->add(intersection->mList[i]);
    }

    needToRead->difference(*intersection);

    delete intersection;
  }

  delete needToRead;
}

BufferManagerOptim::BufferManagerOptim() : BufferManager() {}

BufferManagerOptim::~BufferManagerOptim() {}

MemoryHandle *
BufferManagerOptim::createMemoryHandle(ContextHandle *context,
				       cl_mem_flags flags, size_t size,
				       void *host_ptr) {
  return new MemoryHandleOptim(context, flags, size, host_ptr);
}

void
BufferManagerOptim::writeSingle(DeviceInfo *devInfo, unsigned devId,
				unsigned nbGlobals, unsigned *globalsPos,
				MemoryHandle **memArgs) {
  cl_int err;

  for (unsigned a=0; a<nbGlobals; a++) {
    unsigned pos = globalsPos[a];

    // If the buffer is RO, send just the max used
    size_t cb = memArgs[pos]->isRO ?
      memArgs[pos]->mMaxUsedSize : memArgs[pos]->mSize;

    // Compute the data required.
    ListInterval dataRequired;
    dataRequired.add(Interval(0, cb-1));

    MemoryHandleOptim *memHandle =
      static_cast<MemoryHandleOptim *>(memArgs[pos]);

    // Compute the data we need to write
    // (dataRequired \ validDatas)
    ListInterval *needToWrite =
      ListInterval::difference(dataRequired,
			       memHandle->buffersValidData[devId]);

    // Read the data we don't have from others devices to the local buffer
    readLocalMissing(devInfo, memHandle->mNbBuffers, memHandle, needToWrite);

    // Write the needed data from the local buffer to the device
    for (unsigned i=0; i<needToWrite->mList.size(); i++) {
      size_t offset = needToWrite->mList[i].lb;
      size_t cb = needToWrite->mList[i].hb -
	needToWrite->mList[i].lb + 1;

      DEBUG("log",
	    logger->logHtoD(devId, memArgs[pos]->id, needToWrite->mList[i]);
	    );

      err = real_clEnqueueWriteBuffer(devInfo[devId].queue,
				      memArgs[pos]->mBuffers[devId],
				      CL_FALSE,
				      offset,
				      cb,
				      (char *) memArgs[pos]->mLocalPtr +
				      offset,
				      0, NULL, NULL);
      clCheck(err, __FILE__, __LINE__);
    }

    // Validate the data written on the device
    memHandle->buffersValidData[devId].myUnion(*needToWrite);

    delete needToWrite;
  }
}


void
BufferManagerOptim::readSingle(DeviceInfo *devInfo, unsigned devId,
			       unsigned nbGlobals, unsigned *globalsPos,
			       MemoryHandle **memArgs) {
  (void) devInfo;

  // Do not read any data, just unvalidate the data written the kernel on
  // others buffers.

  for (unsigned a=0; a<nbGlobals; a++) {
    unsigned pos = globalsPos[a];
    unsigned nbDevices = memArgs[pos]->mNbBuffers;
    MemoryHandleOptim *memHandle =
      static_cast<MemoryHandleOptim *>(memArgs[pos]);

    // If the buffer is RO don't unvalidate any buffer
    if (memHandle->isRO)
      continue;

    ListInterval dataWritten;

    dataWritten.add(Interval(0, memHandle->mSize-1));

    for (unsigned d2=0; d2<nbDevices; d2++) {
      if (devId == d2)
	continue;
      memHandle->buffersValidData[d2].difference(dataWritten);
    }
    memHandle->localValidData->difference(dataWritten);
  }
}

size_t
BufferManagerOptim::computeAndReadMissing(DeviceInfo *devInfo,
					  unsigned nbDevices,
					  unsigned nbGlobals,
					  unsigned *globalsPos,
					  MemoryHandle **memArgs) {
  size_t total = 0;

  // Compute the data needed for each device (dataRequired and needToWrite)
  for (unsigned d=0; d<nbDevices; d++) {
    for (unsigned a=0; a<nbGlobals; a++) {
      unsigned pos = globalsPos[a];

      MemoryHandleOptim *memHandle =
	static_cast<MemoryHandleOptim *>(memArgs[pos]);

      // Clear needToRead
      devInfo[d].needToRead[a].clear();

      // If interval is undefined, send the whole buffer
      if (devInfo[d].dataRequired[a].isUndefined()) {
	devInfo[d].dataRequired[a].clearList();

	// If buffer is RO, send just the max used
	size_t cb = memHandle->isRO ?
	  memHandle->mMaxUsedSize : memHandle->mSize;

	devInfo[d].dataRequired[a].add(Interval(0, cb-1));
      }

      // Restrict interval to buffer size
      Interval bufferInterval(0, memHandle->mSize-1);
      ListInterval bufferListInterval;
      bufferListInterval.add(bufferInterval);
      ListInterval *restrictInterval =
	ListInterval::intersection(devInfo[d].dataRequired[a],
				   bufferListInterval);
      devInfo[d].dataRequired[a].clearList();
      devInfo[d].dataRequired[a].myUnion(*restrictInterval);
      delete restrictInterval;

      // Compute the data we need to write
      // (dataRequired \ validDatas)
      devInfo[d].needToWrite[a] =
	ListInterval::difference(devInfo[d].dataRequired[a],
				 memHandle->buffersValidData[d]);
      total += devInfo[d].needToWrite[a]->total();
    }
  }

  // Compute the data we need to read from others devices
  for (unsigned d=0; d<nbDevices; d++) {
    for (unsigned a=0; a<nbGlobals; a++) {
      unsigned pos = globalsPos[a];

      MemoryHandleOptim *memHandle =
	static_cast<MemoryHandleOptim *>(memArgs[pos]);

      // Compute the intervals of data we need to write and we don't have in
      // the local buffer (needTowrite \ localValidDatas)
      ListInterval *remainsToRead =
	ListInterval::difference(*devInfo[d].needToWrite[a],
				 *memHandle->localValidData);

      // Read the data we don't have from others devices to the local buffer
      // So for each device :
      for (unsigned d2=0; d2<nbDevices; d2++) {
	// Compute the intervals of data it has
	ListInterval *intersection =
	  ListInterval::intersection(*remainsToRead,
				     memHandle->buffersValidData[d2]);

	// Append to needToRead
	devInfo[d2].needToRead[a].myUnion(*intersection);

	// Remove from remainsToRead
	remainsToRead->difference(*intersection);

	// Validate the data read on the local buffer
	memHandle->localValidData->myUnion(*intersection);

	delete intersection;
      }

      delete remainsToRead;
    }
  }

  // Read the needed data
  for (unsigned d=0; d<nbDevices; d++) {
    for (unsigned a=0; a<nbGlobals; a++) {
      unsigned pos = globalsPos[a];

      MemoryHandleOptim *memHandle =
	static_cast<MemoryHandleOptim *>(memArgs[pos]);

      ListInterval *needToRead = &devInfo[d].needToRead[a];

      // Read the needed data from the device to the local buffer
      for (unsigned i=0; i<needToRead->mList.size(); i++) {
	cl_int err;
	size_t offset = needToRead->mList[i].lb;
	size_t cb = needToRead->mList[i].hb -
	  needToRead->mList[i].lb + 1;

	DEBUG("log", logger->logDtoH(d, memHandle->id, needToRead->mList[i]););

	err = real_clEnqueueReadBuffer(devInfo[d].queue,
				       memHandle->mBuffers[d],
				       CL_TRUE,
				       offset,
				       cb,
				       (char *) memHandle->mLocalPtr + offset,
				       0, NULL, NULL);

	clCheck(err, __FILE__, __LINE__);
      }
    }
  }

  return total;
}

void
BufferManagerOptim::writeDeviceNo(DeviceInfo *devInfo, unsigned d,
				  unsigned nbGlobals, unsigned *globalsPos,
				  MemoryHandle **memArgs) {
  for (unsigned a=0; a<nbGlobals; a++) {
    unsigned pos = globalsPos[a];

    MemoryHandleOptim *memHandle =
      static_cast<MemoryHandleOptim *>(memArgs[pos]);

    ListInterval *needToWrite = devInfo[d].needToWrite[a];

    // Write the needed data from the local buffer to the device
    for (unsigned i=0; i<needToWrite->mList.size(); i++) {
      cl_int err;
      size_t offset = needToWrite->mList[i].lb;
      size_t cb = needToWrite->mList[i].hb -
	needToWrite->mList[i].lb + 1;
      cl_event event;
      devInfo[d].commEvents.push_back(event);

      DEBUG("log",
	    logger->logHtoD(d, memHandle->id, needToWrite->mList[i]);
	    );

      err = real_clEnqueueWriteBuffer(devInfo[d].queue,
				      memHandle->mBuffers[d],
				      CL_FALSE,
				      offset,
				      cb,
				      (char *) memHandle->mLocalPtr +
				      offset,
				      0, NULL,
				      &devInfo[d].commEvents
				      [devInfo[d].commEvents.size()-1]);

      clCheck(err, __FILE__, __LINE__);
    }

    // Validate the data written on the device
    memHandle->buffersValidData[d].myUnion(*needToWrite);

    delete needToWrite;
  }
}

void
BufferManagerOptim::readDevices(DeviceInfo *devInfo, unsigned nbDevices,
				unsigned nbGlobals, unsigned *globalsPos,
				bool *argIsMerge,
				MemoryHandle **memArgs) {
  // Do not read any data, just unvalidate the data written the kernel on
  // others buffers.
  for (unsigned a=0; a<nbGlobals; a++) {
    unsigned pos = globalsPos[a];

    MemoryHandleOptim *memHandle =
      static_cast<MemoryHandleOptim *>(memArgs[pos]);

    if (argIsMerge[globalsPos[a]]) {
      for (unsigned d=0; d<nbDevices; d++) {
	memHandle->buffersValidData[d].clear();
      }
      memHandle->localValidData->clear();
      memHandle->localValidData->add(Interval(0, memHandle->mSize-1));
      continue;
    }

    for (unsigned d=0; d<nbDevices; d++) {

      ListInterval &dataWritten = devInfo[d].dataWritten[a];

      for (unsigned d2=0; d2<nbDevices; d2++) {
	if (d == d2)
	  continue;
	memHandle->buffersValidData[d2].difference(dataWritten);
      }
      memHandle->localValidData->difference(dataWritten);
    }
  }
}

void
BufferManagerOptim::fakeReadDevices(DeviceInfo *devInfo, unsigned nbDevices,
				    unsigned nbGlobals, unsigned *globalsPos,
				    bool *argIsMerge,
				    MemoryHandle **memArgs) {
  // Do not read any data, just unvalidate the data written the kernel on
  // others buffers.
  for (unsigned a=0; a<nbGlobals; a++) {
    unsigned pos = globalsPos[a];

    MemoryHandleOptim *memHandle =
      static_cast<MemoryHandleOptim *>(memArgs[pos]);

    for (unsigned d=0; d<nbDevices; d++) {

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

      ListInterval &dataWritten = devInfo[d].dataWritten[a];

      for (unsigned d2=0; d2<nbDevices; d2++) {
	if (d == d2)
	  continue;
	memHandle->buffersValidData[d2].difference(dataWritten);
      }
      memHandle->localValidData->difference(dataWritten);
    }
  }
}

void
BufferManagerOptim::readDevicesMerge(unsigned nbDevices,
				     unsigned nbWrittenArgs,
				     unsigned *writtenArgsPos,
				     MemoryHandle **memArgs) {
  // For each written args
  for (unsigned a=0; a<nbWrittenArgs; a++) {
    unsigned pos = writtenArgsPos[a];

    MemoryHandleOptim *memHandle =
      static_cast<MemoryHandleOptim *>(memArgs[pos]);

    // Invalidate devices buffers.
    for (unsigned d=0; d<nbDevices; d++)
      memHandle->buffersValidData[d].clear();

    // Validate local buffer.
    memHandle->localValidData->clear();
    memHandle->localValidData->add(Interval(0, memHandle->mSize-1));
  }
}
