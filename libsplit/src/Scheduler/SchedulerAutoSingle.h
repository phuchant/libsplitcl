#ifndef SCHEDULERAUTOSINGLE_H
#define SCHEDULERAUTOSINGLE_H

#include <Scheduler/Scheduler.h>

#include <limits>
#include <map>

namespace libsplit {

  class SchedulerAutoSingle : public Scheduler {
  public:
    SchedulerAutoSingle(BufferManager *buffManager, unsigned nbDevices);
    virtual ~SchedulerAutoSingle();

    virtual void getPartition(KernelHandle *k, /* IN */
			      size_t work_dim, /* IN */
			      const size_t *global_work_offset, /* IN */
			      const size_t *global_work_size, /* IN */
			      const size_t *local_work_size, /* IN */
			      bool *needOtherExecutionToComplete, /* OUT */
			      std::vector<SubKernelExecInfo *> &subkernels, /* OUT */
			      std::vector<DeviceBufferRegion> &dataRequired, /* OUT */
			      std::vector<DeviceBufferRegion> &dataWritten) /* OUT */;

    virtual void setH2DEvents(KernelHandle *k,
			      unsigned devId,
			      std::vector<Event> *events);

    virtual void setD2HEvents(KernelHandle *k,
			      unsigned devId,
			      std::vector<Event> *events);

  protected:
    virtual void initGranu(SubKernelSchedInfo *KI, unsigned id);
    virtual void initScheduler(SubKernelSchedInfo *KI, unsigned id) = 0;
    virtual void getGranu(SubKernelSchedInfo *KI) = 0;

  private:
    void (SchedulerAutoSingle::*updatePerfDescr)(SubKernelSchedInfo *);
    void (SchedulerAutoSingle::*getNextPartition)(SubKernelSchedInfo *);

    void updatePerfDescrWithComm(SubKernelSchedInfo *KI);
    void updatePerfDescrWithoutComm(SubKernelSchedInfo *KI);

    void getNextPartitionFull(SubKernelSchedInfo *KI);
    void getNextPartitionBest(SubKernelSchedInfo *KI);
    void getNextPartitionNil(SubKernelSchedInfo *KI);
  };

};


#endif /* SCHEDULERAUTOSINGLE_H */
