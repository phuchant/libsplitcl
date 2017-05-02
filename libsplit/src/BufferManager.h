#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H

#include <Handle/MemoryHandle.h>
#include <ListInterval.h>

#include <utility>
#include <vector>

namespace libsplit {
  struct DeviceBufferRegion {
    DeviceBufferRegion(MemoryHandle *m, unsigned devId, ListInterval region)
      : m(m), devId(devId), region(region) {}
    ~DeviceBufferRegion() {}

    MemoryHandle *m;
    unsigned devId;
    ListInterval region;
  };

  class BufferManager {
  public:

    BufferManager();
    virtual ~BufferManager();

    void read(MemoryHandle *m, cl_bool blocking, size_t offset, size_t size,
	      void *ptr);

    void write(MemoryHandle *m, cl_bool blocking, size_t offset, size_t size,
	       const void *ptr);

    void copy(MemoryHandle *src, MemoryHandle *dst, size_t src_offset,
	      size_t dst_offset, size_t size);

    void computeTransfers(std::vector<DeviceBufferRegion> &dataRequired,
			  std::vector<DeviceBufferRegion> &D2HTransferList,
			  std::vector<DeviceBufferRegion> &H2DTransferList);

    bool noMemcpy;
    bool delayedWrite;
  };
};

#endif /* BUFFERMANAGER_H */
