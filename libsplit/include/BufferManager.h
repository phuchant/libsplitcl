#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H

#include "ContextHandle.h"
#include "DeviceInfo.h"
#include "MemoryHandle.h"

class BufferManager {

public:

  enum type {
    SIMPLE,
    OPTIM
  };

  BufferManager() {}

  virtual ~BufferManager() {};

  virtual MemoryHandle *createMemoryHandle(ContextHandle *context,
					   cl_mem_flags flags, size_t size,
					   void *host_ptr) = 0;

  // Write data for a single kernel onto a single device.
  virtual void writeSingle(DeviceInfo *devInfo, unsigned devId,
			   unsigned nbGlobals, unsigned *globalsPos,
			   MemoryHandle **memArgs) = 0;

  // Read data for a single kernel from a single device.
  virtual void readSingle(DeviceInfo *devInfo, unsigned devId,
			  unsigned nbGlobals, unsigned *globalsPos,
			  MemoryHandle **memArgs) = 0;

  // Compute communications and read missing local data from devices.
  virtual size_t computeAndReadMissing(DeviceInfo *devInfo,
				       unsigned nbDevices,
				       unsigned nbGlobals,
				       unsigned *globalsPos,
				       MemoryHandle **memArgs) = 0;

  // Write data onto the device no.
  virtual void writeDeviceNo(DeviceInfo *devInfo, unsigned d,
			     unsigned nbGlobals, unsigned *globalsPos,
			     MemoryHandle **memArgs) = 0;

  // Read data from the devices.
  virtual void readDevices(DeviceInfo *devInfo, unsigned nbDevices,
			   unsigned nbGlobals, unsigned *globalsPos,
			   bool *argIsMerge,
			   MemoryHandle **memArgs) = 0;

  // Read data from the devices.
  virtual void fakeReadDevices(DeviceInfo *devInfo, unsigned nbDevices,
			       unsigned nbGlobals, unsigned *globalsPos,
			       bool *argIsMerge,
			       MemoryHandle **memArgs) = 0;

  // Read data from the devices.
  virtual void readDevicesMerge(unsigned nbDevices, unsigned nbWrittenArgs,
				unsigned *writtenArgsPos,
				MemoryHandle **memArgs) = 0;
};

#endif /* BUFFERMANAGER_H */
