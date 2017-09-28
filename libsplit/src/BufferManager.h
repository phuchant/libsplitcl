#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H

#include <Handle/MemoryHandle.h>
#include <ListInterval.h>

#include <utility>
#include <vector>

namespace libsplit {
  struct DeviceBufferRegion {
    DeviceBufferRegion(MemoryHandle *m, unsigned devId, ListInterval &region,
		       void *tmp = NULL)
      : m(m), devId(devId), region(region), tmp(tmp) {}
    ~DeviceBufferRegion() {}

    MemoryHandle *m;
    unsigned devId;
    ListInterval region;
    void *tmp;
  };

  struct BufferIndirectionRegion {
    BufferIndirectionRegion(unsigned subkernelId,
			    unsigned indirectionId,
			    MemoryHandle *m,
			    size_t cb,
			    size_t lb,
			    size_t hb)
      : subkernelId(subkernelId), indirectionId(indirectionId), m(m),
	cb(cb), lb(lb), hb(hb) {}
    ~BufferIndirectionRegion() {}

    unsigned subkernelId;
    unsigned indirectionId;
    MemoryHandle *m;
    size_t cb;
    size_t lb;
    size_t hb;
    int lbValue;
    int hbValue;
  };

  class BufferManager {
  public:

    BufferManager(bool delayedWrite);
    virtual ~BufferManager();

    void read(MemoryHandle *m, cl_bool blocking, size_t offset, size_t size,
	      void *ptr);

    void write(MemoryHandle *m, cl_bool blocking, size_t offset, size_t size,
	       const void *ptr);

    void copy(MemoryHandle *src, MemoryHandle *dst, size_t src_offset,
	      size_t dst_offset, size_t size);

    void computeIndirectionTransfers(const std::vector<BufferIndirectionRegion> &regions,
				     std::vector<DeviceBufferRegion> &D2HTransferList);

    void computeTransfers(std::vector<DeviceBufferRegion> &dataRequired,
			  std::vector<DeviceBufferRegion> &dataWritten,
			  std::vector<DeviceBufferRegion> &dataWrittenOr,
			  std::vector<DeviceBufferRegion> &dataWrittenAtomicSum,
			  std::vector<DeviceBufferRegion> &dataWrittenAtomicMax,
			  std::vector<DeviceBufferRegion> &D2HTransferList,
			  std::vector<DeviceBufferRegion> &H2DTransferList,
			  std::vector<DeviceBufferRegion> &OrD2HTransferList,
			  std::vector<DeviceBufferRegion> &AtomicSumD2HTransferList,
			  std::vector<DeviceBufferRegion> &AtomicMaxD2HTransferList);

    bool noMemcpy;
    bool delayedWrite;
  };
};

#endif /* BUFFERMANAGER_H */
