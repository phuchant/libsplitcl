#include <Globals.h>
#include <OpenCLFunctions.h>
#include <Options.h>

static bool initDone = false;


cl_int clGetPlatformIDs(cl_uint num_entries,
			cl_platform_id *platforms,
			cl_uint *num_platforms) {
  if (!initDone) {
    initDone = true;
    parseEnvOptions();
    getOpenCLFunctions();
    initGlobals();
  }

  return real_clGetPlatformIDs(num_entries, platforms, num_platforms);
}
