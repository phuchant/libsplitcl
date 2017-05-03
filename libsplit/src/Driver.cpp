#include <Queue/DeviceQueue.h>
#include <Queue/Event.h>
#include <Scheduler/Scheduler.h>
#include <Scheduler/SchedulerBadBroyden.h>
#include <Scheduler/SchedulerBroyden.h>
#include <Scheduler/SchedulerEnv.h>
#include <Scheduler/SchedulerFixedPoint.h>
#include <Scheduler/SchedulerMKGR.h>
#include <Scheduler/SchedulerSample.h>
#include <Utils/Debug.h>
#include <Utils/Utils.h>
#include <Driver.h>
#include <Options.h>

namespace libsplit {

  static void waitForEvents(cl_uint num_events_in_wait_list,
			    const cl_event *event_wait_list)
  {
    cl_int err;

    if (event_wait_list) {
      err = real_clWaitForEvents(num_events_in_wait_list, event_wait_list);
      clCheck(err, __FILE__, __LINE__);
    }
  }

  static void createFakeEvent(cl_event *event, cl_command_queue queue) {
    if (event) {
      cl_int err;
      cl_context context;

      err = real_clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT,
				       sizeof(context), &context, NULL);
      clCheck(err, __FILE__, __LINE__);

      *event = real_clCreateUserEvent(context, &err);
      clCheck(err, __FILE__, __LINE__);
      err = real_clSetUserEventStatus(*event, CL_COMPLETE);
      clCheck(err, __FILE__, __LINE__);
    }
  }

  static void debugRegions(std::vector<DeviceBufferRegion> &dataRequired,
			   std::vector<DeviceBufferRegion> &dataWritten) {
    for (unsigned i=0; i<dataRequired.size(); i++) {
      std::cerr << "data required on dev " << dataRequired[i].devId << " :";
      dataRequired[i].region.debug();
      std::cerr << "\n";
    }
    for (unsigned i=0; i<dataWritten.size(); i++) {
      std::cerr << "data written on dev " << dataWritten[i].devId << " :";
      dataWritten[i].region.debug();
      std::cerr << "\n";
    }
  }

  Driver::Driver() {
    bufferMgr = new BufferManager();
    unsigned nbDevices = optDeviceSelection.size() / 2;

    switch(optScheduler) {
    case Scheduler::BADBROYDEN:
      scheduler = new SchedulerBadBroyden(bufferMgr, nbDevices);
      break;
    case Scheduler::BROYDEN:
      scheduler = new SchedulerBroyden(bufferMgr, nbDevices);
      break;
    case Scheduler::FIXEDPOINT:
      scheduler = new SchedulerFixedPoint(bufferMgr, nbDevices);
      break;
    case Scheduler::MKGR:
      scheduler = new SchedulerMKGR(bufferMgr, nbDevices);
      break;
    case Scheduler::SAMPLE:
      scheduler = new SchedulerSample(bufferMgr, nbDevices);
      break;
    default:
      scheduler = new SchedulerEnv(bufferMgr, nbDevices);
    };
  }

  Driver::~Driver() {
    delete scheduler;
    delete bufferMgr;
  }

  void
  Driver::enqueueReadBuffer(cl_command_queue queue,
			    MemoryHandle *m,
			    cl_bool blocking,
			    size_t offset,
			    size_t size,
			    void *ptr,
			    cl_uint num_events_in_wait_list,
			    const cl_event *event_wait_list,
			    cl_event *event) {

    waitForEvents(num_events_in_wait_list, event_wait_list);

    bufferMgr->read(m, blocking, offset, size, ptr);

    createFakeEvent(event, queue);
  }

  void
  Driver::enqueueWriteBuffer(cl_command_queue queue,
			     MemoryHandle *m,
			     cl_bool blocking,
			     size_t offset,
			     size_t size,
			     const void *ptr,
			     cl_uint num_events_in_wait_list,
			     const cl_event *event_wait_list,
			     cl_event *event) {
    waitForEvents(num_events_in_wait_list, event_wait_list);

    bufferMgr->write(m, blocking, offset, size, ptr);

    createFakeEvent(event, queue);
  }

  void
  Driver::enqueueCopyBuffer(cl_command_queue queue,
			    MemoryHandle *src,
			    MemoryHandle *dst,
			    size_t src_offset,
			    size_t dst_offset,
			    size_t size,
			    cl_uint num_events_in_wait_list,
			    const cl_event *event_wait_list,
			    cl_event *event) {
    waitForEvents(num_events_in_wait_list, event_wait_list);

    bufferMgr->copy(src, dst, src_offset, dst_offset, size);

    createFakeEvent(event, queue);
  }

  static void printDriverTimers(double t1, double t2, double t3, double t4,
				double t5, double t6)
  {
    std::cerr << "driver getPartition " << (t2-t1)*1.0e3 << "\n";
    std::cerr << "driver compute transfers " << (t3-t2)*1.0e3 << "\n";
    std::cerr << "driver D2H tranfers " << (t4-t3)*1.0e3 << "\n";
    std::cerr << "driver enqueue H2D + subkernels " << (t5-t4)*1.0e3 << "\n";
    std::cerr << "driver finish H2D + subkernels " << (t6-t5)*1.0e3 << "\n";
  }

  void
  Driver::enqueueNDRangeKernel(cl_command_queue queue,
			       KernelHandle *k,
			       cl_uint work_dim,
			       const size_t *global_work_offset,
			       const size_t *global_work_size,
			       const size_t *local_work_size,
			       cl_uint num_events_in_wait_list,
			       const cl_event *event_wait_list,
			       cl_event *event) {
    double t1 = get_time();
    if (local_work_size == NULL) {
      std::cerr << "Error, local_work_size NULL not handled !\n";
      exit(EXIT_FAILURE);
    }

    waitForEvents(num_events_in_wait_list, event_wait_list);

    std::vector<SubKernelExecInfo *> subkernels;
    std::vector<DeviceBufferRegion> dataRequired;
    std::vector<DeviceBufferRegion> dataWritten;
    std::vector<DeviceBufferRegion> D2HTranfers;
    std::vector<DeviceBufferRegion> H2DTranfers;
    bool needOtherExecutionToComplete = false;
    unsigned kerId = 0;

    scheduler->getPartition(k, work_dim, global_work_offset,
			    global_work_size, local_work_size,
			    &needOtherExecutionToComplete,
			    subkernels,
			    dataRequired, dataWritten, &kerId);
    double t2 = get_time();

    DEBUG("regions", debugRegions(dataRequired, dataWritten));

    bufferMgr->computeTransfers(dataRequired, D2HTranfers, H2DTranfers);

    double t3 = get_time();

    startD2HTransfers(kerId, D2HTranfers);

    // Barrier
    ContextHandle *context = k->getContext();
    for (unsigned i=0; i<context->getNbDevices(); i++) {
      context->getQueueNo(i)->finish();
    }

    startH2DTransfers(kerId, H2DTranfers);

    double t4 = get_time();

    // No nead for a barrier given the fact that we use in order queues.
    // Barrier

    enqueueSubKernels(k, subkernels, dataWritten);

    double t5 = get_time();

    // Barrier
    for (unsigned i=0; i<context->getNbDevices(); i++) {
      context->getQueueNo(i)->finish();
    }
    double t6 = get_time();

    // Case where we need another execution to complete the whole original
    // NDRange.
    if (needOtherExecutionToComplete) {
      return enqueueNDRangeKernel(queue, k, work_dim,
				  global_work_offset,
				  global_work_size,
				  local_work_size,
				  0, NULL, event);
    }

    createFakeEvent(event, queue);

    DEBUG("drivertimers", printDriverTimers(t1, t2, t3, t4, t5, t6));
  }

  void
  Driver::startD2HTransfers(unsigned kerId,
			    const std::vector<DeviceBufferRegion>
			    &transferList) {
    DEBUG("transfers", std::cerr << "start D2H\n");

    // For each device
    for (unsigned i=0; i<transferList.size(); ++i) {
      std::vector<Event> events;
      MemoryHandle *m = transferList[i].m;
      unsigned d = transferList[i].devId;
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      // 1) enqueue transfers
      for (unsigned j=0; j<transferList[i].region.mList.size(); j++) {
	size_t offset = transferList[i].region.mList[j].lb;
	size_t cb = transferList[i].region.mList[j].hb -
	  transferList[i].region.mList[j].lb + 1;

	Event event;
	events.push_back(event);

	DEBUG("tranfers",
	      std::cerr << "reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) m->mLocalBuffer + offset,
			   0, NULL,
			   &events[events.size()-1]);
      }

      // 2) update valid data
      m->hostValidData.myUnion(transferList[i].region);

      // 3) pass transfers events to the scheduler
      scheduler->setD2HEvents(kerId, d, events);
    }

    DEBUG("transfers", std::cerr << "end D2H\n");
  }

  void
  Driver::startH2DTransfers(unsigned kerId,
			    const std::vector<DeviceBufferRegion>
			    &transferList) {
    DEBUG("transfers", std::cerr << "start H2D\n");

    // For each device
    for (unsigned i=0; i<transferList.size(); ++i) {
      std::vector<Event> events;
      MemoryHandle *m = transferList[i].m;
      unsigned d = transferList[i].devId;
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      // 1) enqueue transfers
      for (unsigned j=0; j<transferList[i].region.mList.size(); j++) {
	size_t offset = transferList[i].region.mList[j].lb;
	size_t cb = transferList[i].region.mList[j].hb -
	  transferList[i].region.mList[j].lb + 1;

	Event event;
	events.push_back(event);

	DEBUG("transfers",
	      std::cerr << "writing [" << offset << "," << offset+cb-1
	      << "] to dev " << d << "\n");

	queue->enqueueWrite(m->mBuffers[d],
			    CL_FALSE,
			    offset, cb,
			    (char *) m->mLocalBuffer + offset,
			    0, NULL,
			    &events[events.size()-1]);
      }

      // 2) update valid data
      m->devicesValidData[d].myUnion(transferList[i].region);

      // 3) pass transfers events to the scheduler
      scheduler->setH2DEvents(kerId, d, events);
    }

    DEBUG("transfers", std::cerr << "end H2D\n");
  }

  void
  Driver::enqueueSubKernels(KernelHandle *k,
			    std::vector<SubKernelExecInfo *> &subkernels,
			    const std::vector<DeviceBufferRegion> &dataWritten)
  {
    // 1) enqueue subkernels with events
    for (unsigned i=0; i<subkernels.size(); ++i) {
      unsigned d = subkernels[i]->device;
      DeviceQueue *queue = k->getContext()->getQueueNo(d);

      k->setNumgroupsArg(d, subkernels[i]->numgroups);
      k->setSplitdimArg(d, subkernels[i]->splitdim);

      queue->enqueueExec(k->getDeviceKernel(d),
			 subkernels[i]->work_dim,
			 subkernels[i]->global_work_offset,
			 subkernels[i]->global_work_size,
			 subkernels[i]->local_work_size,
			 k->getKernelArgsForDevice(d),
			 0, NULL,
			 &subkernels[i]->event);
    }

    // 2) update valid data
    for (unsigned i=0; i<dataWritten.size(); i++) {
      MemoryHandle *m = dataWritten[i].m;
      unsigned dev = dataWritten[i].devId;
      unsigned nbDevices = m->mNbBuffers;
      m->devicesValidData[dev].myUnion(dataWritten[i].region);
      m->hostValidData.difference(dataWritten[i].region);
      for (unsigned j=0; j<nbDevices; j++) {
	if (j == dev)
	  continue;
	m->devicesValidData[j].difference(dataWritten[i].region);
      }
    }
  }

};
