#include "ListInterval.h"

#include <CL/opencl.h>

#ifndef DEVICEINFO_H
#define DEVICEINFO_H

struct DeviceInfo {
  DeviceInfo();
  ~DeviceInfo();

  unsigned deviceIndex; // index im mem map buffer and also cl_kernel

  cl_command_queue queue; // Command queue for this device

  ListInterval *dataRequired; // one per global argument
  ListInterval *dataWritten; // one per global argument

  ListInterval *needToRead; // one per global argument
  // Missing data we need to read from device to host.
  ListInterval **needToWrite; // one per global argument
  // Missing data we need to write from host de device.

  unsigned nbKernels; // Number of kernel on this device
  unsigned *kernelsIndex; // Index of each kernel in the analysis
  cl_event *kernelsEvent;

  // Sub-kernels NDRanges
  size_t **mGlobalWorkSize; // Global work size of each sub-kernel
  size_t **mGlobalWorkOffset; // Global work offset of each sub-kernel

  std::vector<cl_event> commEvents;

  char padding[256];
};

#endif /* DEVICEINFO_H */
