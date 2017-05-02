#include <Dispatch/OpenCLFunctions.h>
#include <Handle/ContextHandle.h>
#include <Utils/Utils.h>
#include <Init.h>

#include <iostream>

#include <CL/cl.h>

using namespace libsplit;

static cl_context my_clCreateContext()
{
  init();

  // Create and return a ContextHandle
  ContextHandle *ret = new ContextHandle();

  return (cl_context) ret;
}


/* Context APIs  */
cl_context
clCreateContext(const cl_context_properties * properties,
		cl_uint                 num_devices,
		const cl_device_id *    devices,
		void (CL_CALLBACK * pfn_notify)(const char *, const void *, size_t, void *),
		void *                  user_data,
		cl_int *                errcode_ret)
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

cl_context
clCreateContextFromType(const cl_context_properties * properties,
			cl_device_type          device_type,
			void (CL_CALLBACK *     pfn_notify )(const char *, const void *, size_t, void *),
			void *                  user_data,
			cl_int *                errcode_ret)
{
  (void) properties;
  (void) device_type;
  (void) pfn_notify;
  (void) user_data;

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return my_clCreateContext();
}

cl_int
clRetainContext(cl_context context)
{
  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);
  c->retain();
  return CL_SUCCESS;
}

cl_int
clReleaseContext(cl_context context)
{
  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);
  c->release();
  return CL_SUCCESS;
}

cl_int
clGetContextInfo(cl_context         context,
		 cl_context_info    param_name,
		 size_t             param_value_size,
		 void *             param_value,
		 size_t *           param_value_size_ret)
{
  cl_int err;

  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);

  cl_context ctxt = c->getContext(0);

  err = real_clGetContextInfo(ctxt, param_name, param_value_size, param_value,
			      param_value_size_ret);
  clCheck(err, __FILE__, __LINE__);

  return CL_SUCCESS;
}
