#include <EventFactory.h>
#include <Globals.h>
#include <Scheduler/SchedulerMKStatic.h>
#include <Utils/Debug.h>

#include <iostream>

#include <cstring>

extern "C" {
#include <gsl/gsl_fit.h>
}

namespace libsplit {

  SchedulerMKStatic::SchedulerMKStatic(BufferManager *buffManager, unsigned nbDevices)
    : MultiKernelScheduler(buffManager, nbDevices) {


  }

  SchedulerMKStatic::~SchedulerMKStatic() {
  }

  void
  SchedulerMKStatic::getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete) {
    *needOtherExecToComplete = false;
    kerID2SchedInfoMap[kerId] = SI;

    // PARTITION<id> or PARTITION option
    std::vector<unsigned> *vecParams = &optPartition;
    std::vector<unsigned> vecPartition;

    char envPartition[64];
    sprintf(envPartition, "PARTITION%u", kerId);
    char *partition = getenv(envPartition);
    if (partition) {
      std::cerr << "PARTITION!!\n";
      std::string s(partition);
      std::istringstream is(s);
      int n;
      while (is >> n)
	vecPartition.push_back(n);
      vecParams = &vecPartition;
    }

    if (!vecParams->empty()) {
      int nbSplits = ((*vecParams).size() - 1) / 2;

      SI->req_size_gr = nbSplits * 3;
      SI->real_size_gr = nbSplits * 3;
      delete[] SI->req_granu_dscr;
      delete[] SI->real_granu_dscr;
      SI->req_granu_dscr = new double[SI->req_size_gr];
      SI->real_granu_dscr = new double[SI->req_size_gr];
      SI->size_perf_dscr = SI->req_size_gr;
      delete[] SI->kernel_perf_dscr;
      SI->kernel_perf_dscr = new double[SI->req_size_gr];

      int denominator = (*vecParams)[0];
      int total = 0;
      for (int i=0; i<nbSplits; i++) {
	int nominator = (*vecParams)[1 + 2*i];
	int deviceno = (*vecParams)[2 + 2*i];
	double granu = (double) nominator / denominator;

	SI->req_granu_dscr[i*3] = deviceno;
	SI->req_granu_dscr[i*3+1] = 1;
	SI->req_granu_dscr[i*3+2] = granu;

	SI->kernel_perf_dscr[i*3] = deviceno;
	SI->kernel_perf_dscr[i*3+1] = granu;

	total += nominator;
      }

      if (total != denominator) {
	std::cerr << "Error bad split parameters !\n";
	exit(EXIT_FAILURE);
      }

      return;
    }

    // GRANUDSCR option
    if (!optGranudscr.empty()) {
      SI->req_size_gr = SI->real_size_gr = optGranudscr.size();
      delete[] SI->req_granu_dscr;
      delete[] SI->real_granu_dscr;
      SI->req_granu_dscr = new double[SI->req_size_gr];
      SI->real_granu_dscr = new double[SI->req_size_gr];
      for (int i=0; i<SI->req_size_gr; i++)
	SI->req_granu_dscr[i] = optGranudscr[i];

      return;
    }

    // Uniform splitting by default
    for (unsigned i=0; i<nbDevices; i++) {
      SI->req_granu_dscr[i*3] = i;
      SI->req_granu_dscr[i*3+1] = 1;
      SI->req_granu_dscr[i*3+2] = 1.0 / nbDevices;
      SI->kernel_perf_dscr[i*3] = SI->req_granu_dscr[i*3];
      SI->kernel_perf_dscr[i*3+1] = SI->req_granu_dscr[i*3+2];
      SI->kernel_perf_dscr[i*3+2] = 0;
    }
  }

  void
  SchedulerMKStatic::getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis) {
    (void) SI;
    *needOtherExecToComplete = false;
    *needToInstantiateAnalysis = false;

    if (kerId == 0) {

      double totalCyclePerDevice[nbDevices] = {0};

      for (unsigned k=0; k<cycleLength; k++) {
	SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];

	DEBUG("timers",
	      KSI->updateTimers();
	      KSI->updatePerfDescr();
	      std::cerr << "kernel " << k << "\n";
	      KSI->printTimers();
	      for (unsigned d=0; d<nbDevices; d++) {
		totalCyclePerDevice[d] += KSI->D2HTimes[d] + KSI->H2DTimes[d] +
		  KSI->kernelTimes[d];
	      }
	      );

      }

      DEBUG("timers",
	    for (unsigned d=0; d<nbDevices; d++) {
	      std::cerr << "total iter time on device " << d << ": "
			<< totalCyclePerDevice[d] << "\n";
	    }
	    );

      for (unsigned k=0; k<cycleLength; k++) {
	SubKernelSchedInfo *KSI = kerID2SchedInfoMap[k];
	KSI->clearEvents();
	KSI->clearTimers();
      }

      eventFactory->freeEvents();
    }
  }

  unsigned
  SchedulerMKStatic::getCycleIter() const {
      return count / cycleLength;
  }

};
