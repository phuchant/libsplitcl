#include <Handle/ContextHandle.h>
#include <Handle/MemoryHandle.h>
#include <Utils/Utils.h>

#include <cassert>
#include <iostream>

#include <CL/cl.h>

using namespace libsplit;

/* Memory Object APIs */
cl_mem
clCreateBuffer(cl_context   context,
	       cl_mem_flags flags,
	       size_t       size,
	       void *       host_ptr,
	       cl_int *     errcode_ret)
{
  ContextHandle *c = reinterpret_cast<ContextHandle *>(context);
  MemoryHandle *ret = new MemoryHandle(c, flags, size, host_ptr);

  if (errcode_ret)
    *errcode_ret = CL_SUCCESS;

  return (cl_mem) ret;
}

cl_mem
clCreateSubBuffer(cl_mem                   /* buffer */,
		  cl_mem_flags             /* flags */,
		  cl_buffer_create_type    /* buffer_create_type */,
		  const void *             /* buffer_create_info */,
		  cl_int *                 /* errcode_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_mem
clCreateImage(cl_context              /* context */,
	      cl_mem_flags            /* flags */,
	      const cl_image_format * /* image_format */,
	      const cl_image_desc *   /* image_desc */,
	      void *                  /* host_ptr */,
	      cl_int *                /* errcode_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clRetainMemObject(cl_mem memobj)
{
  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(memobj);
  m->retain();
  return CL_SUCCESS;
}

cl_int
clReleaseMemObject(cl_mem memobj)
{
  MemoryHandle *m = reinterpret_cast<MemoryHandle *>(memobj);
  m->retain();
  return CL_SUCCESS;
}

cl_int
clGetSupportedImageFormats(cl_context           /* context */,
			   cl_mem_flags         /* flags */,
			   cl_mem_object_type   /* image_type */,
			   cl_uint              /* num_entries */,
			   cl_image_format *    /* image_formats */,
			   cl_uint *            /* num_image_formats */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clGetMemObjectInfo(cl_mem           memobj,
		   cl_mem_info      param_name,
		   size_t           param_value_size,
		   void *           param_value,
		   size_t *         param_value_size_ret)
{
  if (param_name == CL_MEM_TYPE) {
    assert(param_value_size >= sizeof(cl_mem_object_type));
    if (param_value_size_ret)
      *param_value_size_ret = sizeof(cl_mem_object_type);
    if (param_value)
      *((cl_mem_object_type *) param_value) = CL_MEM_OBJECT_BUFFER;
    return CL_SUCCESS;
  }

  if (param_name == CL_MEM_SIZE) {
    assert(param_value_size >= sizeof(size_t));
    if (param_value_size_ret)
      *param_value_size_ret = sizeof(size_t);
    if (param_value) {
      MemoryHandle *m = (MemoryHandle *) memobj;
      *((size_t *) param_value) = m->mSize;
    }
    return CL_SUCCESS;
  }
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clGetImageInfo(cl_mem           /* image */,
	       cl_image_info    /* param_name */,
	       size_t           /* param_value_size */,
	       void *           /* param_value */,
	       size_t *         /* param_value_size_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}

cl_int
clSetMemObjectDestructorCallback(  cl_mem /* memobj */,
				   void (CL_CALLBACK * /*pfn_notify*/)( cl_mem /* memobj */, void* /*user_data*/),
				   void * /*user_data */ )
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
  return CL_SUCCESS;
}
