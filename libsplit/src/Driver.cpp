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

#include <set>

#include <cstring>

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
    bufferMgr = new BufferManager(optDelayedWrite);
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

  void *
  Driver::enqueueMapBuffer(cl_command_queue queue,
			   MemoryHandle *m,
			   cl_bool blocking_map,
			   cl_map_flags map_flags,
			   size_t offset,
			   size_t size,
			   cl_uint num_events_in_wait_list,
			   const cl_event *event_wait_list,
			   cl_event *event) {
    waitForEvents(num_events_in_wait_list, event_wait_list);

    createFakeEvent(event, queue);

    return bufferMgr->map(m, blocking_map, map_flags, offset, size);
  }

  void
  Driver::enqueueUnmapMemObject(cl_command_queue queue,
				MemoryHandle *m,
				void *mapped_ptr,
				cl_uint num_events_in_wait_list,
				const cl_event *event_wait_list,
				cl_event *event) {
    waitForEvents(num_events_in_wait_list, event_wait_list);

    bufferMgr->unmap(m, mapped_ptr);

    createFakeEvent(event, queue);
  }

  static void printDriverTimers(double t1, double t2, double t3, double t4,
				double t5, double t6)
  {
    std::cerr << "driver getPartition " << (t2-t1)*1.0e3 << "\n";
    std::cerr << "driver compute transfers " << (t3-t2)*1.0e3 << "\n";
    std::cerr << "driver D2H transfers " << (t4-t3)*1.0e3 << "\n";
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

    std::cerr << "kernel " << k->getName() << "\n";

    waitForEvents(num_events_in_wait_list, event_wait_list);

    std::vector<SubKernelExecInfo *> subkernels;
    std::vector<DeviceBufferRegion> dataRequired;
    std::vector<DeviceBufferRegion> dataWritten;
    std::vector<DeviceBufferRegion> dataWrittenOr;
    std::vector<DeviceBufferRegion> dataWrittenAtomicSum;
    std::vector<DeviceBufferRegion> dataWrittenAtomicMax;
    std::vector<DeviceBufferRegion> D2HTransfers;
    std::vector<DeviceBufferRegion> H2DTransfers;
    std::vector<DeviceBufferRegion> OrD2HTransfers;
    std::vector<DeviceBufferRegion> AtomicSumD2HTransfers;
    std::vector<DeviceBufferRegion> AtomicMaxD2HTransfers;
    bool needOtherExecutionToComplete = false;
    unsigned kerId = 0;

    // Read required data for indirections while scheduler is not done.
    bool done = false;
    do {
      std::vector<BufferIndirectionRegion> indirectionRegions;
      std::vector<DeviceBufferRegion> D2HTransfers;

      scheduler->getIndirectionRegions(k,
				       work_dim,
				       global_work_offset,
				       global_work_size,
				       local_work_size,
				       indirectionRegions);

      bufferMgr->computeIndirectionTransfers(indirectionRegions, D2HTransfers);
      startD2HTransfers(D2HTransfers);

      // Barrier
      ContextHandle *context = k->getContext();
      for (unsigned i=0; i<context->getNbDevices(); i++) {
	context->getQueueNo(i)->finish();
      }

      // Fill indirection values.
      for (unsigned i=0; i<indirectionRegions.size(); i++) {
	size_t cb = indirectionRegions[i].cb;
	size_t lb = indirectionRegions[i].lb;
	size_t hb = indirectionRegions[i].hb;
	MemoryHandle *m = indirectionRegions[i].m;
	char *lbAddress = ((char *)m->mLocalBuffer) + lb;
	char *hbAddress = ((char *)m->mLocalBuffer) + hb;
	switch(cb) {
	case 8:
	  indirectionRegions[i].lbValue = (int) *((long *) lbAddress);
	  indirectionRegions[i].hbValue = (int) *((long *) hbAddress);
	  break;
	case 4:
	  indirectionRegions[i].lbValue = *((int *) lbAddress);
	  indirectionRegions[i].hbValue = *((int *) hbAddress);
	  break;
	case 2:
	  indirectionRegions[i].lbValue = (int) *((short *) lbAddress);
	  indirectionRegions[i].hbValue = (int) *((short *) hbAddress);
	  break;
	case 1:
	  indirectionRegions[i].lbValue = (int) *((char *) lbAddress);
	  indirectionRegions[i].hbValue = (int) *((char *) hbAddress);
	  break;
	default:
	  std::cerr << "Error: Unhandled integer size : " << cb << "\n";
	  exit(EXIT_FAILURE);
	};

	DEBUG("indirection",
	      std::cerr << "kerId= " << indirectionRegions[i].subkernelId
	      << " lbAddr= " << indirectionRegions[i].lb
	      << " hbAddr= " << indirectionRegions[i].hb
	      << " indirId=" << indirectionRegions[i].indirectionId
	      << " lb=" << indirectionRegions[i].lbValue << " hb=" <<indirectionRegions[i].hbValue << "\n";
	      );
      }

      done = scheduler->setIndirectionValues(k, indirectionRegions);

    } while(!done);

    // Get partition from scheduler along with data required and data written.
    scheduler->getPartition(k,
			    &needOtherExecutionToComplete,
			    subkernels,
			    dataRequired, dataWritten,
			    dataWrittenOr,
			    dataWrittenAtomicSum, dataWrittenAtomicMax,
			    &kerId);

    double t2 = get_time();

    DEBUG("regions", debugRegions(dataRequired, dataWritten));

    DEBUG("granu",
    std::cerr << k->getName() << ": ";
	  for (SubKernelExecInfo *ki : subkernels)
	    std::cerr << "<" << ki->device << "," << ki->global_work_size[ki->splitdim] << "> ";
	  std::cerr << "\n";);

    bufferMgr->computeTransfers(dataRequired,
				dataWritten,
				dataWrittenOr,
				dataWrittenAtomicSum, dataWrittenAtomicMax,
				D2HTransfers, H2DTransfers,
				OrD2HTransfers,
				AtomicSumD2HTransfers, AtomicMaxD2HTransfers);

    DEBUG("transfers",
	  std::cerr << "OrD2HTransfers.size()="<< OrD2HTransfers.size() << "\n";
	  std::cerr << "AtomicSumD2HTransfers.size()="<< AtomicSumD2HTransfers.size() << "\n";
	  std::cerr << "AtomicMaxD2HTransfers.size()="<< AtomicMaxD2HTransfers.size() << "\n";);

    double t3 = get_time();

    startD2HTransfers(kerId, D2HTransfers);

    // Barrier
    ContextHandle *context = k->getContext();
    for (unsigned i=0; i<context->getNbDevices(); i++) {
      context->getQueueNo(i)->finish();
    }

    startH2DTransfers(kerId, H2DTransfers);

    double t4 = get_time();

    // No nead for a barrier given the fact that we use in order queues.
    // Barrier

    enqueueSubKernels(k, subkernels, dataWritten);

    startOrD2HTransfers(kerId, OrD2HTransfers);
    startAtomicSumD2HTransfers(kerId, AtomicSumD2HTransfers);
    startAtomicMaxD2HTransfers(kerId, AtomicMaxD2HTransfers);

    double t5 = get_time();

    // Barrier
    for (unsigned i=0; i<context->getNbDevices(); i++) {
      context->getQueueNo(i)->finish();
    }

    performHostOrVariableReduction(OrD2HTransfers);
    performHostAtomicSumReduction(k, AtomicSumD2HTransfers);
    performHostAtomicMaxReduction(k, AtomicMaxD2HTransfers);

    double t6 = get_time();

    // Case where we need another execution to complete the whole original
    // NDRange.
    if (needOtherExecutionToComplete) {
      assert(false);
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
    DEBUG("transfers",
	  std::cerr << "start D2H\n");

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
	      std::cerr << "D2H: reading [" << offset << "," << offset+cb-1
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
  Driver::startD2HTransfers(const std::vector<DeviceBufferRegion>
			    &transferList) {
    DEBUG("transfers",
	  std::cerr << "start D2H\n");

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
	      std::cerr << "D2H: reading [" << offset << "," << offset+cb-1
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
	      << "] to dev " << d << " on buffer " << m->id << "\n");

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
  Driver::startOrD2HTransfers(unsigned kerId,
			      const std::vector<DeviceBufferRegion>
			      &transferList) {
    DEBUG("transfers", std::cerr << "start OR D2H\n");

    // For each device
    for (unsigned i=0; i<transferList.size(); ++i) {
      std::vector<Event> events;
      MemoryHandle *m = transferList[i].m;
      unsigned d = transferList[i].devId;
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      // 1) enqueue transfers

      size_t tmpOffset = 0;

      for (unsigned j=0; j<transferList[i].region.mList.size(); j++) {
	size_t offset = transferList[i].region.mList[j].lb;
	size_t cb = transferList[i].region.mList[j].hb -
	  transferList[i].region.mList[j].lb + 1;

	Event event;
	events.push_back(event);

	DEBUG("transfers",
	      std::cerr << "OrD2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) transferList[i].tmp + tmpOffset,
			    0, NULL,
			    &events[events.size()-1]);
	tmpOffset += cb;
      }
    }

    DEBUG("transfers", std::cerr << "end OR D2H\n");
  }

  void
  Driver::startAtomicSumD2HTransfers(unsigned kerId,
				     const std::vector<DeviceBufferRegion>
				     &transferList) {
    DEBUG("transfers", std::cerr << "start ATOMIC SUM D2H\n");

    // For each device
    for (unsigned i=0; i<transferList.size(); ++i) {
      std::vector<Event> events;
      MemoryHandle *m = transferList[i].m;
      unsigned d = transferList[i].devId;
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      // 1) enqueue transfers

      size_t tmpOffset = 0;

      for (unsigned j=0; j<transferList[i].region.mList.size(); j++) {
	size_t offset = transferList[i].region.mList[j].lb;
	size_t cb = transferList[i].region.mList[j].hb -
	  transferList[i].region.mList[j].lb + 1;

	Event event;
	events.push_back(event);

	DEBUG("transfers",
	      std::cerr << "AtomicSum D2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) transferList[i].tmp + tmpOffset,
			    0, NULL,
			    &events[events.size()-1]);
	tmpOffset += cb;
      }
    }

    DEBUG("transfers", std::cerr << "end AtomicSum D2H\n");
  }

  void
  Driver::startAtomicMaxD2HTransfers(unsigned kerId,
				     const std::vector<DeviceBufferRegion>
				     &transferList) {
    DEBUG("transfers", std::cerr << "start ATOMIC MAX D2H\n");

    // For each device
    for (unsigned i=0; i<transferList.size(); ++i) {
      std::vector<Event> events;
      MemoryHandle *m = transferList[i].m;
      unsigned d = transferList[i].devId;
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      // 1) enqueue transfers

      size_t tmpOffset = 0;

      for (unsigned j=0; j<transferList[i].region.mList.size(); j++) {
	size_t offset = transferList[i].region.mList[j].lb;
	size_t cb = transferList[i].region.mList[j].hb -
	  transferList[i].region.mList[j].lb + 1;

	Event event;
	events.push_back(event);

	DEBUG("transfers",
	      std::cerr << "AtomicMax D2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) transferList[i].tmp + tmpOffset,
			    0, NULL,
			    &events[events.size()-1]);
	tmpOffset += cb;
      }
    }

    DEBUG("transfers", std::cerr << "end AtomicMax D2H\n");
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

  void
  Driver::performHostOrVariableReduction(const std::vector<DeviceBufferRegion> &
					 transferList) {
    if (transferList.empty())
      return;

    std::set<MemoryHandle *> memHandles;
    std::map<MemoryHandle *, std::vector<DeviceBufferRegion> > mem2RegMap;

    for (unsigned i=0; i<transferList.size(); ++i) {
      memHandles.insert(transferList[i].m);
      mem2RegMap[transferList[i].m].push_back(transferList[i]);
    }

    for (MemoryHandle *m : memHandles) {
      std::vector<DeviceBufferRegion> regVec = mem2RegMap[m];
      assert(regVec.size() > 0);

      // Perform OR reduction
      for (unsigned o=0; o<regVec[0].region.total(); o++) {

	// // DEBUG
	// for (unsigned i=0; i<regVec.size(); ++i)
	//   std::cerr << "avant or offset " << o << " = " << (int) ((char *) regVec[i].tmp)[o] << "\n";

	for (unsigned i=1; i<regVec.size(); ++i)
	  ((char *) regVec[0].tmp)[o] |= ((char *) regVec[i].tmp)[o];

	// // DEBUG
	// std::cerr << "apres or offset " << o << " = " << (int) ((char *) regVec[0].tmp)[o] << "\n";
      }

      // Update value in local buffer
      unsigned tmpOffset = 0;
      for (unsigned id=0; id<regVec[0].region.mList.size(); ++id) {
	size_t myoffset = regVec[0].region.mList[id].lb;
	size_t mycb = regVec[0].region.mList[id].hb - myoffset + 1;
	memcpy((char *) m->mLocalBuffer + myoffset,
	       (char *) regVec[0].tmp + tmpOffset,
	       mycb);
	tmpOffset += mycb;
      }

      // Update valid data
      for (unsigned d=0; d<m->mNbBuffers; d++)
	m->devicesValidData[d].difference(regVec[0].region);
      m->hostValidData.myUnion(regVec[0].region);

      // Free tmp buffers
      for (unsigned i=0; i<regVec.size(); ++i)
	free(regVec[i].tmp);
    }
  }

  template<typename T>
  void doReduction(MemoryHandle *m, std::vector<DeviceBufferRegion> &regVec) {
    size_t total_size = regVec[0].region.total();
    size_t elemSize = sizeof(T);
    assert(total_size % elemSize == 0);

    // For each elem
    for (size_t o = 0; elemSize * o < total_size; o++) {
      // For each device
      for (unsigned i=1; i<regVec.size(); i++) {
	DEBUG("reduction",
	      std::cerr << "reduce sum [" << o << "] " << ((T *) regVec[0].tmp)[o] << " += "
	      << ((T *) regVec[i].tmp)[o] << "\n";);
	((T *) regVec[0].tmp)[o] += ((T *) regVec[i].tmp)[o];
      }
    }

    DEBUG("reduction", std::cerr << "final: ";);
    unsigned tmpOffset = 0;
    for (unsigned id=0; id<regVec[0].region.mList.size(); ++id) {
      size_t myoffset = regVec[0].region.mList[id].lb;
      size_t mycb = regVec[0].region.mList[id].hb - myoffset + 1;
      assert(mycb % elemSize == 0);
      for (size_t o=0; elemSize * o < mycb; o++) {
	T *ptr = &((T *) ((char *) m->mLocalBuffer + myoffset))[o];
	*ptr = ((T *) ((char *) regVec[0].tmp + tmpOffset))[o];
      }
      tmpOffset += mycb;
    }

    return;
  }

  void
  Driver::performHostAtomicSumReduction(KernelHandle *k,
					const std::vector<DeviceBufferRegion> &
					transferList) {
    if (transferList.empty())
      return;

    std::set<MemoryHandle *> memHandles;
    std::map<MemoryHandle *, std::vector<DeviceBufferRegion> > mem2RegMap;

    for (unsigned i=0; i<transferList.size(); ++i) {
      memHandles.insert(transferList[i].m);
      mem2RegMap[transferList[i].m].push_back(transferList[i]);
    }

    for (MemoryHandle *m : memHandles) {
      std::vector<DeviceBufferRegion> regVec = mem2RegMap[m];
      assert(regVec.size() > 0);

      // Get argument type
      ArgumentAnalysis::TYPE type = k->getArgType(m);

      // Perform atomic sum reduction
      switch(type) {
      case ArgumentAnalysis::BOOL:
      case ArgumentAnalysis::UNKNOWN:
	assert(false);
      case ArgumentAnalysis::CHAR:
	break;
      case ArgumentAnalysis::UCHAR:
	break;
      case ArgumentAnalysis::SHORT:
	  doReduction<short>(m, regVec);
	  break;
      case ArgumentAnalysis::USHORT:
	  doReduction<unsigned short>(m, regVec);
	  break;
      case ArgumentAnalysis::INT:
	  doReduction<int>(m, regVec);
	  break;
      case ArgumentAnalysis::UINT:
	  doReduction<unsigned int>(m, regVec);
	  break;
      case ArgumentAnalysis::LONG:
	  doReduction<long>(m, regVec);
	  break;
      case ArgumentAnalysis::ULONG:
	  doReduction<unsigned long>(m, regVec);
	  break;
      case ArgumentAnalysis::FLOAT:
	  doReduction<float>(m, regVec);
	  break;
      case ArgumentAnalysis::DOUBLE:
	  doReduction<double>(m, regVec);
	  break;
      };

      // Update valid data
      for (unsigned d=0; d<m->mNbBuffers; d++)
	m->devicesValidData[d].difference(regVec[0].region);
      m->hostValidData.myUnion(regVec[0].region);

      // Free tmp buffers
      for (unsigned i=0; i<regVec.size(); ++i)
	free(regVec[i].tmp);
    }
  }

  template<typename T>
  void doMaxReduction(MemoryHandle *m, std::vector<DeviceBufferRegion> &regVec) {
    size_t total_size = regVec[0].region.total();
    size_t elemSize = sizeof(T);
    assert(total_size % elemSize == 0);

    for (size_t o = 0; elemSize * o < total_size; o++) {
      for (unsigned i=1; i<regVec.size(); i++) {
	T a = ((T *) regVec[0].tmp)[o];
	T b = ((T *) regVec[i].tmp)[o];
	DEBUG("reduction",
	      std::cerr << "reduce max " << a << "," << b << "\n";);
	((T *) regVec[0].tmp)[o] = b > a ? b : a;
      }
    }

    unsigned tmpOffset = 0;
    for (unsigned id=0; id<regVec[0].region.mList.size(); ++id) {
      size_t myoffset = regVec[0].region.mList[id].lb;
      size_t mycb = regVec[0].region.mList[id].hb - myoffset + 1;
      assert(mycb % elemSize == 0);
      for (size_t o=0; elemSize * o < mycb; o++) {
	T *ptr = &((T *) ((char *) m->mLocalBuffer + myoffset))[o];
	*ptr = ((T *) ((char *) regVec[0].tmp + tmpOffset))[o] -
	  *ptr * regVec.size();
	DEBUG("reduction", std::cerr << *ptr << " ";);
      }
      tmpOffset += mycb;
    }

    return;
  }

  void
  Driver::performHostAtomicMaxReduction(KernelHandle *k,
					const std::vector<DeviceBufferRegion> &
					transferList) {
    if (transferList.empty())
      return;

    std::set<MemoryHandle *> memHandles;
    std::map<MemoryHandle *, std::vector<DeviceBufferRegion> > mem2RegMap;

    for (unsigned i=0; i<transferList.size(); ++i) {
      memHandles.insert(transferList[i].m);
      mem2RegMap[transferList[i].m].push_back(transferList[i]);
    }

    for (MemoryHandle *m : memHandles) {
      std::vector<DeviceBufferRegion> regVec = mem2RegMap[m];
      assert(regVec.size() > 0);

      // Get argument type
      ArgumentAnalysis::TYPE type = k->getArgType(m);

      // Perform atomic sum reduction
      switch(type) {
      case ArgumentAnalysis::BOOL:
      case ArgumentAnalysis::UNKNOWN:
	assert(false);
      case ArgumentAnalysis::CHAR:
	break;
      case ArgumentAnalysis::UCHAR:
	break;
      case ArgumentAnalysis::SHORT:
	  doMaxReduction<short>(m, regVec);
	  break;
      case ArgumentAnalysis::USHORT:
	  doMaxReduction<unsigned short>(m, regVec);
	  break;
      case ArgumentAnalysis::INT:
	  doMaxReduction<int>(m, regVec);
	  break;
      case ArgumentAnalysis::UINT:
	  doMaxReduction<unsigned int>(m, regVec);
	  break;
      case ArgumentAnalysis::LONG:
	  doMaxReduction<long>(m, regVec);
	  break;
      case ArgumentAnalysis::ULONG:
	  doMaxReduction<unsigned long>(m, regVec);
	  break;
      case ArgumentAnalysis::FLOAT:
	  doMaxReduction<float>(m, regVec);
	  break;
      case ArgumentAnalysis::DOUBLE:
	  doMaxReduction<double>(m, regVec);
	  break;
      };

      // Update valid data
      for (unsigned d=0; d<m->mNbBuffers; d++)
	m->devicesValidData[d].difference(regVec[0].region);
      m->hostValidData.myUnion(regVec[0].region);

      // Free tmp buffers
      for (unsigned i=0; i<regVec.size(); ++i)
	free(regVec[i].tmp);
    }
  }
};
