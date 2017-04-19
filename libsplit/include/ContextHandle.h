#ifndef CONTEXTHANDLE_H
#define CONTEXTHANDLE_H

#include "Retainable.h"

#include <CL/opencl.h>

// This class creates the contexts required and one command queue per device.

class ContextHandle : public Retainable {
public:
  ContextHandle();
  ~ContextHandle();

  unsigned getNbDevices() const;
  cl_context getContext(unsigned i);
  cl_device_id getDevice(unsigned i);
  cl_command_queue getQueueNo(unsigned i);
  unsigned getNbContexts() const;

  virtual bool release();

private:
  unsigned mNbDevices;
  cl_device_id *mDevices; // Array of devices
  cl_context *mDevContexts; // Array of size mNbDevices containing the context
  // associated with each devices.

  unsigned mNbContexts;
  cl_context *mContexts;
  cl_command_queue *mQueues;

  void createOneCtxtPerDevice(unsigned *platformDevices);
  void createOneCtxtPerPlatform(unsigned *platformDevices);
};



#endif /* CONTEXTHANDLE_H */
