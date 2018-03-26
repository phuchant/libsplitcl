#include <Options.h>
#include <Scheduler/Scheduler.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <cassert>
#include <cstring>
#include <unistd.h>

namespace libsplit {

  extern bool DebugFlag;
  extern std::vector<std::string> CurrentDebugType;

  std::vector<unsigned> optDeviceSelection;
  bool optPerPlatform = false;
  bool optDontSplit = false;
  unsigned optSingleDeviceID = 0;
  unsigned optNbSkipIter = 0;
  int optScheduler = Scheduler::ENV;
  bool optNoMemcpy = false;
  int optLogMaxIter = -1;
  std::vector<unsigned> optPartition;
  std::vector<double> optGranudscr;
  unsigned optSkipKernels = 0;
  bool optNoComm = false;
  bool optBest = false;
  bool optStop = false;
  std::vector<double> optGranustart;
  unsigned optCycleLength = 0;
  std::vector<double> optDComm;
  bool optLockFreeQueue = false;
  int optSampleSteps = 0;
  bool optEnableIndirections = true;
  bool optDelayedWrite = false;
  char *optFakeSources = nullptr;
  unsigned optShiftInit = 0;
  unsigned optShiftStep = 0;
  char *optBuildOptionDev[MAXDEVICES] = {nullptr, nullptr, nullptr, nullptr,
					 nullptr, nullptr, nullptr, nullptr,
					 nullptr, nullptr};
  bool optPinnedMem = true;

  struct option {
    const char *name;
    const char *descr;
    bool required;
    void (*action)(char *env);
  };

  static void helpOption(char *env);
  static void debugOption(char *env);
  static void debugtypeOption(char *env);
  static void devicesOption(char *env);
  static void perplatformOption(char *env);
  static void singleidOption(char *env);
  static void dontsplitOption(char *env);
  static void nbskipiterOption(char *env);
  static void schedOption(char *env);
  static void nomemcpyOption(char *env);
  static void partitionOption(char *env);
  static void partitionsOption(char *env);
  static void granudscrOption(char *env);
  static void skipOption(char *env);
  static void nocommOption(char *env);
  static void bestOption(char *env);
  static void stopOption(char *env);
  static void granustartOption(char *env);
  static void granustartsOption(char *env);
  static void cyclelengthOption(char *env);
  static void dcommOption(char *env);
  static void lockfreequeueOption(char *env);
  static void samplestepsOption(char *env);
  static void enableindirOption(char *env);
  static void delayedWriteOption(char *env);
  static void fakeSourcesOption(char *env);
  static void shiftInitOption(char *env);
  static void shiftStepOption(char *env);
  static void buildOptionDevOption(char *env);
  static void pinnedMemOption(char *env);

  static option opts[] = {
    {"HELP", "Display available options.", false, helpOption},
    {"DEBUG", "Execute all debug macros.", false, debugOption},
    {"DEBUGTYPE", "Execute only selected debug macros (e.g. DEBUGTYPE=foo,bar)." \
     "\n                      Available DEBUGTYPE are: broyden, kernelstats, " \
     "log, memcpy, programhandle, schedtimer, timer.", false, debugtypeOption},
    {"DEVICES", "Devices selection. Must be of the following form : " \
     "<pf id1, dev_id1, ..., pf_idN, dev_idN>.", true, devicesOption},
    {"PERPLATFORM", "One context per platform when set to 1," \
     " one context per device otherwise.", false, perplatformOption},
    {"SINGLEID", "The ID of the device to execute a kernel when it is not split.",
     false, singleidOption},
    {"DONTSPLIT", "When set to 1 no kernel is split.", false, dontsplitOption},
    {"NBSKIPITER", "Number of iterations to skip before splitting the kernel.",
     false, nbskipiterOption},
    {"SCHED", "Scheduler (BADBROYDEN, BROYDEN, ENV, FIXEDPOINT, MKGR, " \
     "MKSTATIC, SAMPLE)",
     false, schedOption},
    {"NOMEMCPY", "(not safe)", false, nomemcpyOption},
    {"PARTITION", "Kernel partition of the following form : " \
     " <denominator> <nominator1> <dev id1> ... <nominatorN> <dev idN>.", false,
     partitionOption},
    {"PARTITION<kerId>", "Kernel partition of the kernel kerId of the following" \
     " form : <denominator> <nominator1> <dev id1> ... <nominatorN> <dev idN>.",
     false, partitionsOption},
    {"GRANUDSCR", "Kernel partition of the follwing form: <dev, nb, granu, ... >",
     false, granudscrOption},
    {"SKIPKERNELS", "Start with Uniform splitting and  use the given partition after " \
     "SKIP kernels.", false, skipOption},
    {"NOCOMM", "Do not take into account communication time to compute the " \
     "granularity.", false, nocommOption},
    {"BEST", "Stop scheduler after reaching the \"best\" granularity.", false,
     bestOption},
    {"STOP", "Stop application after raching the \"best\" granularity.", false,
     stopOption},
    {"GRANUSTART", "Start with the given granularity instead of uniform " \
     "splitting.", false, granustartOption},
    {"GRANUSTART<kerId>", "Start with the given granularity instead of uniform " \
     "splitting for the kernel kerId.", false, granustartsOption},
    {"CYCLELENGTH", "Number of kernel in the sequence when using scheduler MKGR.",
     false, cyclelengthOption},
    {"DCOMM", "<d11> <12> for scheduler MKGR.",
     false, dcommOption},
    {"LOCKFREEQUEUE", "Use a lock free queue instead of a mutex queue.", false,
     lockfreequeueOption},
    {"SAMPLESTEPS", "Number of granularity steps sampled for each subkernels.",
     false,
     samplestepsOption},
    {"ENABLEINDIR", "Enable indirections.", false,
     enableindirOption},
    {"DELAYEDWRITE", "Delayed write (enabled by default)", false,
     delayedWriteOption},
    {"FAKESOURCES", "Use a fake OpenCL source for kernel analysis.", false,
     fakeSourcesOption},
    {"SHIFTINIT", "Initial number of workgroups shifted.", false,
     shiftInitOption},
    {"SHIFTSTEP", "Shift step.", false,
     shiftStepOption},
    {"BUILDOPTION<devId>", "Build option for device i.", false,
     buildOptionDevOption},
    {"PINNEDMEM", "Use pinned memory.", false,
     pinnedMemOption},

  };

  static void helpOption(char *env)
  {
    if (!env)
      return;

    std::cerr << "\n----------------------------------- LIBSPLIT --------------" \
      "--------------------\n\n";

    std::cerr << "Options :\n\n";

    for (option o : opts) {
      std::cerr << std::setw(20);
      std::cerr << std::left <<  o.name << "- " << o.descr << "\n";
    }

    exit(EXIT_SUCCESS);
  }

  static void debugOption(char *env) {
    if (env)
      DebugFlag = true;
  }

  static void debugtypeOption(char *env) {
    if (!env)
      return;

    DebugFlag = true;
    char *type;
    while ((type = strtok(env, ",")) != NULL) {
      env = NULL;
      CurrentDebugType.push_back(std::string(type));
    }
  }

  static void devicesOption(char *env) {
    if (!env)
      return;

    // Parse DEVICES
    std::string s(env);
    std::istringstream is(s);
    unsigned n;
    while (is >> n)
      optDeviceSelection.push_back(n);

    unsigned size = optDeviceSelection.size();
    if (size % 2 != 0) {
      std::cerr << "Error: bad environment variable DEVICES\n";
      exit(EXIT_FAILURE);
    }
    if (size / 2 > MAXDEVICES) {
      std::cerr << "Error: Maximum " << MAXDEVICES << " devices allowed !\n";
      exit(EXIT_FAILURE);
    }
  }

  static void perplatformOption(char *env) {
    if (env)
      optPerPlatform = true;
  }

  static void singleidOption(char *env) {
    if (env)
      optSingleDeviceID = atoi(env);
    else
      optSingleDeviceID = 0;
  }

  static void dontsplitOption(char *env) {
    if (env && atoi(env) == 1)
      optDontSplit = true;
    else
      optDontSplit = false;
  }

  static void nbskipiterOption(char *env) {
    if (env)
      optNbSkipIter = atoi(env);
  }

  static void schedOption(char *env) {
    if (env) {
      if (!strcmp(env, "BADBROYDEN")) {
	optScheduler = Scheduler::BADBROYDEN;
      } else if (!strcmp(env, "BROYDEN")) {
	optScheduler = Scheduler::BROYDEN;
      } else if (!strcmp(env, "FIXEDPOINT")) {
	optScheduler = Scheduler::FIXEDPOINT;
      } else if (!strcmp(env, "MKGR")) {
	optScheduler = Scheduler::MKGR;
      } else if (!strcmp(env, "MKSTATIC")) {
	optScheduler = Scheduler::MKSTATIC;
      } else if (!strcmp(env, "SAMPLE")) {
	optScheduler = Scheduler::SAMPLE;
      } else {
	optScheduler = Scheduler::ENV;
      }
    }
  }

  static void nomemcpyOption(char *env) {
    if (env)
      optNoMemcpy = true;
  }

  static void partitionOption(char *env) {
    if (!env)
      return;

    std::string s(env);
    std::istringstream is(s);
    int n;
    while (is >> n)
      optPartition.push_back(n);

    unsigned sum = 0;
    for (unsigned i=1; i<optPartition.size(); i+=2)
      sum += optPartition[i];
    if (sum != optPartition[0]) {
      std::cerr << "Error: bad partition !\n";
      exit(EXIT_FAILURE);
    }
  }

  static void partitionsOption(char *env) {
    (void) env;

    /* Do nothing, this option is handled directly by the scheduler. */
  }

  static void granudscrOption(char *env) {
    if (!env)
      return;

    std::string s(env);
    std::istringstream is(s);
    double n;
    while (is >> n)
      optGranudscr.push_back(n);
  }

  static void skipOption(char *env) {
    if (!env)
      return;

    optSkipKernels = atoi(env);
  }

  static void nocommOption(char *env) {
    if (!env)
      return;

    optNoComm = atoi(env) == 1;
  }

  static void bestOption(char *env) {
    if (!env)
      return;

    optBest = atoi(env) == 1;
  }

  static void stopOption(char *env) {
    if (!env)
      return;

    optBest = atoi(env) == 1;
  }

  static void granustartOption(char *env) {
    if (!env)
      return;

    std::string s(env);
    std::istringstream is(s);
    double n;
    while (is >> n)
      optGranustart.push_back(n);
  }

  static void granustartsOption(char *env) {
    (void) env;

    /* Do nothing, this option is handled directly by the scheduler. */

    return;
  }

  static void cyclelengthOption(char *env) {
    if (!env)
      return;

    optCycleLength = atoi(env);
  }

  static void dcommOption(char *env) {
    if (!env)
      return;

    std::string s(env);
    std::istringstream is(s);
    double n;
    while (is >> n)
      optDComm.push_back(n);
  }

  static void lockfreequeueOption(char *env) {
    if (!env)
      return;

    if (atoi(env) == 1)
      optLockFreeQueue = true;
  }

  static void samplestepsOption(char *env) {
    if (!env)
      return;

    optSampleSteps = atoi(env);
  }

  static void enableindirOption(char *env) {
    if (!env)
      return;

    optEnableIndirections = atoi(env);
  }

  static void delayedWriteOption(char *env) {
    if (!env)
      return;

    optDelayedWrite = atoi(env);
  }

  static void fakeSourcesOption(char *env) {
    if (!env)
      return;

    optFakeSources = (char *) malloc(strlen(env) + 1);
    strcpy(optFakeSources, env);
    assert(access(optFakeSources, R_OK) == 0);
  }

  static void shiftInitOption(char *env) {
    if (!env)
      return;

    optShiftInit = atoi(env);
  }

  static void shiftStepOption(char *env) {
    if (!env)
      return;

    optShiftStep = atoi(env);
  }

  static void buildOptionDevOption(char *env) {
    char opt_name[64];
    for (unsigned i=0; i<MAXDEVICES; i++) {
      sprintf(opt_name, "BUILDOPTION%d", i);
      env = getenv(opt_name);
      if (!env)
	continue;
      optBuildOptionDev[i] = strdup(env);
    }
  }

  static void pinnedMemOption(char *env) {
    if (!env)
      return;
    optPinnedMem = atoi(env);
  }

  void parseEnvOptions()
  {
    for (option o : opts) {
      char *env = getenv(o.name);
      if (o.required && !env) {
	std::cerr << "Error: environment option " << o.name << " not set !\n";
	std::cerr << "Set environment variable HELP to 1 to have more "
		  << "informations.\n";
	exit(EXIT_FAILURE);
      }

      o.action(env);
    }

    if ((optScheduler == Scheduler::MKGR ||
	 optScheduler == Scheduler::MKSTATIC) && optCycleLength == 0) {
      std::cerr << "Error: option CYCLELENGTH must be set when using MKGR or " \
	"MKSTATIC scheduler.\n";
      exit(EXIT_FAILURE);
    }

    if (optScheduler == Scheduler::SAMPLE && optSampleSteps <= 0) {
      std::cerr << "Error: option SAMPLESTEPS must be set when using SAMPLE " \
	"scheduler.\n";
      exit(EXIT_FAILURE);
    }
  }

};
