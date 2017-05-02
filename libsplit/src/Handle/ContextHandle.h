#ifndef CONTEXTHANDLE_H
#define CONTEXTHANDLE_H

#include <Utils/Retainable.h>

#include <CL/opencl.h>

#include <map>

// This class creates the contexts required and one command queue per device.

namespace libsplit {

  class DeviceQueue;

  class ContextHandle : public Retainable {
  public:
    ContextHandle();
    ~ContextHandle();

    unsigned getNbDevices() const;
    cl_context getContext(unsigned i);
    cl_device_id getDevice(unsigned i);
    DeviceQueue *getQueueNo(unsigned i);
    unsigned getNbContexts() const;
    cl_context getContextFromDevice(cl_device_id d);

  private:
    unsigned nbDevices;
    cl_device_id *devices; // Array of devices

    std::map<cl_device_id, cl_context> dev2ContextMap;

    unsigned nbContexts;
    cl_context *contexts;
    DeviceQueue **queues;

    void createOneCtxtPerDevice(unsigned *platformDevices);
    void createOneCtxtPerPlatform(unsigned *platformDevices);
  };

};

#endif /* CONTEXTHANDLE_H */
