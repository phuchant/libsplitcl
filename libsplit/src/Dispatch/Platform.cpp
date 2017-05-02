#include <Dispatch/OpenCLFunctions.h>
#include <Init.h>

#include <iostream>

#include <CL/cl.h>

using namespace libsplit;

/* Platform API */
cl_int
clGetPlatformIDs(cl_uint          num_entries,
		 cl_platform_id * platforms,
		 cl_uint *        num_platforms)
{
  init();

  return real_clGetPlatformIDs(num_entries, platforms, num_platforms);
}

cl_int
clGetPlatformInfo(cl_platform_id   platform,
		  cl_platform_info param_name,
		  size_t           param_value_size,
		  void *           param_value,
		  size_t *         param_value_size_ret)
{
  return real_clGetPlatformInfo(platform, param_name, param_value_size,
				param_value, param_value_size_ret);
}
