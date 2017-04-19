#include "ContextHandle.h"
#include <Debug.h>
#include "Globals.h"
#include "OpenCLFunctions.h"
#include <Options.h>
#include "Utils.h"

#include <iostream>

ContextHandle::ContextHandle() {

  mNbDevices = optDeviceSelection.size() / 2;

  // Get real opencl functions
  getOpenCLFunctions();

  cl_int err;

  if (optPerPlatform)
    createOneCtxtPerPlatform(optDeviceSelection.data());
  else
    createOneCtxtPerDevice(optDeviceSelection.data());

  // Create one command queue for each device
  mQueues = new cl_command_queue[mNbDevices];
  for (cl_uint i=0; i<mNbDevices; ++i) {
    mQueues[i] = real_clCreateCommandQueue(getContext(i),
					   getDevice(i),
					   CL_QUEUE_PROFILING_ENABLE,
					   &err);
    clCheck(err, __FILE__, __LINE__);
  }

}

ContextHandle::~ContextHandle() {
  DEBUG("log",
	logger->toDot("graph.dot");
	std::cerr << "Log graph written to file graph.dot\n";
	);

  delete[] mContexts;
  delete[] mDevContexts;
  delete[] mDevices;
  delete[] mQueues;
}

bool
ContextHandle::release() {
  if (--ref_count > 0)
    return false;

  cl_int err = CL_SUCCESS;

  for (unsigned i=0; i<mNbContexts; i++) {
    err |= real_clReleaseContext(mContexts[i]);
    clCheck(err, __FILE__, __LINE__);
  }

  for (unsigned i=0; i<mNbDevices; ++i) {
    err |= real_clReleaseCommandQueue(mQueues[i]);
    clCheck(err, __FILE__, __LINE__);
  }

  return true;
}

cl_context
ContextHandle::getContext(unsigned i) {
  return mDevContexts[i];
}

cl_device_id
ContextHandle::getDevice(unsigned i) {
  return mDevices[i];
}

unsigned
ContextHandle::getNbDevices() const {
  return mNbDevices;
}

cl_command_queue
ContextHandle::getQueueNo(unsigned i) {
  return mQueues[i];
}

void
ContextHandle::createOneCtxtPerDevice(unsigned *platformDevices) {
  cl_int err;

  mNbContexts = mNbDevices;

  // Get nb platforms
  cl_uint machineNbPf;
  err = clGetPlatformIDs(0, NULL, &machineNbPf);
  clCheck(err, __FILE__, __LINE__);

  // Get platformIds
  cl_platform_id machinePfs[machineNbPf];
  err = clGetPlatformIDs(machineNbPf, machinePfs, NULL);
  clCheck(err, __FILE__, __LINE__);

  mContexts = new cl_context[mNbDevices];
  mDevContexts = new cl_context[mNbDevices];
  mDevices = new cl_device_id[mNbDevices];

  for (unsigned i=0; i<mNbDevices; ++i) {
    unsigned pfId = platformDevices[i*2];
    unsigned devId = platformDevices[i*2 + 1];

    if (pfId >= machineNbPf) {
      std::cerr << "ContextHandle: Error only " << machineNbPf
		<< " platforms found.\n";
    }

    cl_platform_id currentPf = machinePfs[pfId];

    // Get nb devices
    cl_uint pfNbDevices;
    clGetDeviceIDs(currentPf, CL_DEVICE_TYPE_ALL, 0, NULL,
		   &pfNbDevices);
    clCheck(err, __FILE__, __LINE__);

    if (platformDevices[i*2+1] >= pfNbDevices) {
      std::cerr << "ContextHandle: Error platform no " << platformDevices[i*2]
		<< " has only " << pfNbDevices << "\n";
      exit(EXIT_FAILURE);
    }

    // Get devices
    cl_device_id pfDevices[pfNbDevices];
    err = clGetDeviceIDs(currentPf, CL_DEVICE_TYPE_ALL, pfNbDevices,
			 pfDevices, NULL);
    clCheck(err, __FILE__, __LINE__);

    mDevices[i] = pfDevices[devId];
    mContexts[i] =  real_clCreateContext(NULL, 1, &mDevices[i], NULL, NULL,
					 &err);
    mDevContexts[i] = mContexts[i];
    clCheck(err, __FILE__, __LINE__);
  }
}

void
ContextHandle::createOneCtxtPerPlatform(unsigned *platformDevices) {
  cl_int err;

  // Get nb platforms
  cl_uint machineNbPf;
  err = clGetPlatformIDs(0, NULL, &machineNbPf);
  clCheck(err, __FILE__, __LINE__);

  // Get platformIds
  cl_platform_id machinePfs[machineNbPf];
  err = clGetPlatformIDs(machineNbPf, machinePfs, NULL);
  clCheck(err, __FILE__, __LINE__);

  mDevContexts = new cl_context[mNbDevices];
  mDevices = new cl_device_id[mNbDevices];

  // Compute the number of devices per platform.
  unsigned pfNbRequired[machineNbPf];
  for (unsigned i=0; i<machineNbPf; i++)
    pfNbRequired[i] = 0;

  for (unsigned i=0; i<mNbDevices; ++i) {
    unsigned pfId = platformDevices[i*2];

    if (pfId >= machineNbPf) {
      std::cerr << "ContextHandle: Error only " << machineNbPf
		<< " platforms found.\n";
      exit(EXIT_FAILURE);
    }

    pfNbRequired[pfId]++;
  }

  // Count the number of contexts required and alloc mContexts.
  mNbContexts = 0;
  for (unsigned p=0; p<machineNbPf; p++) {
    if (pfNbRequired[p])
      mNbContexts++;
  }

  mContexts = new cl_context[mNbContexts];
  unsigned ctxtId = 0;

  // For each platform, create a context if the number of device is > 0
  for (unsigned p=0; p<machineNbPf; p++) {
    if (!pfNbRequired[p])
      continue;

    cl_platform_id currentPf = machinePfs[p];

    // Get nb devices
    cl_uint pfNbDevices;
    err = clGetDeviceIDs(currentPf, CL_DEVICE_TYPE_ALL, 0, NULL,
			 &pfNbDevices);
    clCheck(err, __FILE__, __LINE__);

    // Get devices
    cl_device_id pfDevices[pfNbDevices];
    err = clGetDeviceIDs(currentPf, CL_DEVICE_TYPE_ALL, pfNbDevices,
			 pfDevices, NULL);
    clCheck(err, __FILE__, __LINE__);

    // Get required devices
    cl_device_id requiredDevices[pfNbRequired[p]];

    unsigned idx = 0;

    for (unsigned i=0; i<mNbDevices; ++i) {
      unsigned pfId = platformDevices[i*2];

      if (pfId != p)
	continue;

      unsigned devId = platformDevices[i*2 + 1];

      if (devId > pfNbDevices) {
	std::cerr << "ContextHandle: Error platform no " << platformDevices[i*2]
		  << " has only " << pfNbDevices << "\n";
	exit(EXIT_FAILURE);
      }

      requiredDevices[idx++] = pfDevices[devId];
      mDevices[i] = pfDevices[devId];
    }

    if (idx != pfNbRequired[p]) {
      std::cerr << "idx=" << idx << " != nbRequired=" << pfNbRequired[p] << "\n";
      exit(EXIT_FAILURE);
    }

    // Create context
    cl_context context = real_clCreateContext(NULL,
					      pfNbRequired[p],
					      requiredDevices,
					      NULL, NULL, &err);
    clCheck(err, __FILE__, __LINE__);

    mContexts[ctxtId++] = context;

    for (unsigned i=0; i<mNbDevices; ++i) {
      unsigned pfId = platformDevices[i*2];
      if (pfId == p)
	mDevContexts[i] = context;
    }
  }
}

unsigned
ContextHandle::getNbContexts() const {
  return mNbContexts;
}
