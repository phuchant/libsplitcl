#ifndef PROGRAMHANDLE_H
#define PROGRAMHANDLE_H

#include <Handle/ContextHandle.h>
#include <Utils/Retainable.h>

#include <CL/opencl.h>

#include <map>

namespace libsplit {

  class ProgramHandle : public Retainable {
  private:
    void init(ContextHandle *context);

  public:
    ProgramHandle(ContextHandle *context, cl_uint count, const char **strings,
		  const size_t *lengths);
    ProgramHandle(ContextHandle *context, const unsigned char *binary,
		  size_t length);
    ~ProgramHandle();

    void build(const char *options, void (*pfn_notify)
	       (cl_program, void *user_data), void *user_data);
    void getProgramInfo(cl_program_info param_name, size_t param_value_size,
			void *param_value, size_t *param_value_size_ret);
    void getProgramBuildInfo(cl_device_id device, cl_program_info param_name,
			     size_t param_value_size, void *param_value,
			     size_t *param_value_size_ret);

    // Getters
    cl_program getProgram(unsigned n);
    cl_program getProgramFromDevice(cl_device_id d);
    ContextHandle *getContext();

    int getId();

  private:
    void createProgramsWithSource(const char *options);
    void createProgramsWithBinary();

    cl_uint count;
    char **programSources;
    const size_t *lengths;

    unsigned char *programBinary;
    size_t binaryLength;
    bool isBinary;

    ContextHandle *context;

    bool hasBeenBuilt;
    unsigned nbPrograms;
    cl_program *programs;

    std::map<cl_device_id, cl_program> dev2ProgramMap;

    int idx;

    static int idxCount;
  };

};

#endif /* PROGRAMHANDLE_H */
