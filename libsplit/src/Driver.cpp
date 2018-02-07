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

  static void debugRegions(KernelHandle *k,
			   std::vector<DeviceBufferRegion> &dataRequired,
			   std::vector<DeviceBufferRegion> &dataWritten,
			   std::vector<DeviceBufferRegion> &dataWrittenAtomicSum) {
    for (unsigned i=0; i<dataRequired.size(); i++) {
      std::cerr << "data required on dev " << dataRequired[i].devId << " :";
      std::cerr << "m=" << dataRequired[i].m << " ";
      std::cerr << "pos= " << k->getArgPosFromBuffer(dataRequired[i].m) << " ";
      if (dataRequired[i].region.isUndefined())
	std::cerr << "(undefined) ";
      dataRequired[i].region.debug();
      std::cerr << "\n";
    }
    for (unsigned i=0; i<dataWritten.size(); i++) {
      std::cerr << "data written on dev " << dataWritten[i].devId << " :";
      std::cerr << "m=" << dataWritten[i].m << " ";
      std::cerr << "pos= " << k->getArgPosFromBuffer(dataWritten[i].m) << " ";
      if (dataWritten[i].region.isUndefined())
	std::cerr << "(undefined) ";
      dataWritten[i].region.debug();
      std::cerr << "\n";
    }
    for (unsigned i=0; i<dataWrittenAtomicSum.size(); i++) {
      std::cerr << "m=" << dataWrittenAtomicSum[i].m << " ";
      std::cerr << "pos= " << k->getArgPosFromBuffer(dataWrittenAtomicSum[i].m) << " ";
      if (dataWrittenAtomicSum[i].region.isUndefined())
	std::cerr << "(undefined) ";

      std::cerr << "data written atomic sum on dev " << dataWrittenAtomicSum[i].devId << " :";
      dataWrittenAtomicSum[i].region.debug();
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

  void
  Driver:: enqueueFillBuffer(cl_command_queue queue,
			     MemoryHandle *m,
			     const void *pattern,
			     size_t pattern_size,
			     size_t offset,
			     size_t size,
			     cl_uint num_events_in_wait_list,
			     const cl_event *event_wait_list,
			     cl_event *event) {
    waitForEvents(num_events_in_wait_list, event_wait_list);

    bufferMgr->fill(m, pattern, pattern_size, offset, size);

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

  static void debugIndirectionRegion(BufferIndirectionRegion &reg) {
    std::cerr << "kerId= " << reg.subkernelId
	      << " lbAddr= " << reg.lb
	      << " hbAddr= " << reg.hb
	      << " indirId=" << reg.indirectionId;

    switch (reg.type) {
    case INT:
      std::cerr << " lb=" << reg.lbValue->getLongValue()
		<< " hb=" << reg.hbValue->getLongValue() << "\n";
      break;
    case FLOAT:
      std::cerr << " lb=" << reg.lbValue->getFloatValue()
		<< " hb=" << reg.hbValue->getFloatValue() << "\n";
      break;
    case DOUBLE:
      std::cerr << " lb=" << reg.lbValue->getDoubleValue()
		<< " hb=" << reg.hbValue->getDoubleValue() << "\n";
      break;
    default:
      std::cerr << "lb=UNKNOWN hb=UNKNOWN\n";
    };
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

    DEBUG("ndrange",
	  for (unsigned i=0; i<work_dim; i++)
	    std::cerr << "#wg" << i << ": " << global_work_size[i] / local_work_size[i] << " ";
	  std::cerr << "\n";);

    waitForEvents(num_events_in_wait_list, event_wait_list);

    std::vector<SubKernelExecInfo *> subkernels;
    std::vector<DeviceBufferRegion> dataRequired;
    std::vector<DeviceBufferRegion> dataWritten;
    std::vector<DeviceBufferRegion> dataWrittenMerge;
    std::vector<DeviceBufferRegion> dataWrittenOr;
    std::vector<DeviceBufferRegion> dataWrittenAtomicSum;
    std::vector<DeviceBufferRegion> dataWrittenAtomicMin;
    std::vector<DeviceBufferRegion> dataWrittenAtomicMax;
    std::vector<DeviceBufferRegion> D2HTransfers;
    std::vector<DeviceBufferRegion> H2DTransfers;
    std::vector<DeviceBufferRegion> OrD2HTransfers;
    std::vector<DeviceBufferRegion> AtomicSumD2HTransfers;
    std::vector<DeviceBufferRegion> AtomicMinD2HTransfers;
    std::vector<DeviceBufferRegion> AtomicMaxD2HTransfers;
    std::vector<DeviceBufferRegion> MergeD2HTransfers;
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

      if (indirectionRegions.size() > 0) {
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
	  void *lbAddress = ((char *)m->mLocalBuffer) + lb;
	  void *hbAddress = ((char *)m->mLocalBuffer) + hb;

	  switch (indirectionRegions[i].type) {
	  case INT:
	  switch(cb) {
	  case 8:
	    indirectionRegions[i].lbValue =
	      IndexExprValue::createLong(*((long *) lbAddress));
	    indirectionRegions[i].hbValue =
	      IndexExprValue::createLong(*((long *) hbAddress));
	    break;
	  case 4:
	    indirectionRegions[i].lbValue =
	      IndexExprValue::createLong((long) *((int *) lbAddress));
	    indirectionRegions[i].hbValue =
	      IndexExprValue::createLong((long) *((int *) hbAddress));
	    break;
	  case 2:
	    indirectionRegions[i].lbValue =
	      IndexExprValue::createLong((long) *((short *) lbAddress));
	    indirectionRegions[i].hbValue =
	      IndexExprValue::createLong((long) *((short *) hbAddress));
	    break;
	  case 1:
	    indirectionRegions[i].lbValue =
	      IndexExprValue::createLong((long) *((char *) lbAddress));
	    indirectionRegions[i].hbValue =
	      IndexExprValue::createLong((long) *((char *) hbAddress));
	    break;
	  default:
	    std::cerr << "Error: Unhandled integer size : " << cb << "\n";
	    exit(EXIT_FAILURE);
	  };
	  break;
	  case FLOAT:
	    assert(cb == 4);
	    indirectionRegions[i].lbValue =
	      IndexExprValue::createFloat(*((float *) lbAddress));
	    indirectionRegions[i].hbValue =
	      IndexExprValue::createFloat(*((float *) hbAddress));
	    break;
	  case DOUBLE:
	    assert(cb == 8);
	    indirectionRegions[i].lbValue =
	      IndexExprValue::createDouble(*((double *) lbAddress));
	    indirectionRegions[i].hbValue =
	      IndexExprValue::createDouble(*((double *) hbAddress));
	    break;
	  case UNDEF:
	    std::cerr << "Error: unknown indirection type.\n";
	    exit(EXIT_FAILURE);
	  };

	  DEBUG("indirection",
		debugIndirectionRegion(indirectionRegions[i]);
		);
	}
      }

      done = scheduler->setIndirectionValues(k, indirectionRegions);

      if (indirectionRegions.size() > 0) {
	for (unsigned i=0; i<indirectionRegions.size(); i++) {
	  delete indirectionRegions[i].lbValue;
	  delete indirectionRegions[i].hbValue;
	}
      }

    } while(!done);

    // Get partition from scheduler along with data required and data written.
    scheduler->getPartition(k,
			    &needOtherExecutionToComplete,
			    subkernels,
			    dataRequired, dataWritten, dataWrittenMerge,
			    dataWrittenOr,
			    dataWrittenAtomicSum, dataWrittenAtomicMin,
			    dataWrittenAtomicMax,
			    &kerId);

    double t2 = get_time();

    DEBUG("regions", debugRegions(k, dataRequired, dataWritten, dataWrittenAtomicSum));

    DEBUG("granu",
    std::cerr << k->getName() << ": ";
	  for (SubKernelExecInfo *ki : subkernels)
	    std::cerr << "<" << ki->device << "," << ki->global_work_size[ki->splitdim] << "> ";
	  std::cerr << "\n";);

    bufferMgr->computeTransfers(dataRequired,
				dataWritten,
				dataWrittenMerge,
				dataWrittenOr,
				dataWrittenAtomicSum, dataWrittenAtomicMin,
				dataWrittenAtomicMax,
				D2HTransfers, H2DTransfers,
				OrD2HTransfers,
				AtomicSumD2HTransfers, AtomicMinD2HTransfers,
				AtomicMaxD2HTransfers, MergeD2HTransfers);

    DEBUG("transfers",
	  std::cerr << "OrD2HTransfers.size()="<< OrD2HTransfers.size() << "\n";
	  std::cerr << "AtomicSumD2HTransfers.size()="<< AtomicSumD2HTransfers.size() << "\n";
	  std::cerr << "AtomicMinD2HTransfers.size()="<< AtomicMinD2HTransfers.size() << "\n";
	  std::cerr << "AtomicMaxD2HTransfers.size()="<< AtomicMaxD2HTransfers.size() << "\n";);

    double t3 = get_time();

    ContextHandle *context = k->getContext();

    if (D2HTransfers.size() > 0) {
      startD2HTransfers(kerId, D2HTransfers);

      // Barrier
      for (unsigned i=0; i<context->getNbDevices(); i++) {
	context->getQueueNo(i)->finish();
      }
    }


    if (H2DTransfers.size() > 0)
      startH2DTransfers(kerId, H2DTransfers);


    double t4 = get_time();

    // No nead for a barrier given the fact that we use in order queues.
    // Barrier

    enqueueSubKernels(k, kerId, subkernels, dataWritten);

    if (OrD2HTransfers.size() > 0)
      startOrD2HTransfers(kerId, OrD2HTransfers);
    if (AtomicSumD2HTransfers.size() > 0)
      startAtomicSumD2HTransfers(kerId, AtomicSumD2HTransfers);
    if (AtomicMinD2HTransfers.size() > 0)
      startAtomicMinD2HTransfers(kerId, AtomicMinD2HTransfers);
    if (AtomicMaxD2HTransfers.size() > 0)
      startAtomicMaxD2HTransfers(kerId, AtomicMaxD2HTransfers);
    if (MergeD2HTransfers.size() > 0)
      startMergeD2HTransfers(kerId, MergeD2HTransfers);


    double t5 = get_time();

    // Barrier
    for (unsigned i=0; i<context->getNbDevices(); i++) {
      context->getQueueNo(i)->finish();
    }

    double t6 = get_time();

    if (OrD2HTransfers.size() > 0)
      performHostOrVariableReduction(OrD2HTransfers);
    if (AtomicSumD2HTransfers.size() > 0)
      performHostAtomicSumReduction(k, AtomicSumD2HTransfers);
    if (AtomicMinD2HTransfers.size() > 0)
      performHostAtomicMinReduction(k, AtomicMinD2HTransfers);
    if (AtomicMaxD2HTransfers.size() > 0)
      performHostAtomicMaxReduction(k, AtomicMaxD2HTransfers);
    if (MergeD2HTransfers.size() > 0)
      performHostMerge(k, MergeD2HTransfers);

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

	DEBUG("transfers",
	      std::cerr << "D2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) m->mLocalBuffer + offset,
			   0, NULL,
			   &event);
	scheduler->setD2HEvent(m->lastWriter, kerId, d, event);
      }

      // 2) update valid data
      m->hostValidData.myUnion(transferList[i].region);
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
      MemoryHandle *m = transferList[i].m;
      unsigned d = transferList[i].devId;
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      // 1) enqueue transfers
      for (unsigned j=0; j<transferList[i].region.mList.size(); j++) {
	size_t offset = transferList[i].region.mList[j].lb;
	size_t cb = transferList[i].region.mList[j].hb -
	  transferList[i].region.mList[j].lb + 1;

	DEBUG("transfers",
	      std::cerr << "D2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) m->mLocalBuffer + offset,
			   0, NULL,
			   NULL /* These tranfers are ignored for the now */);
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
      MemoryHandle *m = transferList[i].m;
      unsigned d = transferList[i].devId;
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      // 1) enqueue transfers
      for (unsigned j=0; j<transferList[i].region.mList.size(); j++) {
	size_t offset = transferList[i].region.mList[j].lb;
	size_t cb = transferList[i].region.mList[j].hb -
	  transferList[i].region.mList[j].lb + 1;

	Event event;

	DEBUG("transfers",
	      std::cerr << "writing [" << offset << "," << offset+cb-1
	      << "] to dev " << d << " on buffer " << m->id << "\n");

	queue->enqueueWrite(m->mBuffers[d],
			    CL_FALSE,
			    offset, cb,
			    (char *) m->mLocalBuffer + offset,
			    0, NULL,
			    &event);
	scheduler->setH2DEvent(m->lastWriter, kerId, d, event);
      }

      // 2) update valid data
      m->devicesValidData[d].myUnion(transferList[i].region);
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

	DEBUG("transfers",
	      std::cerr << "OrD2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) transferList[i].tmp + tmpOffset,
			   0, NULL,
			   NULL /* Or D2H events not used for the moment. */);
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
      MemoryHandle *m = transferList[i].m;
      unsigned d = transferList[i].devId;
      DeviceQueue *queue = m->mContext->getQueueNo(d);

      // 1) enqueue transfers

      size_t tmpOffset = 0;

      for (unsigned j=0; j<transferList[i].region.mList.size(); j++) {
	size_t offset = transferList[i].region.mList[j].lb;
	size_t cb = transferList[i].region.mList[j].hb -
	  transferList[i].region.mList[j].lb + 1;

	DEBUG("transfers",
	      std::cerr << "AtomicSum D2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) transferList[i].tmp + tmpOffset,
			   0, NULL, NULL
			   /* Atomic D2H events not used for the moment. */);
	tmpOffset += cb;
      }
    }

    DEBUG("transfers", std::cerr << "end AtomicSum D2H\n");
  }

  void
  Driver::startAtomicMinD2HTransfers(unsigned kerId,
				     const std::vector<DeviceBufferRegion>
				     &transferList) {
    DEBUG("transfers", std::cerr << "start ATOMIC MIN D2H\n");

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

	DEBUG("transfers",
	      std::cerr << "AtomicMin D2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) transferList[i].tmp + tmpOffset,
			   0, NULL, NULL
			   /* Atomic D2H events not used for the moment. */);
	tmpOffset += cb;
      }
    }

    DEBUG("transfers", std::cerr << "end AtomicMin D2H\n");
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

	DEBUG("transfers",
	      std::cerr << "AtomicMax D2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) transferList[i].tmp + tmpOffset,
			   0, NULL, NULL
			   /* Atomic D2H events not used for the moment. */);
	tmpOffset += cb;
      }
    }

    DEBUG("transfers", std::cerr << "end AtomicMax D2H\n");
  }

  void
  Driver::startMergeD2HTransfers(unsigned kerId,
				 const std::vector<DeviceBufferRegion>
				 &transferList) {

    std::cerr << "Error: merge disabled !\n";
    assert(false);

    DEBUG("transfers", std::cerr << "start MERGE D2H\n");

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

	DEBUG("transfers",
	      std::cerr << "Merge D2H: reading [" << offset << "," << offset+cb-1
	      << "] from dev " << d << "\n");

	queue->enqueueRead(m->mBuffers[d],
			   CL_FALSE,
			   offset, cb,
			   (char *) transferList[i].tmp + tmpOffset,
			   0, NULL, NULL
			   /* Atomic D2H events not used for the moment. */);
	tmpOffset += cb;
      }
    }

    DEBUG("transfers", std::cerr << "end Merge D2H\n");
  }

  void
  Driver::enqueueSubKernels(KernelHandle *k,
			    unsigned kerId,
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
      m->lastWriter = kerId;
      unsigned dev = dataWritten[i].devId;
      unsigned nbDevices = m->mNbBuffers;
      m->hostValidData.difference(dataWritten[i].region);
      for (unsigned j=0; j<nbDevices; j++) {
	if (j == dev)
	  continue;
	m->devicesValidData[j].difference(dataWritten[i].region);
      }
    }
    for (unsigned i=0; i<dataWritten.size(); i++) {
      MemoryHandle *m = dataWritten[i].m;
      unsigned dev = dataWritten[i].devId;
      m->devicesValidData[dev].myUnion(dataWritten[i].region);
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
	for (unsigned i=1; i<regVec.size(); ++i)
	  ((char *) regVec[0].tmp)[o] |= ((char *) regVec[i].tmp)[o];
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
    unsigned nbDevices = regVec.size();
    size_t total_size = regVec[0].region.total();
    size_t elemSize = sizeof(T);
    size_t numElem = total_size / elemSize;
    assert(total_size % elemSize == 0);

    // Sum of elements from all devices in regVec[0]
#pragma omp parallel for
    for (size_t i = 0; i < numElem; ++i) {
      for (unsigned d=1; d<nbDevices; d++) {
	T elem = ((T *) regVec[d].tmp)[i];
	((T *) regVec[0].tmp)[i] += elem;
      }
    }

    // Final res: localBuffer[e] += regVec[e] - nbDevices * localBuffer[e]
    unsigned tmpOffset = 0;
    for (unsigned id=0; id<regVec[0].region.mList.size(); ++id) {
      size_t myoffset = regVec[0].region.mList[id].lb;
      size_t mycb = regVec[0].region.mList[id].hb - myoffset + 1;
      size_t numElem = mycb / elemSize;
      assert(mycb % elemSize == 0);

#pragma omp parallel for
      for (size_t i=0; i < numElem; ++i) {
	T *ptr = &((T *) ((char *) m->mLocalBuffer + myoffset))[i];
	*ptr += ((T *) ((char *) regVec[0].tmp + tmpOffset))[i] -
	  nbDevices * (*ptr);
      }
      tmpOffset += mycb;
    }
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
      ArgumentAnalysis::TYPE type = k->getBufferType(m);

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
	*ptr = ((T *) ((char *) regVec[0].tmp + tmpOffset))[o];
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
      ArgumentAnalysis::TYPE type = k->getBufferType(m);

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

  template<typename T>
  void doMinReduction(MemoryHandle *m, std::vector<DeviceBufferRegion> &regVec) {
    size_t total_size = regVec[0].region.total();
    size_t elemSize = sizeof(T);
    assert(total_size % elemSize == 0);

    for (size_t o = 0; elemSize * o < total_size; o++) {
      for (unsigned i=1; i<regVec.size(); i++) {
	T a = ((T *) regVec[0].tmp)[o];
	T b = ((T *) regVec[i].tmp)[o];
	DEBUG("reduction",
	      std::cerr << "reduce min " << a << "," << b << "\n";);
	((T *) regVec[0].tmp)[o] = b < a ? b : a;
      }
    }

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
  Driver::performHostAtomicMinReduction(KernelHandle *k,
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
      ArgumentAnalysis::TYPE type = k->getBufferType(m);

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
	  doMinReduction<short>(m, regVec);
	  break;
      case ArgumentAnalysis::USHORT:
	  doMinReduction<unsigned short>(m, regVec);
	  break;
      case ArgumentAnalysis::INT:
	  doMinReduction<int>(m, regVec);
	  break;
      case ArgumentAnalysis::UINT:
	  doMinReduction<unsigned int>(m, regVec);
	  break;
      case ArgumentAnalysis::LONG:
	  doMinReduction<long>(m, regVec);
	  break;
      case ArgumentAnalysis::ULONG:
	  doMinReduction<unsigned long>(m, regVec);
	  break;
      case ArgumentAnalysis::FLOAT:
	  doMinReduction<float>(m, regVec);
	  break;
      case ArgumentAnalysis::DOUBLE:
	  doMinReduction<double>(m, regVec);
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
  void doMergeReduction(MemoryHandle *m, std::vector<DeviceBufferRegion> &regVec) {
    std::cerr << "Error: merge disabled !\n";
    assert(false);
    unsigned nbDevices = regVec.size();
    size_t total_size = regVec[0].region.total();
    size_t elemSize = sizeof(T);
    assert(total_size % elemSize == 0);

    {
      unsigned tmpOffset = 0;
      for (unsigned id=0; id<regVec[0].region.mList.size(); ++id) {
    	size_t myoffset = regVec[0].region.mList[id].lb;
    	size_t mycb = regVec[0].region.mList[id].hb - myoffset + 1;
    	assert(mycb % elemSize == 0);
    	for (size_t o=0; elemSize * o < mycb; o++) {
    	  T *ptr = &((T *) ((char *) m->mLocalBuffer + myoffset))[o];
    	  std::cerr << myoffset + o * elemSize << " ";
    	  std::cerr << *ptr << " ";
	  bool found = false;
    	  for (unsigned i=0; i<nbDevices; i++) {
    	    std::cerr << ((T *) ((char *) regVec[i].tmp + tmpOffset))[o] << " ";
	    if (*ptr == ((T *) ((char *) regVec[i].tmp + tmpOffset))[o])
	      found = true;
    	  }

	  assert(found);
    	}
    	tmpOffset += mycb;
      }
    }

    // Sum of elements from all devices in regVec[0]
    for (size_t o = 0; elemSize * o < total_size; o++) {
      for (unsigned i=1; i<nbDevices; i++) {
	T b = ((T *) regVec[i].tmp)[o];
	((T *) regVec[0].tmp)[o] += b;
      }
    }

    // Final res: localBuffer[e] + regVec[e] - nbDevices * localBuffer[e]
    unsigned tmpOffset = 0;
    for (unsigned id=0; id<regVec[0].region.mList.size(); ++id) {
      size_t myoffset = regVec[0].region.mList[id].lb;
      size_t mycb = regVec[0].region.mList[id].hb - myoffset + 1;
      assert(mycb % elemSize == 0);
      for (size_t o=0; elemSize * o < mycb; o++) {
	T *ptr = &((T *) ((char *) m->mLocalBuffer + myoffset))[o];
	*ptr += ((T *) ((char *) regVec[0].tmp + tmpOffset))[o] -
	  nbDevices * (*ptr);
      }
      tmpOffset += mycb;
    }

    return;
  }

  void
  Driver::performHostMerge(KernelHandle *k,
			   const std::vector<DeviceBufferRegion> &
			   transferList) {
    std::cerr << "Error: merge disabled !\n";
    assert(false);
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
      ArgumentAnalysis::TYPE type = k->getBufferType(m);

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
	  doMergeReduction<short>(m, regVec);
	  break;
      case ArgumentAnalysis::USHORT:
	  doMergeReduction<unsigned short>(m, regVec);
	  break;
      case ArgumentAnalysis::INT:
	  doMergeReduction<int>(m, regVec);
	  break;
      case ArgumentAnalysis::UINT:
	  doMergeReduction<unsigned int>(m, regVec);
	  break;
      case ArgumentAnalysis::LONG:
	  doMergeReduction<long>(m, regVec);
	  break;
      case ArgumentAnalysis::ULONG:
	  doMergeReduction<unsigned long>(m, regVec);
	  break;
      case ArgumentAnalysis::FLOAT:
	  doMergeReduction<float>(m, regVec);
	  break;
      case ArgumentAnalysis::DOUBLE:
	  doMergeReduction<double>(m, regVec);
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
