#include <iostream>

#include "BufferManagerSimple.h"
#include "BufferManagerOptim.h"
#include <Debug.h>
#include "ContextHandle.h"
#include "Globals.h"
#include <Options.h>
#include "Utils.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <CL/opencl.h>
#include <string.h>

static cl_context my_clCreateContext()
{
  // Create and return a ContextHandle
  ContextHandle *ret = new ContextHandle();

  DEBUG("log",
	if (!logger)
	  logger = new Logger(optDeviceSelection.size() / 2);
	);

  return (cl_context) ret;
}

cl_context clCreateContextFromType(const cl_context_properties   *properties,
				   cl_device_type  device_type,
				   void  (*pfn_notify)
				   (const char *errinfo,
				    const void  *private_info,
				    size_t  cb,
				    void  *user_data),
				   void  *user_data,
				   cl_int  *errcode_ret)
{
  (void) properties;
  (void) device_type;
  (void) pfn_notify;
  (void) user_data;

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return my_clCreateContext();
}

cl_context clCreateContext(const cl_context_properties *properties,
			   cl_uint num_devices,
			   const cl_device_id *devices,
			   void (*pfn_notify)
			   (const char *errinfo,
			    const void *private_info, size_t cb,
			    void *user_data),
			   void *user_data,
			   cl_int *errcode_ret)
{
  (void) properties;
  (void) num_devices;
  (void) devices;
  (void) pfn_notify;
  (void) user_data;

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return my_clCreateContext();
}

cl_int clRetainContext(cl_context context) {
  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);
  c->retain();
  return CL_SUCCESS;
}

cl_int clReleaseContext(cl_context context) {
  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);
  // If reference counter = 0 we can delete the ContextHandle object.
  if (c->release())
    delete c;

  return CL_SUCCESS;
}

cl_int clGetContextInfo(cl_context context,
			cl_context_info param_name,
			size_t param_value_size,
			void *param_value,
			size_t * param_value_size_ret) {
  cl_int err;

  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);

  cl_context ctxt = c->getContext(0);

  err = real_clGetContextInfo(ctxt, param_name, param_value_size, param_value,
			      param_value_size_ret);
  clCheck(err, __FILE__, __LINE__);

  return CL_SUCCESS;
}
