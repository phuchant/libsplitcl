#ifndef DEVICEQUEUE_H
#define DEVICEQUEUE_H

#include <Queue/Event.h>
#include <Handle/KernelHandle.h>

#include <CL/cl.h>

#ifdef USE_HWLOC
#include <hwloc.h>
#endif /* USE_HWLOC */

namespace libsplit {

  class Command;

  class DeviceQueue {
  public:
    virtual ~DeviceQueue();

    void enqueueWrite(cl_mem buffer,
		      size_t offset,
		      size_t cb,
		      const void *ptr,
		      Event *event);

    void enqueueRead(cl_mem buffer,
		     size_t offset,
		     size_t cb,
		     const void *ptr,
		     Event *event);

    void enqueueExec(cl_kernel kernel,
		     cl_uint work_dim,
		     const size_t *global_work_offset,
		     const size_t *global_work_size,
		     const size_t *local_work_size,
		     KernelArgs args,
		     Event *event);

    void enqueueFill(cl_mem buffer,
		     const void *pattern,
		     size_t pattern_size,
		     size_t offset,
		     size_t size,
		     Event *event);

    void finish();

    virtual void run() = 0;

    cl_command_queue cl_queue;

  protected:
    DeviceQueue(cl_context context, cl_device_id dev, unsigned dev_id);

    virtual void enqueue(Command *command) = 0;

    void bindThread();
    static void *threadFunc(void *args);

    cl_context context;
    cl_device_id device;
    unsigned dev_id;

    Event *lastEvent;

    bool isCudaDevice;
    bool isAMDDevice;

#ifdef USE_HWLOC
    hwloc_topology_t topology;
    static hwloc_bitmap_t globalSet;
#endif /* USE_HWLOC */
  };

};

#endif /* DEVICEQUEUE_H */
