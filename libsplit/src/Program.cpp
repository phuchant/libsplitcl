#include "ContextHandle.h"
#include "ProgramHandle.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

cl_program clCreateProgramWithBinary (cl_context context,
				      cl_uint num_devices,
				      const cl_device_id *device_list,
				      const size_t *lengths,
				      const unsigned char **binaries,
				      cl_int *binary_status,
				      cl_int *errcode_ret) {
  (void) device_list;

  if (!binaries || !lengths) {
    std::cerr << "clCreateProgramWithBinary error\n";
    exit(EXIT_FAILURE);
  }

  std::cerr << "WARNING: clCreateProgramWithBinary works only with SPIR 1.2\n"
	    << "calls to get_group_id(), get_num_groups() and get_global_size()"
	    << " not handled yet!\n";

  // Create and return a ProgramHandle object instead of a cl_program

  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);

  ProgramHandle *p = new ProgramHandle(c, binaries[0], lengths[0]);

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  if (binary_status) {
    for (cl_uint i=0; i<num_devices; i++) {
      binary_status[i] = CL_SUCCESS;
    }
  }

  return (cl_program) p;
}

cl_program clCreateProgramWithSource (cl_context context,
				      cl_uint count,
				      const char **strings,
				      const size_t *lengths,
				      cl_int *errcode_ret)
{
  // Create and return a ProgramHandle object instead of a cl_program

  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);

  ProgramHandle *p = new ProgramHandle(c, count, strings, lengths);

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return (cl_program) p;
}

cl_int clBuildProgram (cl_program program,
		       cl_uint num_devices,
		       const cl_device_id *device_list,
		       const char *options,
		       void (*pfn_notify)(cl_program, void *user_data),
		       void *user_data)
{
  // ignore devices
  (void) num_devices;
  (void) device_list;

  ProgramHandle *p = reinterpret_cast<ProgramHandle *>(program);

  p->build(options, pfn_notify, user_data);

  return CL_SUCCESS;
}

cl_int clRetainProgram (cl_program program)
{
  ProgramHandle *p = reinterpret_cast<ProgramHandle *>(program);
  p->retain();
  return CL_SUCCESS;
}

cl_int clReleaseProgram(cl_program program)
{
  ProgramHandle *p = reinterpret_cast<ProgramHandle *>(program);

  // If reference counter = 0 we can delete the ProgramHandle object.
  if (p->release())
    delete p;

  return CL_SUCCESS;
}

cl_int clGetProgramInfo(cl_program program,
			cl_program_info param_name,
			size_t param_value_size,
			void *param_value,
			size_t *param_value_size_ret)
{
  ProgramHandle *p = reinterpret_cast<ProgramHandle *>(program);

  p->getProgramInfo(param_name, param_value_size, param_value,
		    param_value_size_ret);

  return CL_SUCCESS;
}

cl_int clGetProgramBuildInfo (cl_program  program,
			      cl_device_id  device,
			      cl_program_build_info  param_name,
			      size_t  param_value_size,
			      void  *param_value,
			      size_t  *param_value_size_ret)
{
  ProgramHandle *p = reinterpret_cast<ProgramHandle *>(program);

  p->getProgramBuildInfo(device, param_name, param_value_size, param_value,
			 param_value_size_ret);

  return CL_SUCCESS;
}

cl_int clCompileProgram ( cl_program program,
			  cl_uint num_devices,
			  const cl_device_id *device_list,
			  const char *options,
			  cl_uint num_input_headers,
			  const cl_program *input_headers,
			  const char **header_include_names,
			  void (CL_CALLBACK *pfn_notify)( cl_program program,
							  void *user_data),
			  void *user_data) {
  (void) num_devices;
  (void) device_list;
  (void) input_headers;
  (void) header_include_names;

  if (num_input_headers > 0) {
    std::cerr << "error: clCompileProgram with input_headers not handled\n";
    exit(EXIT_FAILURE);
  }

  ProgramHandle *p = reinterpret_cast<ProgramHandle *>(program);

  p->build(options, pfn_notify, user_data);

  return CL_SUCCESS;
}
