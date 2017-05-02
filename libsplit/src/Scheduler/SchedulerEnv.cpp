#include <Handle/KernelHandle.h>
#include <Scheduler/SchedulerEnv.h>
#include <Options.h>

namespace libsplit {


  SchedulerEnv::SchedulerEnv(BufferManager *buffManager, unsigned nbDevices)
    : SingleKernelScheduler(buffManager, nbDevices) {}

  SchedulerEnv::~SchedulerEnv() {}


  void
  SchedulerEnv::getInitialPartition(SubKernelSchedInfo *SI, unsigned kerId,
				    bool *needOtherExecutionToComplete) {
    *needOtherExecutionToComplete = false;

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
  SchedulerEnv::getNextPartition(SubKernelSchedInfo *SI,
				 unsigned kerId,
				 bool *needOtherExecutionToComplete,
				 bool *needToInstantiateAnalysis) {
    (void) SI;
    (void) kerId;

    *needOtherExecutionToComplete = false;
    *needToInstantiateAnalysis = false;
  }

};
