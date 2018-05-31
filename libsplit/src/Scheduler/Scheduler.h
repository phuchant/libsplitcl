#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <BufferManager.h>
#include <Queue/Event.h>
#include <IndexExpr/IndexExprValue.h>
#include <NDRange.h>

#include <set>
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
    Event *event;
  };

  class Scheduler {
  public:

    enum TYPE {
      BADBROYDEN,
      BROYDEN,
      ENV,
      FIXEDPOINT,
      MKGR,
      MKSTATIC,
      SAMPLE,
    };

    Scheduler(BufferManager *buffManager, unsigned nbDevices);
    virtual ~Scheduler();


    virtual void
    getIndirectionRegions(KernelHandle *k, /* IN */
			  size_t work_dim, /* IN */
			  const size_t *global_work_offset, /* IN */
			  const size_t *global_work_size, /* IN */
			  const size_t *local_work_size, /* IN */
			  std::vector<BufferIndirectionRegion> &regions /* OUT */);

    virtual bool
    setIndirectionValues(KernelHandle *k, /* IN */
			 const std::vector<BufferIndirectionRegion> &values /* IN */);


    virtual void getPartition(KernelHandle *k, /* IN */
			      bool *needOtherExecutionToComplete, /* OUT */
			      std::vector<SubKernelExecInfo *> &subkernels, /* OUT */
			      std::vector<DeviceBufferRegion> &dataRequired, /* OUT */
			      std::vector<DeviceBufferRegion> &dataWritten, /* OUT */
			      std::vector<DeviceBufferRegion> &dataWrittenMerge, /* OUT */
			      std::vector<DeviceBufferRegion> &dataWrittenOr, /* OUT */
			      std::vector<DeviceBufferRegion> &dataWrittenAtomicSum, /* OUT */
			      std::vector<DeviceBufferRegion> &dataWrittenAtomicMin, /* OUT */
			      std::vector<DeviceBufferRegion> &dataWrittenAtomicMax, /* OUT */
			      unsigned *id);

    virtual void setH2DEvent(unsigned srcId,
			     unsigned dstId,
			     unsigned devId,
			     Event *event);

    virtual void setD2HEvent(unsigned srcId,
			      unsigned dstId,
			      unsigned devId,
			      Event *event);

    virtual void setBufferRequired(unsigned kerId, MemoryHandle *m);

  protected:
    BufferManager *buffManager;
    unsigned nbDevices;
    struct SubKernelSchedInfo;
    std::map<unsigned, SubKernelSchedInfo *> kerID2InfoMap;
    unsigned count;
    const int GRANU2INTFACTOR = 1000000;

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


    void printPartition(SubKernelSchedInfo *SI);

    // Compute the array of dimension id from the one with
    // the maximum number of splits.
    void getSortedDim(cl_uint work_dim,
		      const size_t *global_work_size,
		      const size_t *local_work_size,
		      unsigned order[3]);

    bool fillSubkernelInfoMulti(KernelHandle *k,
				double *granu_dscr,
				int *size_gr,
				unsigned splitDim,
				std::vector<SubKernelExecInfo *> &subkernels,
				std::vector<DeviceBufferRegion> &dataRequired,
				std::vector<DeviceBufferRegion> &dataWritten,
				std::vector<DeviceBufferRegion> &dataWrittenMerge,
				std::vector<DeviceBufferRegion> &dataWrittenOr,
				std::vector<DeviceBufferRegion> &dataWrittenAtomicSum,
				std::vector<DeviceBufferRegion> &dataWrittenAtomicMin,
				std::vector<DeviceBufferRegion> &dataWrittenAtomicMax);

    bool fillSubkernelInfoShift(KernelHandle *k,
				SubKernelSchedInfo *SI,
				const NDRange &origNDRange,
				const std::vector<NDRange> &subNDRanges,
				double *granu_dscr,
				int *size_gr,
				unsigned splitDim,
				std::vector<SubKernelExecInfo *> &subkernels,
				unsigned nbGlobals,
				std::vector<DeviceBufferRegion> &dataRequired,
				std::vector<DeviceBufferRegion> &dataWritten,
				std::vector<DeviceBufferRegion> &dataWrittenMerge,
				std::vector<DeviceBufferRegion> &dataWrittenOr,
				std::vector<DeviceBufferRegion> &dataWrittenAtomicSum,
				std::vector<DeviceBufferRegion> &dataWrittenAtomicMin,
				std::vector<DeviceBufferRegion> &dataWrittenAtomicMax);

    bool fillSubkernelInfoSingle(KernelHandle *k,
				 double *granu_dscr,
				 int *size_gr,
				 unsigned splitDim,
				 std::vector<SubKernelExecInfo *> &subkernels,
				 std::vector<DeviceBufferRegion> &dataRequired,
				 std::vector<DeviceBufferRegion> &dataWritten,
				 std::vector<DeviceBufferRegion> &dataWrittenMerge,
				 std::vector<DeviceBufferRegion> &dataWrittenOr,
				 std::vector<DeviceBufferRegion> &dataWrittenAtomicSum,
				 std::vector<DeviceBufferRegion> &dataWrittenAtomicMin,
				 std::vector<DeviceBufferRegion> &dataWrittenAtomicMax);

    void adaptGranudscr(double *granu_dscr, int *size_gr,
			size_t global_work_size, size_t local_work_size);

    static
    void getShiftedPartition(std::vector<NDRange> *shiftedPartition,
			     const NDRange &origNDRange,
			     const NDRange &ndRange,
			     unsigned splitDim,
			     unsigned requiredShift,
			     unsigned *shiftedDeviceIdInPartition);

    static
    bool paramHaveChanged(const SubKernelSchedInfo *SI,
			    const KernelHandle *k);
    static
    void updateParamValues(SubKernelSchedInfo *SI, const KernelHandle *k);

    struct SubKernelSchedInfo {
      SubKernelSchedInfo(KernelHandle *handle, unsigned nbDevices)
      : handle(handle), hasInitPartition(false), hasPartition(false),
	partitionUnchanged(false),
	needOtherExecToComplete(false),
	needToInstantiateAnalysis(true),
	currentDim(0),
	nbDevices(nbDevices),
	shiftingPartition(false),
	nbMergeArgs(0),
	origNDRange(nullptr) {
	req_size_gr = real_size_gr = size_perf_dscr = nbDevices*3;
	req_granu_dscr = new double[req_size_gr];
	real_granu_dscr = new double[real_size_gr];
	granu_intervals = new ListInterval[nbDevices];
	kernel_perf_dscr = new double[size_perf_dscr];

	H2DTimes = new double[nbDevices];
	D2HTimes = new double[nbDevices];
	kernelTimes = new double[nbDevices];

	src2H2DEvents = new std::map<int, std::vector<Event *> >[nbDevices];
	src2D2HEvents = new std::map<int, std::vector<Event *> >[nbDevices];

	src2H2DTimes = new std::map<int, double>[nbDevices];
	src2D2HTimes = new std::map<int, double>[nbDevices];

	iterno = 0;
      }
      ~SubKernelSchedInfo() {
	delete[] req_granu_dscr;
	delete[] real_granu_dscr;
	delete[] granu_intervals;
	delete[] kernel_perf_dscr;
	delete[] H2DTimes;
	delete[] D2HTimes;
	delete[] kernelTimes;
	delete[] src2H2DEvents;
	delete[] src2D2HEvents;
	delete[] src2H2DTimes;
	delete[] src2D2HTimes;
      }

      void updateTimers();
      void clearEvents();
      void clearTimers();
      void updatePerfDescr();
      void printTimers() const;

      KernelHandle *handle;

      bool hasInitPartition;
      bool hasPartition;
      bool partitionUnchanged;
      bool needOtherExecToComplete;
      bool needToInstantiateAnalysis;

      unsigned currentDim;
      unsigned dimOrder[3];

      unsigned nbDevices;

      // granularity descriptor
      int req_size_gr;
      double *req_granu_dscr; //  <dev, nb, granu, ... >
      int real_size_gr;
      double *real_granu_dscr; //  <dev, nb, granu, ... >
      ListInterval *granu_intervals;

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
      std::map<int, std::vector<Event *> > *src2H2DEvents;
      std::map<int, std::vector<Event *> > *src2D2HEvents;

      // Buffers required
      std::set<MemoryHandle *>buffersRequired;

      // timers
      double *H2DTimes;
      double *D2HTimes;
      double *kernelTimes;
      std::map<int, double> *src2H2DTimes;
      std::map<int, double> *src2D2HTimes;

      // shifting
      unsigned shiftingPartition;
      unsigned shiftingDevice;
      unsigned shiftedDeviceIdInPartition;
      unsigned shiftingWgs;
      unsigned nbMergeArgs;
      std::map<unsigned, unsigned> mergeArg2GlobalPos;
      std::map<unsigned, ListInterval *> fullWrittenRegion;
      NDRange *origNDRange;
      std::vector<NDRange> requiredSubNDRanges;
      std::vector<NDRange> shiftedSubNDRanges;

      // read and written regions
      std::vector<DeviceBufferRegion> dataRequired;
      std::vector<DeviceBufferRegion> dataWritten;
      std::vector<DeviceBufferRegion> dataWrittenMerge;
      std::vector<DeviceBufferRegion> dataWrittenOr;
      std::vector<DeviceBufferRegion> dataWrittenAtomicSum;
      std::vector<DeviceBufferRegion> dataWrittenAtomicMin;
      std::vector<DeviceBufferRegion> dataWrittenAtomicMax;

      // read and written shift regions
      std::vector<std::vector<ListInterval> > shiftDataRequired;
      std::vector<std::vector<ListInterval> > shiftDataWritten;
      std::vector<std::vector<ListInterval> > shiftDataWrittenOr;
      std::vector<std::vector<ListInterval> > shiftDataWrittenAtomicSum;
      std::vector<std::vector<ListInterval> > shiftDataWrittenAtomicMin;
      std::vector<std::vector<ListInterval> > shiftDataWrittenAtomicMax;

      // Arguments values
      std::vector<IndexExprValue *> argsValues;

      // iteration count
      unsigned iterno;
    };

  };
};

#endif /* SCHEDULER_H */
