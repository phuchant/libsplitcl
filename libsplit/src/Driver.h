#ifndef DRIVER_H
#define DRIVER_H

#include <BufferManager.h>
#include <Handle/KernelHandle.h>
#include <Handle/MemoryHandle.h>

namespace libsplit {

  class Scheduler;
  class SubKernelExecInfo;

  class Driver {
  public:
    Driver();
    virtual ~Driver();

    void enqueueReadBuffer(cl_command_queue queue,
			   MemoryHandle *m,
			   cl_bool blocking,
			   size_t offset,
			   size_t size,
			   void *ptr,
			   cl_uint num_events_in_wait_list,
			   const cl_event *event_wait_list,
			   cl_event *event);

    void enqueueWriteBuffer(cl_command_queue queue,
			    MemoryHandle *m,
			    cl_bool blocking,
			    size_t offset,
			    size_t size,
			    const void *ptr,
			    cl_uint num_events_in_wait_list,
			    const cl_event *event_wait_list,
			    cl_event *event);

    void enqueueCopyBuffer(cl_command_queue queue,
			   MemoryHandle *src,
			   MemoryHandle *dst,
			   size_t src_offset,
			   size_t dst_offset,
			   size_t size,
			   cl_uint num_events_in_wait_list,
			   const cl_event *event_wait_list,
			   cl_event *event);

    void enqueueNDRangeKernel(cl_command_queue queue,
			      KernelHandle *k,
			      cl_uint work_dim,
			      const size_t *global_work_offset,
			      const size_t *global_work_size,
			      const size_t *local_work_size,
			      cl_uint num_events_in_wait_list,
			      const cl_event *event_wait_list,
			      cl_event *event);

  private:
    Scheduler *scheduler;
    BufferManager *bufferMgr;

    void startD2HTransfers(unsigned kerId,
			   const std::vector<DeviceBufferRegion> &transferList);
    void startH2DTransfers(unsigned kerId,
			   const std::vector<DeviceBufferRegion> &transferList);
    void startOrD2HTransfers(unsigned kerId,
			     const std::vector<DeviceBufferRegion>
			     &transferList);

    void enqueueSubKernels(KernelHandle *k,
			   std::vector<SubKernelExecInfo *> &subkernels,
			   const std::vector<DeviceBufferRegion> &dataWritten);

    void performHostOrVariableReduction(const std::vector<DeviceBufferRegion> &
					transferList);
  };

};

#endif /* DRIVER_H */
