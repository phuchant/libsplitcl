#include <iostream>

#include <CL/opencl.h>

cl_event clCreateUserEvent (cl_context context,
			    cl_int *errcode_ret)
{
  (void) context;
  (void) errcode_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}
