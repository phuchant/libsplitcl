#ifndef BUFFERMANAGERSIMPLE_H
#define BUFFERMANAGERSIMPLE_H

#include "BufferManager.h"

class BufferManagerSimple : public BufferManager {
public:
  BufferManagerSimple();
  virtual ~BufferManagerSimple();

  virtual MemoryHandle *createMemoryHandle(ContextHandle *context,
					   cl_mem_flags flags, size_t size,
					   void *host_ptr);

  // Write data for a single kernel onto a single device.
  virtual void writeSingle(DeviceInfo *devInfo, unsigned devId,
			   unsigned nbGlobals, unsigned *globalsPos,
			   MemoryHandle **memArgs);

  // Read data for a single kernel from a single device.
  virtual void readSingle(DeviceInfo *devInfo, unsigned devId,
			  unsigned nbGlobals, unsigned *globalsPos,
			  MemoryHandle **memArgs);

  // Compute communications and read missing local data from devices.
  virtual size_t computeAndReadMissing(DeviceInfo *devInfo,
				       unsigned nbDevices,
				       unsigned nbGlobals,
				       unsigned *globalsPos,
				       MemoryHandle **memArgs);

  // Write data onto the device no.
  virtual void writeDeviceNo(DeviceInfo *devInfo, unsigned d,
			     unsigned nbGlobals, unsigned *globalsPos,
			     MemoryHandle **memArgs);

  // Read data from the devices.
  virtual void readDevices(DeviceInfo *devInfo, unsigned nbDevices,
			   unsigned nbGlobals, unsigned *globalsPos,
			   bool *argIsMerge,
			   MemoryHandle **memArgs);

  // Read data from the devices.
  virtual void fakeReadDevices(DeviceInfo *devInfo, unsigned nbDevices,
			       unsigned nbGlobals, unsigned *globalsPos,
			       bool *argIsMerge,
			       MemoryHandle **memArgs);

  virtual void readDevicesMerge(unsigned nbDevices, unsigned nbWrittenArgs,
				unsigned *writtenArgsPos,
				MemoryHandle **memArgs);
};

#endif /* BUFFERMANAGERSIMPLE_H */
