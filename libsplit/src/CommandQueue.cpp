#include "ContextHandle.h"
#include "Utils.h"

#include <CL/opencl.h>

cl_command_queue clCreateCommandQueue(cl_context context,
				      cl_device_id device,
				      cl_command_queue_properties properties,
				      cl_int *errcode_ret) {
  (void) device;

  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);
  cl_context ctxt = c->getContext(0);
  cl_device_id dev = c->getDevice(0);

  cl_int err;
  cl_command_queue ret = real_clCreateCommandQueue(ctxt, dev, properties, &err);
  clCheck(err, __FILE__, __LINE__);

  if (errcode_ret)
    *errcode_ret = err;

  return ret;
}

// cl_int clRetainCommandQueue(cl_command_queue command_queue)
// Nothing to do

// cl_int clReleaseCommandQueue(cl_command_queue command_queue)
// Nothing to do

// cl_int clSetCommandQueueProperty(cl_command_queue command_queue,
// 				    cl_command_queue_properties properties,
// 				    cl_bool enable,
// 				    cl_command_queue_properties *old_properties)
// Nothing to do
