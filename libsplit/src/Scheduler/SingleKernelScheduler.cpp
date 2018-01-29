#include <Options.h>
#include <Scheduler/SingleKernelScheduler.h>

namespace libsplit {

  SingleKernelScheduler::SingleKernelScheduler(BufferManager *buffManager,
					       unsigned nbDevices)
    : Scheduler(buffManager, nbDevices) {
    if (optNoComm)
      getSubkernelPerf = &SingleKernelScheduler::getSubkernelPerfWithoutComm;
    else
      getSubkernelPerf = &SingleKernelScheduler::getSubkernelPerfWithComm;
  }

  double
  SingleKernelScheduler::getSubkernelPerfWithComm(SubKernelSchedInfo *SI,
						  unsigned d) {
    return SI->kernelTimes[d] + SI->H2DTimes[d];
  }

  double
  SingleKernelScheduler::getSubkernelPerfWithoutComm(SubKernelSchedInfo *SI,
						     unsigned d) {
    return SI->kernelTimes[d];
  }

};
