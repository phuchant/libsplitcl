#include <Queue/Command.h>
#include <Queue/DeviceQueue.h>
#include <Globals.h>

#include <cstring>

#ifdef USE_HWLOC

#ifdef CUDA
#include <hwloc/cudart.h>
#endif /* CUDA */

#include <hwloc/opencl.h>

#endif /* USE_HWLOC */

namespace libsplit {

#ifdef USE_HWLOC
  hwloc_bitmap_t DeviceQueue::globalSet = hwloc_bitmap_alloc();
#endif /* USE_HWLOC */


  DeviceQueue::DeviceQueue(cl_context context, cl_device_id dev, unsigned dev_id)
    : cl_queue(NULL), dev_id(dev_id), context(context), device(dev),
      lastEvent(NULL), isCudaDevice(false), isAMDDevice(false) {
    size_t vendor_len;
    cl_int err;
    char *vendor;

    // Check if it is a CUDA or AMD device
    err = real_clGetDeviceInfo(dev, CL_DEVICE_VENDOR, 0, NULL, &vendor_len);
    clCheck(err, __FILE__, __LINE__);
    vendor = (char *) malloc(vendor_len);
    err = real_clGetDeviceInfo(dev, CL_DEVICE_VENDOR, vendor_len, vendor, NULL);
    if (!strcmp(vendor, "NVIDIA Corporation"))
      isCudaDevice = true;
    if (!strcmp(vendor, "Advanced Micro Devices, Inc."))
      isAMDDevice = true;
    free(vendor);

#ifdef USE_HWLOC
    // HWLOC initialization
    hwloc_topology_init(&topology);
    hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_IO_DEVICES);
    hwloc_topology_load(topology);
#endif /* USE_HWLOC */
  }

  DeviceQueue::~DeviceQueue() {
    cl_int err = real_clReleaseCommandQueue(cl_queue);
    clCheck(err, __FILE__, __LINE__);

#ifdef USE_HWLOC
    hwloc_topology_destroy(topology);
#endif /* USE_HWLOC */
  }

  void
  DeviceQueue::enqueueDummyEvents() {
    Event *dummyEvent = eventFactory->getNewEvent();
    cl_int err = real_clEnqueueMarker(cl_queue, &dummyEvent->event);
    clCheck(err, __FILE__, __LINE__);
    err = real_clFinish(cl_queue);
    clCheck(err, __FILE__, __LINE__);
    timeline->pushH2DEvent(dummyEvent, dev_id);
  }

  void
  DeviceQueue::enqueueWrite(cl_mem buffer,
			    size_t offset,
			    size_t cb,
			    const void *ptr,
			    Event *event) {
    Command *c = new CommandWrite(buffer, offset, cb, ptr, event);
    enqueue(c);
  }

  void
  DeviceQueue::enqueueRead(cl_mem buffer,
			   size_t offset,
			   size_t cb,
			   const void *ptr,
			   Event *event) {
    Command *c = new CommandRead(buffer, offset, cb, ptr, event);
    enqueue(c);
  }

  void
  DeviceQueue::enqueueExec(cl_kernel kernel,
			   cl_uint work_dim,
			   const size_t *global_work_offset,
			   const size_t *global_work_size,
			   const size_t *local_work_size,
			   KernelArgs args,
			   Event *event) {
    Command *c = new CommandExec(kernel, work_dim, global_work_offset,
				 global_work_size, local_work_size, args,
				 event);
    enqueue(c);
  }

  void
  DeviceQueue::enqueueFill(cl_mem buffer,
			   const void *pattern,
			   size_t pattern_size,
			   size_t offset,
			   size_t size,
			   Event *event) {
    Command *c = new CommandFill(buffer, pattern, pattern_size, offset, size,
				 event);
    enqueue(c);
  }


  void
  DeviceQueue::bindThread() {

#ifdef USE_HWLOC
    // Bind thread with HWLOC if it is a CUDA or AMD device
    if (isCudaDevice) {
#ifdef CUDA
      hwloc_bitmap_t set;
      int err;

      set = hwloc_bitmap_alloc();
      err = hwloc_cudart_get_device_cpuset(topology, dev_id, set);
      assert(!err);

      if (!hwloc_bitmap_iszero(globalSet)) {
	hwloc_bitmap_andnot(set, set, globalSet);
      }

      hwloc_bitmap_singlify(set);
      hwloc_bitmap_or(globalSet, globalSet, set);

      hwloc_set_cpubind(topology,
			set,
			HWLOC_CPUBIND_THREAD);

      hwloc_bitmap_free(set);
#endif /* CUDA */
    } else if (isAMDDevice) {
      hwloc_bitmap_t set;
      int err;

      set = hwloc_bitmap_alloc();
      err = hwloc_opencl_get_device_cpuset(topology, device, set);
      assert(!err);

      if (!hwloc_bitmap_iszero(globalSet)) {
	hwloc_bitmap_andnot(set, set, globalSet);
      }

      hwloc_bitmap_singlify(set);
      hwloc_bitmap_or(globalSet, globalSet, set);

      hwloc_set_cpubind(topology,
			set,
			HWLOC_CPUBIND_THREAD);

      hwloc_bitmap_free(set);
    } else {
      // hwloc_obj_t core0;
      // core0 = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, 4);
      // hwloc_set_cpubind(topology,
      // 		      core0->cpuset,
      // 		      HWLOC_CPUBIND_THREAD);
    }

#endif /* USE_HWLOC */
  }

  void
  DeviceQueue::finish() {
    if (lastEvent)
      lastEvent->wait();
  }


  void *
  DeviceQueue::threadFunc(void *args) {
    ((DeviceQueue *) args)->run();
    return NULL;
  }

};
