#include <Handle/KernelHandle.h>
#include <Scheduler/SchedulerAutoSingle.h>
#include <Options.h>

#include <sstream>

#include <cassert>

namespace libsplit {

  SchedulerAutoSingle::SchedulerAutoSingle(BufferManager *buffManager,
					   unsigned nbDevices)
    : Scheduler(buffManager, nbDevices) {
    if (optNoComm)
      updatePerfDescr = &SchedulerAutoSingle::updatePerfDescrWithoutComm;
    else
      updatePerfDescr = &SchedulerAutoSingle::updatePerfDescrWithComm;

    if (optBest)
      getNextPartition = &SchedulerAutoSingle::getNextPartitionBest;
    else
      getNextPartition = &SchedulerAutoSingle::getNextPartitionFull;
  }

  SchedulerAutoSingle::~SchedulerAutoSingle() {
  }

  void
  SchedulerAutoSingle::getPartition(KernelHandle *k, /* IN */
				    size_t work_dim, /* IN */
				    const size_t *global_work_offset, /* IN */
				    const size_t *global_work_size, /* IN */
				    const size_t *local_work_size, /* IN */
				    bool *needOtherExecutionToComplete, /* OUT */
				    std::vector<SubKernelInfo *> &subkernels, /* OUT */
				    std::vector<DeviceBufferRegion> &dataRequired, /* OUT */
				    std::vector<DeviceBufferRegion> &dataWritten) /* OUT */ {
    *needOtherExecutionToComplete = false;

    bool needToInstantiateAnalysis = false;

    KernelInfo *KI = NULL;

    // Create KernelInfo if it doesn't exists yet.
    if (kernel2InfoMap.find(k) == kernel2InfoMap.end()) {
      KI = new KernelInfo(nbDevices);
      kernel2InfoMap[k] = KI;
      initGranu(KI, k->getId());
      initScheduler(KI, k->getId());
      needToInstantiateAnalysis = true;
      std::cerr << "kernel perf init : ";
      for (int i=0; i<KI->size_perf_dscr; i++)
	std::cerr << KI->kernel_perf_dscr[i] << " ";
      std::cerr << "\n";

    } else {
      KI = kernel2InfoMap[k];

      std::cerr << "kernel perf avant : ";
      for (int i=0; i<KI->size_perf_dscr; i++)
	std::cerr << KI->kernel_perf_dscr[i] << " ";
      std::cerr << "\n";


      // Fill kernel perf
      for (unsigned i=0; i<KI->subkernels.size(); i++) {
	cl_ulong start, end;
	cl_int err;
	unsigned dev = KI->subkernels[i]->device;
	err = real_clGetEventProfilingInfo(KI->subkernels[i]->event->event,
					   CL_PROFILING_COMMAND_START,
					   sizeof(start), &start, NULL);
	clCheck(err, __FILE__, __LINE__);
	err = real_clGetEventProfilingInfo(KI->subkernels[i]->event->event,
					   CL_PROFILING_COMMAND_END,
					   sizeof(end), &end, NULL);
	clCheck(err, __FILE__, __LINE__);
	KI->subkernels[i]->event->release();
	std::cerr << "dev " << dev << " time : " << (end - start) * 1e-6 << "\n";
	KI->kernel_perf_dscr[dev*3+2] += (end - start) * 1e-6;
      }

      std::cerr << "kernel perf: ";
      for (int i=0; i<KI->size_perf_dscr; i++)
	std::cerr << KI->kernel_perf_dscr[i] << " ";
      std::cerr << "\n";

      // Get next granu
      KI->iterno++;
      (this->*getNextPartition)(KI);
      needToInstantiateAnalysis = true;

      // Its up to getGranu() to reset kernel_perfs, kernel_time, comm_time.
    }


    std::cerr << "granu: ";
    for (int i=0; i<KI->size_gr; i++)
      std::cerr << KI->granu_dscr[i] << " ";
    std::cerr << "\n";

    // Check if the analysis needs to be instantiated.
    if (!needToInstantiateAnalysis) {
      std::vector<int> currentArgsValues = k->getArgsValuesAsInt();
      for (unsigned i=0; i<KI->argsValues.size(); ++i) {
	if (KI->argsValues[i] != currentArgsValues[i]) {
	  needToInstantiateAnalysis = true;
	  break;
	}
      }
    }

    // Instantiate analysis
    if (needToInstantiateAnalysis) {
      KI->dataRequired.clear();
      KI->dataWritten.clear();
      for (unsigned i=0; i<KI->subkernels.size(); i++) {
	delete KI->subkernels[i];
      }
      KI->subkernels.clear();

      unsigned dimOrder[work_dim];
      getSortedDim(work_dim, global_work_size, local_work_size, dimOrder);
      bool canSplit = false;


      for (unsigned i=0; i<work_dim; i++) {
	canSplit =
	  instantiateAnalysis(k, work_dim, global_work_offset, global_work_size,
			      local_work_size, dimOrder[i], KI->granu_dscr,
			      &KI->size_gr, KI->subkernels, KI->dataRequired,
			      KI->dataWritten);
	if (canSplit)
	  break;
      }

      if (!canSplit) {
	std::cerr << "Error: cannot split kernel !\n";
      }

      KI->argsValues = k->getArgsValuesAsInt();

      // Update perf dscr
      KI->size_perf_dscr = KI->size_gr;
      for (int i=0; i<KI->size_perf_dscr/3; i++) {
	KI->kernel_perf_dscr[i*3] = KI->granu_dscr[i*3]; // dev
	KI->kernel_perf_dscr[i*3+1] = KI->granu_dscr[i*3+2]; // granu
      }
    }

    // Reset kernel and comm times
    for (unsigned i=0; i<nbDevices; i++) {
      KI->D2HTimes[i] = 0;
      KI->H2DTimes[i] = 0;
      KI->kernelTimes[i] = 0;
    }

    // Set output
    for (unsigned i=0; i<KI->subkernels.size(); i++)
      subkernels.push_back(KI->subkernels[i]);
    dataRequired = KI->dataRequired;
    dataWritten = KI->dataWritten;
    // First execition
    /*
      initgranu
      instantiate
      initscheduler
      return
     */

    // else
    // getNextGranu
  }

  void
  SchedulerAutoSingle::setH2devents(KernelHandle *k,
				    unsigned devId,
				    std::vector<Event> *events) {
    assert(kernel2InfoMap.find(k) != kernel2InfoMap.end());
    KernelInfo *KI = kernel2InfoMap[k];

    delete KI->H2DEvents[devId];
    KI->H2DEvents[devId] = events;
  }

  void
  SchedulerAutoSingle::setD2HEvents(KernelHandle *k,
				    unsigned devId,
				    std::vector<Event> *events) {
    assert(kernel2InfoMap.find(k) != kernel2InfoMap.end());
    KernelInfo *KI = kernel2InfoMap[k];

    delete KI->D2HEvents[devId];
    KI->D2HEvents[devId] = events;
  }

  void
  SchedulerAutoSingle::initGranu(KernelInfo *KI, unsigned id) {
    char granustartEnv[64];
    sprintf(granustartEnv, "GRANUSTART%u", id);
    char *granustart = getenv(granustartEnv);

    if (!optGranustart.empty()) {
      KI->size_gr = optGranustart.size();
      delete[] KI->granu_dscr;
      KI->granu_dscr = new double[KI->size_gr];
      for (int i=0; i<KI->size_gr; i++)
	KI->granu_dscr[i] = optGranustart[i];

      KI->size_perf_dscr = KI->size_gr;
      delete[] KI->kernel_perf_dscr;
      KI->kernel_perf_dscr = new double[KI->size_gr];
      for (int i = 0; i<KI->size_gr/3; i++) {
	KI->kernel_perf_dscr[i*3] = KI->granu_dscr[i*3];
	KI->kernel_perf_dscr[i*3+1] = KI->granu_dscr[i*3+2];
	KI->kernel_perf_dscr[i*3+2] = 0;
      }
    }

    //  GRANUSTART X option
    else if (granustart) {
      std::vector<double> optGranuStartX;
      std::string s(granustart);
      std::istringstream is(s);
      double n;
      while (is >> n)
	optGranuStartX.push_back(n);

      KI->size_gr = optGranuStartX.size();
      delete[] KI->granu_dscr;
      KI->granu_dscr = new double[KI->size_gr];
      for (int i=0; i<KI->size_gr; ++i)
	KI->granu_dscr[i] = optGranuStartX[i];

      KI->size_perf_dscr = KI->size_gr;
      delete[] KI->kernel_perf_dscr;
      KI->kernel_perf_dscr = new double[KI->size_gr];
      for (int i = 0; i<KI->size_gr/3; i++) {
	KI->kernel_perf_dscr[i*3] = KI->granu_dscr[i*3];
	KI->kernel_perf_dscr[i*3+1] = KI->granu_dscr[i*3+2];
	KI->kernel_perf_dscr[i*3+2] = 0;
      }
    }

    // Homogeneous splitting
    else {
      std::cerr << "homogeneous!!\n";
      KI->size_gr = nbDevices * 3;
      KI->size_perf_dscr = nbDevices * 3;
      for (unsigned i=0; i<nbDevices; i++) {
	KI->granu_dscr[i*3] = i; // dev id
	KI->granu_dscr[i*3+1] = 1; // nb kernels
	KI->granu_dscr[i*3+2] = 1.0 / nbDevices; // granu
      }
      delete[] KI->kernel_perf_dscr;
      KI->kernel_perf_dscr = new double[KI->size_gr];
      for (int i = 0; i<KI->size_gr/3; i++) {
	KI->kernel_perf_dscr[i*3] = KI->granu_dscr[i*3];
	KI->kernel_perf_dscr[i*3+1] = KI->granu_dscr[i*3+2];
	KI->kernel_perf_dscr[i*3+2] = 0;
      }
    }
  }

  void
  SchedulerAutoSingle::updatePerfDescrWithComm(KernelInfo *KI) {
    // Compute D2H and H2D Times.
    for (unsigned d=0; d<nbDevices; d++) {
      if (KI->H2DEvents[d]) {
	for (unsigned i=0; i<KI->H2DEvents[d]->size(); ++i) {
	  cl_ulong start, end;
	  cl_int err;
	  err = real_clGetEventProfilingInfo((*KI->H2DEvents[d])[i]->event,
					     CL_PROFILING_COMMAND_START,
					     sizeof(start), &start, NULL);
	  clCheck(err, __FILE__, __LINE__);
	  err = real_clGetEventProfilingInfo((*KI->H2DEvents[d])[i]->event,
	  					 CL_PROFILING_COMMAND_END,
	  					 sizeof(end), &end, NULL);
	  clCheck(err, __FILE__, __LINE__);
	  (*KI->H2DEvents[d])[i]->release();
	  KI->H2DTimes[d] += (end - start) * 1e-6;
	}
      }

      if (KI->D2HEvents[d]) {
	for (unsigned i=0; i<KI->D2HEvents[d]->size(); ++i) {
	  cl_ulong start, end;
	  cl_int err;
	  err = real_clGetEventProfilingInfo((*KI->D2HEvents[d])[i]->event,
					     CL_PROFILING_COMMAND_START,
					     sizeof(start), &start, NULL);
	  clCheck(err, __FILE__, __LINE__);
	  err = real_clGetEventProfilingInfo((*KI->D2HEvents[d])[i]->event,
	  					 CL_PROFILING_COMMAND_END,
	  					 sizeof(end), &end, NULL);
	  clCheck(err, __FILE__, __LINE__);
	  (*KI->D2HEvents[d])[i]->release();
	  KI->D2HTimes[d] += (end - start) * 1e-6;
	}
      }
    }
    double longest = 0;
    int nbSplits = KI->size_gr / 3;

    for (int i=0; i<nbSplits; i++) {
      double time = KI->kernelTimes[(int) KI->granu_dscr[i*3]] +
	KI->H2DTimes[(int) KI->granu_dscr[i*3]];
      KI->kernel_perf_dscr[i * 3 + 2] = time;
      longest = time > longest ? time : longest;
    }

    if (longest <KI->bestTime) {
      KI->bestTime = longest;
      KI->best_granu_size = KI->size_gr;
      std::copy(KI->granu_dscr, KI->granu_dscr + KI->size_gr,
		KI->best_granu_dscr);
    } else {
      KI->bestReached = true;

      if (KI->stopBest) {
	for (int i=0; i<KI->size_gr; i++) {
	  std::cerr << KI->granu_dscr[i] << " ";
	}
	std::cerr << "\n";
	exit(0);
      }
    }
  }

  void
  SchedulerAutoSingle::updatePerfDescrWithoutComm(KernelInfo *KI) {
    double longest = 0;
    int nbSplits = KI->size_gr / 3;

    for (int i=0; i<nbSplits; i++) {
      double time = KI->kernelTimes[(int) KI->granu_dscr[i*3]];
      KI->kernel_perf_dscr[i * 3 + 2] = time;
      longest = time > longest ? time : longest;
    }

    if (longest <KI->bestTime) {
      KI->bestTime = longest;
      KI->best_granu_size = KI->size_gr;
      std::copy(KI->granu_dscr, KI->granu_dscr + KI->size_gr,
		KI->best_granu_dscr);
    } else {
      KI->bestReached = true;

      if (KI->stopBest) {
	for (int i=0; i<KI->size_gr; i++) {
	  std::cerr << KI->granu_dscr[i] << " ";
	}
	std::cerr << "\n";
	exit(0);
      }
    }
  }

  void
  SchedulerAutoSingle::getNextPartitionFull(KernelInfo *KI) {
    getGranu(KI);
  }

  void
  SchedulerAutoSingle::getNextPartitionBest(KernelInfo *KI) {
    if (!KI->bestReached) {
      getGranu(KI);
    } else {
      KI->size_gr = KI->best_granu_size;
      std::copy(KI->granu_dscr, KI->granu_dscr + KI->size_gr,
		KI->best_granu_dscr);
      getNextPartition = &SchedulerAutoSingle::getNextPartitionNil;
    }
  }

  void
  SchedulerAutoSingle::getNextPartitionNil(KernelInfo *KI) {
    (void) KI;
    // Do nothing.
  }

};
