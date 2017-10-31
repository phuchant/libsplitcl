#include <BufferManager.h>
#include <Options.h>
#include <Queue/DeviceQueue.h>
#include <Utils/Debug.h>

#include <cassert>
#include <cstring>

namespace libsplit {
  BufferManager::BufferManager(bool delayedWrite)
    : delayedWrite(delayedWrite) {
    noMemcpy = optNoMemcpy;
  }

  BufferManager::~BufferManager() {}

  void
  BufferManager::read(MemoryHandle *m, cl_bool blocking, size_t offset,
		      size_t size, void *ptr) {
    (void) blocking;

    ListInterval dataRequired;
    dataRequired.add(Interval(offset, offset+size-1));

    // Compute the intervals of data that are valid on the host.
    ListInterval *intersection =
      ListInterval::intersection(dataRequired, m->hostValidData);

    // Copy them from the host buffer to the user ptr.
    if (noMemcpy && ptr == ((char *) m->mLocalBuffer) + offset) {
      // Do nothing.
    } else {
      for (unsigned id=0; id<intersection->mList.size(); id++) {
	size_t myoffset = intersection->mList[id].lb;
	size_t mycb = intersection->mList[id].hb - intersection->mList[id].lb
	  + 1;
	memcpy((char *) ptr+myoffset-offset, (char *)
	       m->mLocalBuffer + myoffset,
	       mycb);
      }
    }

    // Compute the intervals of data that are not valid on the local buffer.
    dataRequired.difference(*intersection);
    delete intersection;

    // Read them from devices buffer to the user pointer.

    // For each device compute the intervals of data it owns
    // and read them from the device to the host pointer
    ListInterval *toReadDevice[m->mNbBuffers];
    size_t total = 0;

    for (unsigned d=0; d<m->mNbBuffers; d++) {
      toReadDevice[d] =
	ListInterval::intersection(dataRequired, m->devicesValidData[d]);

      total += toReadDevice[d]->total();
      dataRequired.difference(*toReadDevice[d]);
    }

    assert(dataRequired.mList.empty());

    for (unsigned d=0; d<m->mNbBuffers; d++) {
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      for (unsigned id=0; id<toReadDevice[d]->mList.size(); id++) {
	size_t myoffset = toReadDevice[d]->mList[id].lb;
	size_t mycb =
	  toReadDevice[d]->mList[id].hb - toReadDevice[d]->mList[id].lb + 1;

	DEBUG("tranfers",
	      std::cerr << "enqueueRead: reading [" << myoffset << "," << myoffset+mycb-1
	      << "] from dev " << d << " for buffer " << m->id << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   myoffset,
			   mycb,
			   (char *) ptr + myoffset - offset,
			   0, NULL, NULL);
      }
    }

    // FIXME: It is always blocking because a call to clEnqueueReadBuffer
    // returns a fake event. Thus there is no way for the user to know
    // when the command has been executed.
    for (unsigned d=0; d<m->mNbBuffers; d++) {
      DeviceQueue *queue = m->mContext->getQueueNo(d);
      queue->finish();
    }

    // If no memcpy and ptr == localptr, we can validate the data read on the
    // local buffer
    if (noMemcpy && ptr == ((char *) m->mLocalBuffer) + offset) {
      for (unsigned d=0; d<m->mNbBuffers; d++) {
	m->hostValidData.myUnion(*toReadDevice[d]);
      }
    }

    for (unsigned d=0; d<m->mNbBuffers; d++)
      delete toReadDevice[d];
  }

  void
  BufferManager::write(MemoryHandle *m, cl_bool blocking, size_t offset,
		       size_t size, const void *ptr) {
    (void) blocking;

    // Update max used size
    size_t total_cb = offset + size;
    m->mMaxUsedSize = total_cb > m->mMaxUsedSize ? total_cb : m->mMaxUsedSize;

    if (!delayedWrite) {
      for (unsigned d=0; d<m->mNbBuffers; d++) {
	DeviceQueue *queue = m->mContext->getQueueNo(d);
	queue->enqueueWrite(m->mBuffers[d], CL_FALSE, offset, size, ptr,
			    0, NULL, NULL);
      }

      // Update valid data.
      Interval inter(offset, offset+size-1);
      m->hostValidData.remove(inter);

      for (unsigned i=0; i<m->mNbBuffers; i++)
	m->devicesValidData[i].add(inter);

      return;
    }

    if (noMemcpy) {
      // FIXME: Doesn't work when offset is not zero.
      m->mLocalBuffer = (void *) ptr;
    } else {
      // Copy datas
      memcpy((char *) m->mLocalBuffer + offset, ptr, size);
    }

    // Update valid data.
    Interval inter(offset, offset+size-1);
    m->hostValidData.add(inter);

    for (unsigned i=0; i<m->mNbBuffers; i++)
      m->devicesValidData[i].remove(inter);
  }

  void
  BufferManager::copy(MemoryHandle *src, MemoryHandle *dst, size_t src_offset,
		      size_t dst_offset, size_t size) {
    // Update max used size
    size_t total_cb = dst_offset + size;
    size_t dst_max = dst->mMaxUsedSize;
    dst->mMaxUsedSize = total_cb > dst_max ? total_cb : dst_max;


    // First ensure that local buffer contains valid data for the required
    // interval.

    ListInterval missing;
    Interval src_interval(src_offset, src_offset+size-1);
    missing.add(src_interval);

    ListInterval *intersection =
      ListInterval::intersection(missing, src->hostValidData);

    missing.difference(*intersection);
    delete intersection;

    // Get needed data from devices.
    if (missing.mList.size() > 0) {
      for (unsigned d=0; d<src->mNbBuffers; d++) {
	DeviceQueue *queue = src->mContext->getQueueNo(d);

	intersection = ListInterval::intersection(missing,
						  src->devicesValidData[d]);

	for (unsigned id=0; id<intersection->mList.size(); id++) {
	  size_t myoffset = intersection->mList[id].lb;
	  size_t mycb =
	    intersection->mList[id].hb - intersection->mList[id].lb + 1;

	  DEBUG("tranfers",
		std::cerr << "enqueueCopy: reading [" << myoffset << "," << myoffset+mycb-1
		<< "] from dev " << d << "\n");

	  queue->enqueueRead(src->mBuffers[d],
			     CL_FALSE,
			     myoffset,
			     mycb,
			     (char *) src->mLocalBuffer + myoffset,
			     0, NULL, NULL);
	}

	missing.difference(*intersection);
	delete intersection;
      }

      assert(missing.mList.empty());

      for (unsigned d=0; d<src->mNbBuffers; d++) {
	DeviceQueue *queue = src->mContext->getQueueNo(d);
	queue->finish();
      }
    }

    src->hostValidData.add(src_interval);

    // Then copy to dst buffer and validate the interval on local and invalidate
    // on device.
    memcpy((char *) dst->mLocalBuffer + dst_offset,
	   (char *) src->mLocalBuffer + src_offset,
	   size);

    Interval dstInterval(dst_offset, dst_offset+size-1);

    dst->hostValidData.add(dstInterval);

    for (unsigned d=0; d<dst->mNbBuffers; d++)
      dst->devicesValidData[d].remove(dstInterval);
  }

  void *
  BufferManager::map(MemoryHandle *m, cl_bool blocking_map, cl_map_flags flags,
		     size_t offset, size_t cb) {
    (void) blocking_map;

    void *address = (void *) (((char *) m->mLocalBuffer) + offset);
    if (map_entries.find(address) != map_entries.end()) {
      std::cerr << "Error: address already mapped !\n";
      exit(EXIT_FAILURE);
    }


    // If the region is mapped for reading we have to get the missing valid data
    // on the host.
    if (flags & CL_MAP_READ) {
      ListInterval dataRequired;
      dataRequired.add(Interval(offset, offset+cb-1));
      ListInterval *missing
	= ListInterval::difference(dataRequired, m->hostValidData);
      if (missing->total() == 0) {
	delete missing;
	return address;
      }

      for (unsigned d=0; d<m->mNbBuffers; d++) {
	ListInterval *toRead =
	  ListInterval::intersection(*missing, m->devicesValidData[d]);
	DeviceQueue *queue = m->mContext->getQueueNo(d);
	for (unsigned i=0; i<toRead->mList.size(); i++) {
	  size_t myoffset = toRead->mList[i].lb;
	  size_t mycb = toRead->mList[i].hb - myoffset + 1;
	  queue->enqueueRead(m->mBuffers[d], CL_FALSE, myoffset, mycb,
			     (char *) m->mLocalBuffer + myoffset,
			     0, NULL, NULL);
	}
	missing->difference(*toRead);
	delete toRead;
	if (missing->total() == 0)
	  break;
      }

      //      assert(missing->total() == 0);
      delete missing;

      for (unsigned d=0; d<m->mNbBuffers; d++) {
	DeviceQueue *queue = m->mContext->getQueueNo(d);
	queue->finish();
      }
    }

    map_entries.emplace(address, map_entry(offset, cb, flags & CL_MAP_WRITE));

    return address;
  }

  void
  BufferManager::unmap(MemoryHandle *m, void *mapped_ptr) {
    auto I = map_entries.find(mapped_ptr);
    if (I == map_entries.end()) {
      std::cerr << "Error: unmap\n";
      exit(EXIT_FAILURE);
    }

    Interval inter(I->second.offset, I->second.offset+I->second.cb-1);
    map_entries.erase(I);

    if (!I->second.isWrite)
      return;



    for (unsigned d=0; d<m->mNbBuffers; d++)
      m->devicesValidData[d].remove(inter);
  }

  void
  BufferManager::fill(MemoryHandle *m, const void *pattern, size_t pattern_size,
		      size_t offset, size_t size) {
    Event events[m->mNbBuffers];

    // EnqueueFillBuffer for all devices.
    for (unsigned d=0; d<m->mNbBuffers; d++) {
      DeviceQueue *queue = m->mContext->getQueueNo(d);
      queue->enqueueFill(m->mBuffers[d], pattern, pattern_size, offset, size,
			 0, NULL, NULL);

    }

    // Update valid data.
    Interval inter(offset, offset+size-1);
    for (unsigned d=0; d<m->mNbBuffers; d++)
      m->devicesValidData[d].add(inter);
    m->hostValidData.remove(inter);

    // Wait for all events to be submitted.
    for (unsigned d=0; d<m->mNbBuffers; d++)
      m->mContext->getQueueNo(d)->finish();

  }


  void
  BufferManager::computeIndirectionTransfers(std::vector<BufferIndirectionRegion> &regions,
					     std::vector<DeviceBufferRegion> &D2HTransferList) {
    for (unsigned i=0; i<regions.size(); i++) {
      MemoryHandle *m = regions[i].m;
      // Restrict region to buffer size
      if (regions[i].lb < 0)
	regions[i].lb = 0;
      if (regions[i].hb + regions[i].cb -1 >= m->mSize) {
	regions[i].hb = m->mSize - regions[i].cb;
      }

      size_t lb = regions[i].lb;
      size_t hb = regions[i].hb;
      size_t cb = regions[i].cb;

      // Compute data required.
      ListInterval required;
      required.add(Interval(lb, lb+cb-1));
      required.add(Interval(hb, hb+cb-1));

      // Compute data missing on the host.
      ListInterval *missing = ListInterval::difference(required, m->hostValidData);
      if (missing->total() == 0) {
	delete missing;
	continue;
      }

      // Compute D2H tranfers required to get the data missing back to the host.
      for (unsigned d=0; d<m->mNbBuffers; d++) {
	ListInterval *intersection =
	  ListInterval::intersection(m->devicesValidData[d], *missing);
	if (intersection->total() == 0) {
	  delete intersection;
	  continue;
	}

	D2HTransferList.push_back(DeviceBufferRegion(m, d, *intersection));
	missing->difference(*intersection);
	delete intersection;
	if (missing->total() == 0)
	  break;
      }

      if (missing->total() != 0) {
	std::cerr << "missing : ";
	missing->debug();
	std::cerr << "\n";
	std::cerr << "buffer size : " << m->mSize << "\n";
	std::cerr << "indirectionId : " << regions[i].indirectionId << "\n";
      }
      assert(missing->total() == 0);
      delete missing;
    }
  }

  void
  BufferManager::computeTransfers(std::vector<DeviceBufferRegion> &
				  dataRequired,
				  std::vector<DeviceBufferRegion> &
				  dataWritten,
				  std::vector<DeviceBufferRegion> &
				  dataWrittenOr,
				  std::vector<DeviceBufferRegion> &
				  dataWrittenAtomicSum,
				  std::vector<DeviceBufferRegion> &
				  dataWrittenAtomicMin,
				  std::vector<DeviceBufferRegion> &
				  dataWrittenAtomicMax,
				  std::vector<DeviceBufferRegion> &
				  D2HTransferList,
				  std::vector<DeviceBufferRegion> &
				  H2DTransferList,
				  std::vector<DeviceBufferRegion> &
				  OrD2HTransferList,
				  std::vector<DeviceBufferRegion> &
				  AtomicSumD2HTransferList,
				  std::vector<DeviceBufferRegion> &
				  AtomicMinD2HTransferList,
				  std::vector<DeviceBufferRegion> &
				  AtomicMaxD2HTransferList) {

    // If data written is undefined, we consider that the whole buffer is
    // written.
    for (unsigned i=0; i<dataWritten.size(); i++) {
      MemoryHandle *m = dataWritten[i].m;
      if (dataWritten[i].region.isUndefined()) {
	dataWritten[i].region.clearList();
	size_t cb = m->mSize;
	dataWritten[i].region.add(Interval(0, cb-1));
      }
    }

    // If atomic sum region is undefined, we consider that the whole buffer is
    // written.
    for (unsigned i=0; i<dataWrittenAtomicSum.size(); i++) {
      MemoryHandle *m = dataWrittenAtomicSum[i].m;
      if (dataWrittenAtomicSum[i].region.isUndefined()) {
	dataWrittenAtomicSum[i].region.clearList();
	size_t cb = m->mSize;
	dataWrittenAtomicSum[i].region.add(Interval(0, cb-1));
      }
    }

    // Compute a map of the written atomic sum region for each memory handle.
    std::map<MemoryHandle *, ListInterval> atomicSumHostRequiredData;
    for (unsigned i=0; i<dataWrittenAtomicSum.size(); i++) {
      MemoryHandle *m = dataWrittenAtomicSum[i].m;
      atomicSumHostRequiredData[m].myUnion(dataWrittenAtomicSum[i].region);
    }

    // Compute a map of the written atomic min region for each memory handle.
    std::map<MemoryHandle *, ListInterval> atomicMinHostRequiredData;
    for (unsigned i=0; i<dataWrittenAtomicMin.size(); i++) {
      MemoryHandle *m = dataWrittenAtomicMin[i].m;
      atomicMinHostRequiredData[m].myUnion(dataWrittenAtomicMin[i].region);
    }

    // Compute a map of the written atomic max region for each memory handle.
    std::map<MemoryHandle *, ListInterval> atomicMaxHostRequiredData;
    for (unsigned i=0; i<dataWrittenAtomicMax.size(); i++) {
      MemoryHandle *m = dataWrittenAtomicMax[i].m;
      atomicMaxHostRequiredData[m].myUnion(dataWrittenAtomicMax[i].region);
    }

    // Compute D2H and H2D Transfers for data required by subkernels
    for (unsigned i=0; i<dataRequired.size(); i++) {
      MemoryHandle *m = dataRequired[i].m;
      unsigned d = dataRequired[i].devId;

      // If data required is undefined, we consider that the whole buffer is
      // required.
      if (dataRequired[i].region.isUndefined()) {
	dataRequired[i].region.clearList();
	size_t cb = m->isRO ? m->mMaxUsedSize : m->mSize;
	dataRequired[i].region.add(Interval(0, cb-1));
      }

      // Restrict the region to buffer size.
      ListInterval bufferRegion; bufferRegion.add(Interval(0, m->mSize-1));
      ListInterval *restrictRegion =
	ListInterval::intersection(dataRequired[i].region, bufferRegion);
      dataRequired[i].region.clearList();
      dataRequired[i].region.myUnion(*restrictRegion);
      delete restrictRegion;

      // Compute the data missing on the device (H2D transfer).
      ListInterval *missing = ListInterval::difference(dataRequired[i].region,
						       m->devicesValidData[d]);

      if (missing->total() == 0) {
	delete missing;
	continue;
      }

      H2DTransferList.push_back(DeviceBufferRegion(m, d, *missing));

      // Compute the data missing on the host.
      ListInterval *hostMissing =
	ListInterval::difference(*missing, m->hostValidData);
      delete missing;

      ListInterval *atomicSumHostMissing =
	ListInterval::difference(atomicSumHostRequiredData[m],
				 m->hostValidData);

      hostMissing->myUnion(*atomicSumHostMissing);
      delete atomicSumHostMissing;

      ListInterval *atomicMinHostMissing =
	ListInterval::difference(atomicMinHostRequiredData[m],
				 m->hostValidData);

      hostMissing->myUnion(*atomicMinHostMissing);
      delete atomicMinHostMissing;

      ListInterval *atomicMaxHostMissing =
	ListInterval::difference(atomicMaxHostRequiredData[m],
				 m->hostValidData);

      hostMissing->myUnion(*atomicMaxHostMissing);
      delete atomicMaxHostMissing;

      if (hostMissing->total() == 0) {
	delete hostMissing;
	continue;
      }

      // Compute D2H transfers required to get the data missing back to the
      // host.
      for (unsigned d2=0; d2<m->mNbBuffers;d2++) {
	if (d2 == d)
	  continue;

	ListInterval *intersection =
	  ListInterval::intersection(m->devicesValidData[d2], *hostMissing);
	if (intersection->total() == 0) {
	  delete intersection;
	  continue;
	}

	D2HTransferList.push_back(DeviceBufferRegion(m, d2, *intersection));
	hostMissing->difference(*intersection);
	delete intersection;
	if (hostMissing->total() == 0)
	  break;
      }

      // assert(hostMissing->total() == 0);
      delete hostMissing;
    }

    // Compute Transfers to data written atomic sum
    for (unsigned i=0; i<dataWrittenAtomicSum.size(); i++) {
      MemoryHandle *m = dataWrittenAtomicSum[i].m;
      unsigned d = dataWrittenAtomicSum[i].devId;

      // dataWritten can be undefined
      DeviceBufferRegion region(m, d, dataWrittenAtomicSum[i].region,
				malloc(dataWrittenAtomicSum[i].region.total()));
      AtomicSumD2HTransferList.push_back(region);
    }

    // Compute Transfers to data written atomic max
    for (unsigned i=0; i<dataWrittenAtomicMax.size(); i++) {
      MemoryHandle *m = dataWrittenAtomicMax[i].m;
      unsigned d = dataWrittenAtomicMax[i].devId;

      assert(!dataWrittenAtomicMax[i].region.isUndefined());

      DeviceBufferRegion region(m, d, dataWrittenAtomicMax[i].region,
				malloc(dataWrittenAtomicMax[i].region.total()));
      AtomicMaxD2HTransferList.push_back(region);
    }

    // Compute Transfers to data written atomic min
    for (unsigned i=0; i<dataWrittenAtomicMin.size(); i++) {
      MemoryHandle *m = dataWrittenAtomicMin[i].m;
      unsigned d = dataWrittenAtomicMin[i].devId;

      assert(!dataWrittenAtomicMin[i].region.isUndefined());

      DeviceBufferRegion region(m, d, dataWrittenAtomicMin[i].region,
				malloc(dataWrittenAtomicMin[i].region.total()));
      AtomicMinD2HTransferList.push_back(region);
    }

    // Compute Transfers for data written_or
    for (unsigned i=0; i<dataWrittenOr.size(); i++) {
      MemoryHandle *m = dataWrittenOr[i].m;
      unsigned d = dataWrittenOr[i].devId;

      assert(!dataWrittenOr[i].region.isUndefined());

      DeviceBufferRegion region(m, d, dataWrittenOr[i].region,
				malloc(dataWrittenOr[i].region.total()));
      OrD2HTransferList.push_back(region);
    }
  }
};
