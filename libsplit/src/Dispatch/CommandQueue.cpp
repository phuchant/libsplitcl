#include <Dispatch/OpenCLFunctions.h>
#include <Handle/ContextHandle.h>
#include <Utils/Utils.h>

#include <iostream>

#include <CL/cl.h>

using namespace libsplit;

/* Command Queue APIs */
cl_command_queue
clCreateCommandQueue(cl_context                     context,
		     cl_device_id                   device,
		     cl_command_queue_properties    properties,
		     cl_int *                       errcode_ret)
{
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

cl_int
clRetainCommandQueue(cl_command_queue command_queue)
{
  return real_clRetainCommandQueue(command_queue);
}

cl_int
clReleaseCommandQueue(cl_command_queue command_queue)
{
  return real_clRetainCommandQueue(command_queue);
}

cl_int
clGetCommandQueueInfo(cl_command_queue      command_queue ,
		      cl_command_queue_info param_name,
		      size_t                param_value_size,
		      void *                param_value,
		      size_t *              param_value_size_ret)
{
  return real_clGetCommandQueueInfo(command_queue, param_name, param_value_size,
				    param_value, param_value_size_ret);
}

/* Flush and Finish APIs */
cl_int
clFlush(cl_command_queue /* command_queue */)
{
  return CL_SUCCESS;
}

cl_int
clFinish(cl_command_queue /* command_queue */)
{
  return CL_SUCCESS;
}
