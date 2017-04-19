#include <iostream>

#include <CL/opencl.h>

cl_sampler clCreateSampler (cl_context context,
			    cl_bool normalized_coords,
			    cl_addressing_mode addressing_mode,
			    cl_filter_mode filter_mode,
			    cl_int *errcode_ret)
{
  (void) context;
  (void) normalized_coords;
  (void) addressing_mode;
  (void) filter_mode;
  (void) errcode_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clRetainSampler(cl_sampler sampler)
{
  (void) sampler;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clReleaseSampler (cl_sampler sampler)
{
  (void) sampler;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int clGetSamplerInfo (cl_sampler sampler,
			 cl_sampler_info param_name,
			 size_t param_value_size,
			 void *param_value,
			 size_t *param_value_size_ret)
{
  (void) sampler;
  (void) param_name;
  (void) param_value_size;
  (void) param_value;
  (void) param_value_size_ret;

  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}


