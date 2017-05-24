#include <Dispatch/OpenCLFunctions.h>
#include <Handle/ContextHandle.h>
#include <Options.h>
#include <Queue/DeviceLFQueue.h>
#include <Queue/DevicePthreadQueue.h>
#include <Utils/Debug.h>
#include <Utils/Utils.h>

#include <cassert>
#include <iostream>

namespace libsplit {

  ContextHandle::ContextHandle() {

    nbDevices = optDeviceSelection.size() / 2;

    if (optPerPlatform)
      createOneCtxtPerPlatform(optDeviceSelection.data());
    else
      createOneCtxtPerDevice(optDeviceSelection.data());

    // Create one command queue for each device
    queues = new DeviceQueue *[nbDevices];

    if (optLockFreeQueue) {
      for (cl_uint i=0; i<nbDevices; ++i)
	queues[i] = new DeviceLFQueue(getContext(i), getDevice(i),
				      optDeviceSelection[i*2+1]);
    } else {
      for (cl_uint i=0; i<nbDevices; ++i)
	queues[i] = new DevicePthreadQueue(getContext(i), getDevice(i),
					   optDeviceSelection[i*2+1]);
    }
  }

  ContextHandle::~ContextHandle() {
    cl_int err = CL_SUCCESS;

    for (unsigned i=0; i<nbContexts; i++) {
      err |= real_clReleaseContext(contexts[i]);
      clCheck(err, __FILE__, __LINE__);
    }

    delete[] contexts;
    delete[] devices;
    delete[] queues;
  }

  cl_context
  ContextHandle::getContext(unsigned i) {
    return dev2ContextMap[devices[i]];
  }

  cl_device_id
  ContextHandle::getDevice(unsigned i) {
    return devices[i];
  }

  unsigned
  ContextHandle::getNbDevices() const {
    return nbDevices;
  }

  DeviceQueue *
  ContextHandle::getQueueNo(unsigned i) {
    return queues[i];
  }

  void
  ContextHandle::createOneCtxtPerDevice(unsigned *platformDevices) {
    cl_int err;

    nbContexts = nbDevices;

    // Get nb platforms
    cl_uint machineNbPf;
    err = real_clGetPlatformIDs(0, NULL, &machineNbPf);
    clCheck(err, __FILE__, __LINE__);

    // Get platformIds
    cl_platform_id machinePfs[machineNbPf];
    err = clGetPlatformIDs(machineNbPf, machinePfs, NULL);
    clCheck(err, __FILE__, __LINE__);

    contexts = new cl_context[nbDevices];
    devices = new cl_device_id[nbDevices];

    for (unsigned i=0; i<nbDevices; ++i) {
      unsigned pfId = platformDevices[i*2];
      unsigned devId = platformDevices[i*2 + 1];

      if (pfId >= machineNbPf) {
	std::cerr << "ContextHandle: Error only " << machineNbPf
		  << " platforms found.\n";
      }

      cl_platform_id currentPf = machinePfs[pfId];

      // Get nb devices
      cl_uint pfNbDevices;
      real_clGetDeviceIDs(currentPf, CL_DEVICE_TYPE_ALL, 0, NULL,
		     &pfNbDevices);
      clCheck(err, __FILE__, __LINE__);

      if (platformDevices[i*2+1] >= pfNbDevices) {
	std::cerr << "ContextHandle: Error platform no " << platformDevices[i*2]
		  << " has only " << pfNbDevices << "\n";
	exit(EXIT_FAILURE);
      }

      // Get devices
      cl_device_id pfDevices[pfNbDevices];
      err = real_clGetDeviceIDs(currentPf, CL_DEVICE_TYPE_ALL, pfNbDevices,
			   pfDevices, NULL);
      clCheck(err, __FILE__, __LINE__);

      devices[i] = pfDevices[devId];

      contexts[i] =  real_clCreateContext(NULL, 1, &devices[i], NULL, NULL,
					   &err);
      dev2ContextMap[devices[i]] = contexts[i];
      clCheck(err, __FILE__, __LINE__);
    }
  }

  void
  ContextHandle::createOneCtxtPerPlatform(unsigned *platformDevices) {
    cl_int err;

    // Get nb platforms
    cl_uint machineNbPf;
    err = real_clGetPlatformIDs(0, NULL, &machineNbPf);
    clCheck(err, __FILE__, __LINE__);

    // Get platformIds
    cl_platform_id machinePfs[machineNbPf];
    err = real_clGetPlatformIDs(machineNbPf, machinePfs, NULL);
    clCheck(err, __FILE__, __LINE__);

    devices = new cl_device_id[nbDevices];

    // Compute the number of devices per platform.
    unsigned pfNbRequired[machineNbPf];
    for (unsigned i=0; i<machineNbPf; i++)
      pfNbRequired[i] = 0;

    for (unsigned i=0; i<nbDevices; ++i) {
      unsigned pfId = platformDevices[i*2];

      if (pfId >= machineNbPf) {
	std::cerr << "ContextHandle: Error only " << machineNbPf
		  << " platforms found.\n";
	exit(EXIT_FAILURE);
      }

      pfNbRequired[pfId]++;
    }

    // Count the number of contexts required and alloc contexts.
    nbContexts = 0;
    for (unsigned p=0; p<machineNbPf; p++) {
      if (pfNbRequired[p])
	nbContexts++;
    }

    contexts = new cl_context[nbContexts];
    unsigned ctxtId = 0;

    // For each platform, create a context if the number of device is > 0
    for (unsigned p=0; p<machineNbPf; p++) {
      if (!pfNbRequired[p])
	continue;

      cl_platform_id currentPf = machinePfs[p];

      // Get nb devices
      cl_uint pfNbDevices;
      err = real_clGetDeviceIDs(currentPf, CL_DEVICE_TYPE_ALL, 0, NULL,
			   &pfNbDevices);
      clCheck(err, __FILE__, __LINE__);

      // Get devices
      cl_device_id pfDevices[pfNbDevices];
      err = real_clGetDeviceIDs(currentPf, CL_DEVICE_TYPE_ALL, pfNbDevices,
			   pfDevices, NULL);
      clCheck(err, __FILE__, __LINE__);

      // Get required devices
      cl_device_id requiredDevices[pfNbRequired[p]];

      unsigned idx = 0;

      for (unsigned i=0; i<nbDevices; ++i) {
	unsigned pfId = platformDevices[i*2];

	if (pfId != p)
	  continue;

	unsigned devId = platformDevices[i*2 + 1];

	if (devId > pfNbDevices) {
	  std::cerr << "ContextHandle: Error platform no "
		    << platformDevices[i*2]
		    << " has only " << pfNbDevices << "\n";
	  exit(EXIT_FAILURE);
	}

	requiredDevices[idx++] = pfDevices[devId];
	devices[i] = pfDevices[devId];
      }

      if (idx != pfNbRequired[p]) {
	std::cerr << "idx=" << idx << " != nbRequired=" << pfNbRequired[p]
		  << "\n";
	exit(EXIT_FAILURE);
      }

      // Create context
      cl_context context = real_clCreateContext(NULL,
						pfNbRequired[p],
						requiredDevices,
						NULL, NULL, &err);
      clCheck(err, __FILE__, __LINE__);

      contexts[ctxtId++] = context;

      for (unsigned i=0; i<nbDevices; ++i) {
	unsigned pfId = platformDevices[i*2];
	if (pfId == p) {
	  dev2ContextMap[devices[i]] = context;
	}
      }
    }
  }

  unsigned
  ContextHandle::getNbContexts() const {
    return nbContexts;
  }

  cl_context
  ContextHandle::getContextFromDevice(cl_device_id d) {
    assert(dev2ContextMap.find(d) != dev2ContextMap.end());
    return dev2ContextMap[d];
  }

};
