#ifndef UTILS_H
#define UTILS_H

#include <Dispatch/OpenCLFunctions.h>

#include <cstdlib>
#include <iostream>
#include <string>


namespace libsplit {
  size_t file_size(const char *filename);

  unsigned char *binary_load(const char *filename, size_t *size);

  char *file_load(const char *filename);

  double get_time();

  inline void clCheck(cl_int err, std::string filename, int linenum) {
    if (err != CL_SUCCESS) {

      std::string name;
      switch(err) {
#define NAMED_ERROR(CODE)			\
	case CODE:				\
	  name = #CODE;				\
	  break;
	NAMED_ERROR(CL_SUCCESS)
	  NAMED_ERROR(CL_DEVICE_NOT_FOUND)
	  NAMED_ERROR(CL_DEVICE_NOT_AVAILABLE)
	  NAMED_ERROR(CL_COMPILER_NOT_AVAILABLE)
	  NAMED_ERROR(CL_MEM_OBJECT_ALLOCATION_FAILURE)
	  NAMED_ERROR(CL_OUT_OF_RESOURCES)
	  NAMED_ERROR(CL_OUT_OF_HOST_MEMORY)
	  NAMED_ERROR(CL_PROFILING_INFO_NOT_AVAILABLE)
	  NAMED_ERROR(CL_MEM_COPY_OVERLAP)
	  NAMED_ERROR(CL_IMAGE_FORMAT_MISMATCH)
	  NAMED_ERROR(CL_IMAGE_FORMAT_NOT_SUPPORTED)
	  NAMED_ERROR(CL_BUILD_PROGRAM_FAILURE)
	  NAMED_ERROR(CL_MAP_FAILURE)
	  NAMED_ERROR(CL_MISALIGNED_SUB_BUFFER_OFFSET)
	  NAMED_ERROR(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST)
	  NAMED_ERROR(CL_COMPILE_PROGRAM_FAILURE)
	  NAMED_ERROR(CL_LINKER_NOT_AVAILABLE)
	  NAMED_ERROR(CL_LINK_PROGRAM_FAILURE)
	  NAMED_ERROR(CL_DEVICE_PARTITION_FAILED)
	  NAMED_ERROR(CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
	  NAMED_ERROR(CL_INVALID_VALUE)
	  NAMED_ERROR(CL_INVALID_DEVICE_TYPE)
	  NAMED_ERROR(CL_INVALID_PLATFORM)
	  NAMED_ERROR(CL_INVALID_DEVICE)
	  NAMED_ERROR(CL_INVALID_CONTEXT)
	  NAMED_ERROR(CL_INVALID_QUEUE_PROPERTIES)
	  NAMED_ERROR(CL_INVALID_COMMAND_QUEUE)
	  NAMED_ERROR(CL_INVALID_HOST_PTR)
	  NAMED_ERROR(CL_INVALID_MEM_OBJECT)
	  NAMED_ERROR(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
	  NAMED_ERROR(CL_INVALID_IMAGE_SIZE)
	  NAMED_ERROR(CL_INVALID_SAMPLER)
	  NAMED_ERROR(CL_INVALID_BINARY)
	  NAMED_ERROR(CL_INVALID_BUILD_OPTIONS)
	  NAMED_ERROR(CL_INVALID_PROGRAM)
	  NAMED_ERROR(CL_INVALID_PROGRAM_EXECUTABLE)
	  NAMED_ERROR(CL_INVALID_KERNEL_NAME)
	  NAMED_ERROR(CL_INVALID_KERNEL_DEFINITION)
	  NAMED_ERROR(CL_INVALID_KERNEL)
	  NAMED_ERROR(CL_INVALID_ARG_INDEX)
	  NAMED_ERROR(CL_INVALID_ARG_VALUE)
	  NAMED_ERROR(CL_INVALID_ARG_SIZE)
	  NAMED_ERROR(CL_INVALID_KERNEL_ARGS)
	  NAMED_ERROR(CL_INVALID_WORK_DIMENSION)
	  NAMED_ERROR(CL_INVALID_WORK_GROUP_SIZE)
	  NAMED_ERROR(CL_INVALID_WORK_ITEM_SIZE)
	  NAMED_ERROR(CL_INVALID_GLOBAL_OFFSET)
	  NAMED_ERROR(CL_INVALID_EVENT_WAIT_LIST)
	  NAMED_ERROR(CL_INVALID_EVENT)
	  NAMED_ERROR(CL_INVALID_OPERATION)
	  NAMED_ERROR(CL_INVALID_GL_OBJECT)
	  NAMED_ERROR(CL_INVALID_BUFFER_SIZE)
	  NAMED_ERROR(CL_INVALID_MIP_LEVEL)
	  NAMED_ERROR(CL_INVALID_GLOBAL_WORK_SIZE)
	  NAMED_ERROR(CL_INVALID_PROPERTY)
	  NAMED_ERROR(CL_INVALID_IMAGE_DESCRIPTOR)
	  NAMED_ERROR(CL_INVALID_COMPILER_OPTIONS)
	  NAMED_ERROR(CL_INVALID_LINKER_OPTIONS)
	  NAMED_ERROR(CL_INVALID_DEVICE_PARTITION_COUNT)
#undef NAMED_ERROR
      default:
	  name = "unknown";
      };

      std::cerr << "OpenCL error : " << name << " from file "
		<< std::string(filename) << " at line "
		<< linenum << "\n";
      exit(EXIT_FAILURE);
    }
  }

};

#endif /* UTILS_H */
