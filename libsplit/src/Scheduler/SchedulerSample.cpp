#include <Scheduler/SchedulerSample.h>

#include <iomanip>

#include <gsl/gsl_fit.h>

namespace libsplit {

  SchedulerSample::SchedulerSample(BufferManager *buffManager,
				   unsigned nbDevices)
    : MultiKernelScheduler(buffManager, nbDevices) {
    if (nbDevices > 4 || nbDevices < 2) {
      std::cerr << "Error: nbDevices should be in [2,4] !\n";
      exit(EXIT_FAILURE);
    }
  }

  SchedulerSample::~SchedulerSample() {
  }

  static
  void get2DevicesPartitions(double **partitions, unsigned *size, int nbgr) {
    double step = 1.0 / nbgr;
    unsigned n = nbgr+1;

    *partitions = new double[n * 2];
    *size = n;

    n = 0;
    for (int i=0; i <= nbgr; i++) {
      int j = nbgr-i;
      (*partitions)[n*2+0] = i * step;
      (*partitions)[n*2+1] = j * step;
      n++;
    }
  }

  static
  void get3DevicesPartitions(double **partitions, unsigned *size, int nbgr) {
    double step = 1.0 / nbgr;
    unsigned n = 0;

    for (int i=0; i <= nbgr; i++) {
      for (int j=0; j <= nbgr-i; j++) {
	n++;
      }
    }

    *partitions = new double[n * 3];
    *size = n;

    n = 0;
    for (int i=0; i <= nbgr; i++) {
      for (int j=0; j <= nbgr-i; j++) {
	int k = nbgr-i-j;
	(*partitions)[n*3+0] = i * step;
	(*partitions)[n*3+1] = j * step;
	(*partitions)[n*3+2] = k * step;
	n++;
      }
    }
  }

  static
  void get4DevicesPartitions(double **partitions, unsigned *size, int nbgr) {
    double step = 1.0 / nbgr;
    unsigned n = 0;

    for (int i=0; i <= nbgr; i++) {
      for (int j=0; j <= nbgr-i; j++) {
    	for (int k=0; k <= nbgr-i-j; k ++) {
	  n++;
    	}
      }
    }

    *partitions = new double[n * 4];
    *size = n;

    n = 0;
    for (int i=0; i <= nbgr; i++) {
      for (int j=0; j <= nbgr-i; j++) {
    	for (int k=0; k <= nbgr-i-j; k ++) {
    	  int l = nbgr-i-j-k;
	  (*partitions)[n*4+0] = i * step;
	  (*partitions)[n*4+1] = j * step;
	  (*partitions)[n*4+2] = k * step;
	  (*partitions)[n*4+3] = l * step;
	  n++;
    	}
      }
    }
  }

    // 2 dev, 5 gr -> 5
    // 2 dev, 4 gr -> 4
    // 2 dev, 3 gr -> 3
    // 2 dev, 2 gr -> 2
    // = ngr

    // 3 dev, 5 gr -> 15  = fib(5) = 1 + 2 + 3 + 4 + 5
    // 3 dev, 4 gr -> 10  = fib(4) = 1 + 2 + 3 + 4
    // 3 dev, 3 gr -> 6   = fib(3) = 1 + 2 + 3
    // 3 dev, 2 gr -> 3   = fib(2) = 1 + 2
    // = fib(ngr)

    // 4 dev, 7 gr -> 84 = 1 + 3 + 6 + 10 + 15 + 21 + 28
    // 4 dev, 6 gr -> 56 = 1 + 3 + 6 + 10 + 15 + 21
    // 4 dev, 5 gr -> 35 = 1 + 3 + 6 + 10 + 15
    // 4 dev, 4 gr -> 20 = 1 + 3 + 6 + 10
    // 4 dev, 3 gr -> 10 = 1 + 3 + 6
    // 4 dev, 2 gr ->  4 = 1 + 3
    // = ??

  static
  void getCyclePartitionEnumeration(double **enumeration,
				    int *nbCyclePartitions,
				    int cycleLength, int nbDevices,
				    int nbgr) {
    double *kernelEnum; unsigned kernelEnumSize;
    if (nbDevices == 2) {
      get2DevicesPartitions(&kernelEnum, &kernelEnumSize, nbgr);
    } else if (nbDevices == 3) {
      get3DevicesPartitions(&kernelEnum, &kernelEnumSize, nbgr);
    } else if (nbDevices == 4) {
      get4DevicesPartitions(&kernelEnum, &kernelEnumSize, nbgr);
    } else {
      exit(EXIT_FAILURE);
    }

    // Compute number of cycle partitions and alloc enumeration array
    *nbCyclePartitions = 1;
    for (int i=0; i<cycleLength; i++)
      *nbCyclePartitions *= kernelEnumSize;
    *enumeration = new double[*nbCyclePartitions * cycleLength *nbDevices];

    // Fill enumeration array
    int kerPartitionIdx[cycleLength];
    for (int i=0; i<cycleLength; i++)
      kerPartitionIdx[i] = 0;

    // std::cerr << "nbPartitions per kernel = " << kernelEnumSize << "\n";

    // For each cycle partition
    for (int i=0; i<*nbCyclePartitions; i++) {
      // std::cerr << "computing cycle with ";
      // for (int k=0; k<cycleLength; k++)
      // 	std::cerr << "k" <<k<< "="<<kerPartitionIdx[k] << " ";
      // std::cerr << "\n";

      // For each kernel in the cycle
      for (int c=0; c<cycleLength; c++) {
	// For each device
	for (int d=0; d<nbDevices; d++) {
	  int index = i * cycleLength * nbDevices +
	              c * nbDevices +
	              d;
	  (*enumeration)[index] = kernelEnum[kerPartitionIdx[c]*nbDevices + d];
	}
      }

      // Compute next indexes.
      int last = cycleLength-1;
      while (true) {
	kerPartitionIdx[last] = (kerPartitionIdx[last] + 1) % kernelEnumSize;
	if (kerPartitionIdx[last] != 0)
	  break;
	last -= 1;
	if (last < 0)
	  break;
      }
    }

    delete[] kernelEnum;
  }

  void
  SchedulerSample::getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete) {
    *needOtherExecToComplete = false;
    std::cerr << "Sample count=" << count << " kerId=" << kerId
	      << " iteration=" << getCycleIter() << "\n";

    for (unsigned i=0; i<nbDevices; i++) {
      SI->req_granu_dscr[i*3] = i;
      SI->req_granu_dscr[i*3+1] = 1;
      SI->req_granu_dscr[i*3+2] = 1.0 / nbDevices;
    }

    kerID2SchedInfoMap[kerId] = SI;

  }

  void
  SchedulerSample::getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis) {
    *needOtherExecToComplete = false;
    *needToInstantiateAnalysis = false;

    if (getCycleIter() % 2 != 0)
      return;

    *needToInstantiateAnalysis = true;

    if (kerId != 0)
      return;

    // Compute cycle partitions enumeration.
    double *enumeration; int nbCyclePartitions;
    getCyclePartitionEnumeration(&enumeration, &nbCyclePartitions,
				 cycleLength, nbDevices, optSampleSteps);
    std::cerr << "nbCyclePartitions = " << nbCyclePartitions << "\n";
    std::cout.precision(2);
    std::cout << std::fixed;

    // Dump header
    dumpHeader();

    // For each cycle partition, compute D2H and H2D tranfers for each
    // devices and dump results.
    for (int i=0; i<nbCyclePartitions; i++) {
      // Compute dataRequired and data Written by each kernel in the cycle
      instantiateCycleAnalysis(&enumeration[i*cycleLength*nbDevices]);

      // Simulate buffer transfers for one execution of the cycle.
      simulateOneCycleTransfers();

      // Compute required transfers for a second execution.
      size_t *D2HPerKernel = new size_t[cycleLength * nbDevices]();
      size_t *H2DPerKernel = new size_t[cycleLength * nbDevices]();
      computeOneCycleDataTransfers(D2HPerKernel, H2DPerKernel);


      // Dump cycle results.
      dumpCycleResults(D2HPerKernel, H2DPerKernel,
		       &enumeration[i*cycleLength*nbDevices]);

      delete[] D2HPerKernel;
      delete[] H2DPerKernel;
    }

    delete[] enumeration;

    exit(EXIT_SUCCESS);
  }

  unsigned
  SchedulerSample::getCycleIter() const {
      return count / cycleLength;
  }

  void
  SchedulerSample::simulateOneCycleTransfers() {
    for (unsigned k=0; k<cycleLength; k++) {
      SubKernelSchedInfo *SI = kerID2SchedInfoMap[k];
      std::vector<DeviceBufferRegion> D2HTranfers;
      std::vector<DeviceBufferRegion> H2DTranfers;
      buffManager->computeTransfers(SI->dataRequired, D2HTranfers, H2DTranfers);

      // Validate data read from device onto host buffer
      for (unsigned i=0; i<D2HTranfers.size(); i++) {
	MemoryHandle *m = D2HTranfers[i].m;
	m->hostValidData.myUnion(D2HTranfers[i].region);
      }

      // Validate data sent to devices onto devices buffer
      for (unsigned i=0; i<H2DTranfers.size(); i++) {
	MemoryHandle *m = H2DTranfers[i].m;
	unsigned d = H2DTranfers[i].devId;
	m->devicesValidData[d].myUnion(H2DTranfers[i].region);
      }

      // Update valid data after subkernels executions
      for (unsigned i=0; i<SI->dataWritten.size(); i++) {
	MemoryHandle *m = SI->dataWritten[i].m;
	unsigned d = SI->dataWritten[i].devId;
	m->devicesValidData[d].myUnion(SI->dataWritten[i].region);
	m->hostValidData.difference(SI->dataWritten[i].region);
	for (unsigned d2=0; d2<nbDevices; d2++) {
	  if (d == d2)
	    continue;
	  m->devicesValidData[d2].difference(SI->dataWritten[i].region);
	}
      }
    }
  }

  void
  SchedulerSample::computeOneCycleDataTransfers(size_t *D2HPerKernel,
						size_t *H2DPerKernel) {
    for (unsigned k=0; k<cycleLength; k++) {
      std::vector<DeviceBufferRegion> D2HTranfers;
      std::vector<DeviceBufferRegion> H2DTranfers;
      SubKernelSchedInfo *SI = kerID2SchedInfoMap[k];
      buffManager->computeTransfers(SI->dataRequired,
				    D2HTranfers, H2DTranfers);
      for (unsigned i=0; i<D2HTranfers.size(); i++) {
	unsigned d = D2HTranfers[i].devId;
	size_t size = D2HTranfers[i].region.total();
	D2HPerKernel[k*nbDevices+d] += size;
	MemoryHandle *m = D2HTranfers[i].m;
	m->hostValidData.myUnion(D2HTranfers[i].region);
      }

      for (unsigned i=0; i<H2DTranfers.size(); i++) {
	unsigned d = H2DTranfers[i].devId;
	size_t size = H2DTranfers[i].region.total();
	H2DPerKernel[k*nbDevices+d] += size;
	MemoryHandle *m = H2DTranfers[i].m;
	m->devicesValidData[d].myUnion(H2DTranfers[i].region);
      }

      // Update valid data after subkernels executions
      for (unsigned i=0; i<SI->dataWritten.size(); i++) {
	MemoryHandle *m = SI->dataWritten[i].m;
	unsigned d = SI->dataWritten[i].devId;
	m->devicesValidData[d].myUnion(SI->dataWritten[i].region);
	m->hostValidData.difference(SI->dataWritten[i].region);
	for (unsigned d2=0; d2<nbDevices; d2++) {
	  if (d == d2)
	    continue;
	  m->devicesValidData[d2].difference(SI->dataWritten[i].region);
	}
      }
    }
  }

  void
  SchedulerSample::dumpHeader() {
    std::cout << "#";
    for (unsigned k=0; k<cycleLength; k++) {
      for (unsigned d=0; d<nbDevices; d++)
	std::cout<<"k"<<k<<"d"<<d<<",";
      for (unsigned d=0; d<nbDevices; d++)
	std::cout<<"D"<<d<<"toH,";
      for (unsigned d=0; d<nbDevices; d++)
	std::cout<<"HtoD"<<d<<",";
    }
    std::cout << "\n";
  }

  void
  SchedulerSample::dumpCycleResults(size_t *D2HPerKernel, size_t *H2DPerKernel,
				    double *cycleGranus) {
    for (unsigned k=0; k<cycleLength; k++) {
      for (unsigned d=0; d<nbDevices; d++)
	std::cout << cycleGranus[k*nbDevices+d] << ",";
      for (unsigned d=0; d<nbDevices; d++)
	std::cout << D2HPerKernel[k*nbDevices+d] << ",";
      for (unsigned d=0; d<nbDevices; d++)
	std::cout << H2DPerKernel[k*nbDevices+d] << ",";
    }
    std::cout << "\n";
  }

  void
  SchedulerSample::instantiateCycleAnalysis(double *cycleGranus) {
    for (unsigned k=0; k<cycleLength; k++) {
      SubKernelSchedInfo *SI = kerID2SchedInfoMap[k];
      double granu_dscr[nbDevices*3];
      int size_gr = nbDevices*3;
      for (unsigned d=0; d<nbDevices; d++) {
	granu_dscr[d*3+0] = d;
	granu_dscr[d*3+1] = 1;
	granu_dscr[d*3+2] = cycleGranus[k*nbDevices+d];
      }
      KernelHandle *handle = kerID2HandleMap[k];
      unsigned splitDim = SI->subkernels[0]->splitdim;
      SI->dataRequired.clear();
      SI->dataWritten.clear();
      for (unsigned ii=0; ii<SI->subkernels.size(); ii++)
	delete SI->subkernels[ii];
      SI->subkernels.clear();
      instantiateAnalysis(handle,
			  SI->last_work_dim,
			  (const size_t *) SI->last_global_work_offset,
			  (const size_t *) SI->last_global_work_size,
			  (const size_t *) SI->last_local_work_size,
			  splitDim,
			  granu_dscr,
			  &size_gr,
			  SI->subkernels,
			  SI->dataRequired,
			  SI->dataWritten);
    }
  }

};

