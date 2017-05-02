#include <Dispatch/OpenCLFunctions.h>

#include <iostream>

#include <CL/cl.h>

/* Device APIs */
cl_int
clGetDeviceIDs(cl_platform_id   platform,
	       cl_device_type   device_type,
	       cl_uint          num_entries,
	       cl_device_id *   devices,
	       cl_uint *        num_devices)
{
  return real_clGetDeviceIDs(platform, device_type, num_entries, devices,
			     num_devices);
}

cl_int
clGetDeviceInfo(cl_device_id    device,
		cl_device_info  param_name,
		size_t          param_value_size,
		void *          param_value,
		size_t *        param_value_size_ret)
{
  return real_clGetDeviceInfo(device, param_name, param_value_size, param_value,
			      param_value_size_ret);
}

cl_int
clCreateSubDevices(cl_device_id                         /* in_device */,
		   const cl_device_partition_property * /* properties */,
		   cl_uint                              /* num_devices */,
		   cl_device_id *                       /* out_devices */,
		   cl_uint *                            /* num_devices_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clRetainDevice(cl_device_id device)
{
  return real_clRetainDevice(device);
}

cl_int
clReleaseDevice(cl_device_id device)
{
  return real_clReleaseDevice(device);
}
