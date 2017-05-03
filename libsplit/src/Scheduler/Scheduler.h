#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <BufferManager.h>
#include <Queue/Event.h>

#include <vector>

namespace libsplit {

  class KernelHandle;

  struct SubKernelExecInfo {
    unsigned device;
    size_t work_dim;
    size_t global_work_offset[3];
    size_t global_work_size[3];
    size_t local_work_size[3];
    unsigned numgroups;
    unsigned splitdim;
    Event event;
  };

  class Scheduler {
  public:

    enum TYPE {
      BADBROYDEN,
      BROYDEN,
      ENV,
      FIXEDPOINT,
      MKGR,
      SAMPLE,
    };

    Scheduler(BufferManager *buffManager, unsigned nbDevices);
    virtual ~Scheduler();

    virtual void getPartition(KernelHandle *k, /* IN */
			      size_t work_dim, /* IN */
			      const size_t *global_work_offset, /* IN */
			      const size_t *global_work_size, /* IN */
			      const size_t *local_work_size, /* IN */
			      bool *needOtherExecutionToComplete, /* OUT */
			      std::vector<SubKernelExecInfo *> &subkernels, /* OUT */
			      std::vector<DeviceBufferRegion> &dataRequired, /* OUT */
			      std::vector<DeviceBufferRegion> &dataWritten, /* OUT */
			      unsigned *id);

    virtual void setH2DEvents(unsigned kerId,
			      unsigned devId,
			      std::vector<Event> &events);

    virtual void setD2HEvents(unsigned kerID,
			      unsigned devId,
			      std::vector<Event> &events);

  protected:
    BufferManager *buffManager;
    unsigned nbDevices;
    struct SubKernelSchedInfo;
    std::map<unsigned, SubKernelSchedInfo *> kerID2InfoMap;
    unsigned count;

    // This function defines the mapping between a kernel and its
    // SubKernelSchedInfo structure and has to be provided by the scheduler
    // implementation.
    // For single kernel schedulers the ID corresponds to the KernelHandle ID,
    // which means tha one kernel maps to exactly one SubKernelSchedInfo
    // structure.
    // Whereas for multi kernel schedulers it corresponds to the possition in
    // the cycle.
    virtual unsigned getKernelID(KernelHandle *k) = 0;

    virtual void getInitialPartition(SubKernelSchedInfo *SI,
				     unsigned kerId,
				     bool *needOtherExecToComplete) = 0;
    virtual void getNextPartition(SubKernelSchedInfo *SI,
				  unsigned kerId,
				  bool *needOtherExecToComplete,
				  bool *needToInstantiateAnalysis) = 0;


    void updateTimers(SubKernelSchedInfo *SI);
    void printTimers(SubKernelSchedInfo *SI);
    void printPartition(SubKernelSchedInfo *SI);
    virtual void updatePerfDescr(SubKernelSchedInfo *SI);

    // Compute the array of dimension id from the one with
    // the maximum number of splits.
    void getSortedDim(cl_uint work_dim,
		      const size_t *global_work_size,
		      const size_t *local_work_size,
		      unsigned *order);

    bool instantiateAnalysis(KernelHandle *k,
			     cl_uint work_dim,
			     const size_t *global_work_offset,
			     const size_t *global_work_size,
			     const size_t *local_work_size,
			     unsigned splitDim,
			     double *granu_dscr,
			     int *size_gr,
			     std::vector<SubKernelExecInfo *> &subkernels,
			     std::vector<DeviceBufferRegion> &dataRequired,
			     std::vector<DeviceBufferRegion> &dataWritten);


    bool instantiateSingleDeviceAnalysis(KernelHandle *k,
					 unsigned dev,
					 cl_uint work_dim,
					 const size_t *global_work_offset,
					 const size_t *global_work_size,
					 const size_t *local_work_size,
					 std::vector<SubKernelExecInfo *> &subkernels,
					 std::vector<DeviceBufferRegion> &dataRequired,
					 std::vector<DeviceBufferRegion> &dataWritten);


    void adaptGranudscr(double *granu_dscr, int *size_gr,
			size_t global_work_size, size_t local_work_size);





    struct SubKernelSchedInfo {
      SubKernelSchedInfo(unsigned nbDevices) {
	req_size_gr = real_size_gr = size_perf_dscr = nbDevices*3;
	req_granu_dscr = new double[req_size_gr];
	real_granu_dscr = new double[real_size_gr];
	kernel_perf_dscr = new double[size_perf_dscr];

	H2DTimes = new double[nbDevices];
	D2HTimes = new double[nbDevices];
	kernelTimes = new double[nbDevices];

	H2DEvents = new std::vector<Event> [nbDevices];
	D2HEvents = new std::vector<Event> [nbDevices];

	iterno = 0;
      }
      ~SubKernelSchedInfo() {
	delete[] req_granu_dscr;
	delete[] real_granu_dscr;
	delete[] kernel_perf_dscr;
	delete[] H2DTimes;
	delete[] D2HTimes;
	delete[] kernelTimes;
	delete[] H2DEvents;
	delete[] D2HEvents;
      }

      // granularity descriptor
      int req_size_gr;
      double *req_granu_dscr; //  <dev, nb, granu, ... >
      int real_size_gr;
      double *real_granu_dscr; //  <dev, nb, granu, ... >

      // perf descriptor
      int size_perf_dscr;
      double *kernel_perf_dscr;

      // last NDRange;
      cl_uint last_work_dim;
      size_t last_global_work_offset[3];
      size_t last_global_work_size[3];
      size_t last_local_work_size[3];

      // subkernels
      std::vector<SubKernelExecInfo *> subkernels;

      // transfers events
      std::vector<Event> *H2DEvents;
      std::vector<Event> *D2HEvents;

      // timers
      double *H2DTimes;
      double *D2HTimes;
      double *kernelTimes;

      // read and written regions
      std::vector<DeviceBufferRegion> dataRequired;
      std::vector<DeviceBufferRegion> dataWritten;

      // scalar arguments values
      std::vector<int> argsValues;

      // iteration count
      unsigned iterno;
    };

  };
};

#endif /* SCHEDULER_H */
