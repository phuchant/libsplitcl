#ifndef BUFFERMANAGEROPTIM_H
#define BUFFERMANAGEROPTIM_H

#include "BufferManager.h"
#include "MemoryHandleOptim.h"

class BufferManagerOptim : public BufferManager {
public:
  BufferManagerOptim();
  virtual ~BufferManagerOptim();

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

  virtual void readLocalMissing(DeviceInfo *devInfo, unsigned nbDevices,
				MemoryHandleOptim *memHandle,
				ListInterval *required);
};

#endif /* BUFFERMANAGEROPTIM_H */
