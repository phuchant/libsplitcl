#include <Define.h>
#include <Handle/ProgramHandle.h>
#include <Utils/Debug.h>
#include <Utils/Utils.h>

#include <fstream>

#include <cassert>
#include <cstdio>
#include <cstring>

namespace libsplit {

  int ProgramHandle::idxCount = -1;

  void
  ProgramHandle::init(ContextHandle *context) {
    this->context = context;
    hasBeenBuilt = false;
    idx = ++idxCount;

    nbPrograms = context->getNbDevices();
    programs = new cl_program[nbPrograms];
  }

  ProgramHandle::ProgramHandle(ContextHandle *context, cl_uint count,
			       const char **strings, const size_t *lengths)
    : count(count), lengths(lengths), programBinary(NULL), binaryLength(0),
      isBinary(false) {
    init(context);

    programSources = new char *[count];
    for(unsigned i=0; i<count; ++i) {

      if (lengths && lengths[i]) {
	programSources[i] = new char[lengths[i] + 1];
	memcpy(programSources[i], strings[i], lengths[i]);
	programSources[i][lengths[i]] = '\0';
      } else {
	programSources[i] = new char[strlen(strings[i]) + 1];
	memcpy(programSources[i], strings[i], strlen(strings[i]));
	programSources[i][strlen(strings[i])] = '\0';
      }
    }
  }

  ProgramHandle::ProgramHandle(ContextHandle *context,
			       const unsigned char *binary,
			       size_t length)
    : count(0), programSources(NULL), lengths(NULL), binaryLength(length),
      isBinary(true) {
    init(context);

    programBinary = new unsigned char[binaryLength];
    memcpy(programBinary, binary, binaryLength);
  }

  ProgramHandle::~ProgramHandle() {
    delete programBinary;

    if (hasBeenBuilt) {
      cl_int err = CL_SUCCESS;
      for (unsigned i=0; i<nbPrograms; i++)
	err |= real_clReleaseProgram(programs[i]);
      clCheck(err, __FILE__, __LINE__);
    }

    for (unsigned i=0; i<count; ++i)
      delete[] programSources[i];
    delete[] programSources;
    delete[] programs;
  }

  void
  ProgramHandle::build(const char *options, void (*pfn_notify)
		       (cl_program, void *user_data), void *user_data) {
    // Make transformations and create program
    if (isBinary)
      createProgramsWithBinary();
    else
      createProgramsWithSource(options);

    // Build programs
    for (unsigned i=0; i<nbPrograms; i++) {
      cl_int err = real_clBuildProgram(programs[i], 0, NULL, options,
				       pfn_notify, user_data);
      if (err != CL_SUCCESS) {
	size_t len;
	real_clGetProgramBuildInfo(programs[i],
				   context->getDevice(i),
				   CL_PROGRAM_BUILD_LOG,
				   0,
				   NULL,
				   &len);

	char *log = new char[len];

	real_clGetProgramBuildInfo(programs[i],
				   context->getDevice(i),
				   CL_PROGRAM_BUILD_LOG,
				   len,
				   log,
				   NULL);

	std::cerr << "build error :\n" << log << "\n";
      }
      clCheck(err, __FILE__, __LINE__);
    }

    // If it is not a binary, generate LLVM IR.
    if (!isBinary) {
      char src_filename[64];
      sprintf(src_filename, ".source%d.cl", idx);

      // Generate inline sources
      char inline_filename[64];
      sprintf(inline_filename, ".source-inline%d.cl", idx);
      {
	char command[1024];
	sprintf(command, "%s %s %s %s > %s 2> /dev/null", CLINLINEPATH,
		options ? options : "",
		"-Dcl_khr_fp64", src_filename, inline_filename);
	// cl_khr_fp64 is defined to avoid Clang errors when using doubles

	DEBUG("programhandle",
	      std::cerr << "inline command : " << command << "\n";
	      );

	system(command);
      }

      // Generate spir
      char bc_filename[64];
      sprintf(bc_filename, ".spir%d.bc", idx);

      // Generate spir with no optimization
      {
	char command[1024];
	sprintf(command, "%s %s %s %s -o %s",
		CLANGPATH,
		OPENCLFLAGS,
		options ? options : "",
		inline_filename,
		bc_filename);

	DEBUG("programhandle",
	      std::cerr << "spir command : " << command << "\n";
	      );

	system(command);
      }

      // Opt passes
      {
	char command[2048];
	sprintf(command, "%s %s -o %s < %s",
		OPTPATH,
		OPTPASSES,
		bc_filename,
		bc_filename);

	DEBUG("programhandle",
	      std::cerr << "opt command : " << command << "\n";
	      );

	system(command);
      }
    }

    hasBeenBuilt = true;
  }

  void
  ProgramHandle::getProgramInfo(cl_program_info param_name,
				size_t param_value_size,
				void *param_value, size_t *param_value_size_ret) {
    if (!hasBeenBuilt) {
      std::cerr << "Error: getProgramBuildInfo called but proram has not been built"
		<< " yet!\n";
      exit(EXIT_FAILURE);
    }

    cl_int err;

    err =  real_clGetProgramInfo(programs[0], param_name, param_value_size,
				 param_value, param_value_size_ret);
    clCheck(err, __FILE__, __LINE__);
  }

  void
  ProgramHandle::getProgramBuildInfo(cl_device_id device,
				     cl_program_info param_name,
				     size_t param_value_size, void *param_value,
				     size_t *param_value_size_ret) {
    if (!hasBeenBuilt) {
      std::cerr << "Error: getProgramBuildInfo called but proram has not been built"
		<< " yet!\n";
      exit(EXIT_FAILURE);
    }

    cl_int err;
    err =  real_clGetProgramBuildInfo(getProgramFromDevice(device),
				      device,
				      param_name,
				      param_value_size, param_value,
				      param_value_size_ret);
    clCheck(err, __FILE__, __LINE__);
  }

  void
  ProgramHandle::createProgramsWithSource(const char *options) {
    cl_int err;

    // Write source
    char src_filename[64];
    sprintf(src_filename, ".source%d.cl", idx);
    FILE *fd = fopen(src_filename, "w");
    for (unsigned i = 0; i < count; ++i)
      fprintf(fd, "%s", programSources[i]);
    fclose(fd);

    // Transform sources with ClTransform
    char trans_filename[64];
    sprintf(trans_filename, ".trans%d.cl", idx);
    {
      char command[1024];
      sprintf(command, "%s %s %s %s > %s 2> /dev/null", CLTRANSFORMPATH,
	      options ? options : "",
	      "-Dcl_khr_fp64", src_filename, trans_filename);
      // cl_khr_fp64 is defined to avoid Clang errors when using doubles

      DEBUG("programhandle",
	    std::cerr << "cltranform command : " << command << "\n";
	    );
      system(command);
    }

    // Load transformed source
    char *trans_source = file_load(trans_filename);

    // Create split programs
    for (unsigned i=0; i<nbPrograms; i++) {
      programs[i] = real_clCreateProgramWithSource(context->getContext(i),
						   1,
						   (const char **) &trans_source,
						   NULL, &err);
      clCheck(err, __FILE__, __LINE__);
      dev2ProgramMap[context->getDevice(i)] = programs[i];
    }

    free(trans_source);
  }

  void
  ProgramHandle::createProgramsWithBinary() {
    std::cerr << "Error: clCreateProgramWithBinary() not handled yet !\n";
    exit(EXIT_FAILURE);
    // cl_int err;

    // // Write binary
    // char bin_filename[64];
    // sprintf(bin_filename, ".spir%d.bc", idx);

    // std::ofstream ofile(bin_filename, std::ios::out | std::ios::trunc
    // 			| std::ios::binary);

    // ofile.write((const char *) programBinary, binaryLength);
    // ofile.close();

    // // Transform sources with ClTransform
    // char trans_filename[64];
    // sprintf(trans_filename, ".trans%d.bc", idx);
    // {
    //   char command[1024];
    //   sprintf(command, "%s -load %s %s %s > %s 2> /dev/null",
    // 	      OPTPATH, SPIRTRANSFORMPATH, SPIRPASS, bin_filename, trans_filename);

    //   system(command);
    // }

    // // Load transformed binary
    // size_t transBinarySize;
    // unsigned char *transBinary = binary_load(trans_filename,
    // 						   &transBinarySize);

    // assert(context->getNbContexts() == context->getNbDevices());

    // // Create split programs
    // for (unsigned i=0; i<nbPrograms; i++) {
    //   cl_context c = context->getContext(i);
    //   cl_device_id d = context->getDevice(i);
    //   programs[i] = real_clCreateProgramWithBinary(c,
    // 						   1,
    // 						   &d,
    // 						   &transBinarySize,
    // 						   (const unsigned char **)
    // 						   &transBinary,
    // 						   NULL, &err);
    //   clCheck(err, __FILE__, __LINE__);
    // }

    // free(transBinary);
  }

  cl_program
  ProgramHandle::getProgram(unsigned n) {
    return programs[n];
  }

  ContextHandle *
  ProgramHandle::getContext() {
    return context;
  }

  int
  ProgramHandle::getId() {
    return idx;
  }

  cl_program
  ProgramHandle::getProgramFromDevice(cl_device_id d) {
    assert(dev2ProgramMap.find(d) != dev2ProgramMap.end());
    return dev2ProgramMap[d];
  }

};
