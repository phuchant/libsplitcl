#ifndef OPTIONS_H
#define OPTIONS_H

#include <vector>

#define MAXDEVICES 10

namespace libsplit {

  extern std::vector<unsigned> optDeviceSelection;
  extern bool optPerPlatform;
  extern bool optDontSplit;
  extern unsigned optSingleDeviceID;
  extern unsigned optMergeDeviceID;
  extern unsigned optNbSkipIter;
  extern int optScheduler;
  extern bool optNoMemcpy;
  extern std::vector<unsigned> optPartition;
  extern std::vector<unsigned> optPartitions;
  extern std::vector<double> optGranudscr;
  extern unsigned optSkip;
  extern bool optNoComm;
  extern bool optBest;
  extern bool optStop;
  extern std::vector<double> optGranustart;
  extern unsigned optCycleLength;
  extern std::vector<double> optDComm;
  extern bool optLockFreeQueue;
  extern int optSampleSteps;
  extern bool optEnableIndirections;
  extern bool optDelayedWrite;
  extern char *optFakeSources;
  extern unsigned optShiftInit;
  extern unsigned optShiftStep;
  extern char *optBuildOptionDev[MAXDEVICES];
  extern unsigned optProfilerVersion;

  void parseEnvOptions();

};

#endif /* OPTIONS_H */
