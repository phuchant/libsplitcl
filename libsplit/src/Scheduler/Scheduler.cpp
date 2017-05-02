#include <Handle/KernelHandle.h>
#include <Scheduler/Scheduler.h>
#include <Utils/Debug.h>

#include <cassert>

namespace libsplit {


  Scheduler::Scheduler(BufferManager *buffManager, unsigned nbDevices) :
    buffManager(buffManager), nbDevices(nbDevices), count(0) {}

  Scheduler::~Scheduler() {}


  void
  Scheduler::getPartition(KernelHandle *k, /* IN */
			  size_t work_dim, /* IN */
			  const size_t *global_work_offset, /* IN */
			  const size_t *global_work_size, /* IN */
			  const size_t *local_work_size, /* IN */
			  bool *needOtherExecutionToComplete, /* OUT */
			  std::vector<SubKernelExecInfo *> &subkernels, /* OUT */
			  std::vector<DeviceBufferRegion> &dataRequired, /* OUT */
			  std::vector<DeviceBufferRegion> &dataWritten,
			  unsigned *id) /* OUT */ {
    SubKernelSchedInfo *SI = NULL;
    bool needToInstantiateAnalysis = false;

    // Get kernel id
    *id = getKernelID(k);

    // First execution
    if (kerID2InfoMap.find(*id) == kerID2InfoMap.end()) {
      // Create schedinfo
      SI = new SubKernelSchedInfo(nbDevices);
      SI->last_work_dim = work_dim;
      for (cl_uint i=0; i<work_dim; i++) {
	SI->last_global_work_offset[i] = (global_work_offset ?
					  global_work_offset[i] : 0);
	SI->last_global_work_size[i] = global_work_size[i];
	SI->last_local_work_size[i] = local_work_size[i];
      }
      kerID2InfoMap[*id] = SI;

      // Get first partition
      getInitialPartition(SI, *id, needOtherExecutionToComplete);

      // Analysis needs to be instantiated
      needToInstantiateAnalysis = true;
    } else {
      SI = kerID2InfoMap[*id];

      // Update timers.
      updateTimers(SI);

      // Update perf descriptor.
      updatePerfDescr(SI);

      // Increment iteration counter.
      SI->iterno++;

      // Get next partition.
      getNextPartition(SI, *id, needOtherExecutionToComplete,
		       &needToInstantiateAnalysis);
    }

    // Check if scalar parameters have changed.
    std::vector<int> currentArgsValues = k->getArgsValuesAsInt();
    for (unsigned i=0; i<SI->argsValues.size(); ++i) {
      if (SI->argsValues[i] != currentArgsValues[i]) {
	needToInstantiateAnalysis = true;
	break;
      }
    }
    SI->argsValues = k->getArgsValuesAsInt();

    // Check if original NDRange has changed.
    if (SI->last_work_dim != work_dim) {
      needToInstantiateAnalysis = true;
    } else {
      for (cl_uint i=0; i<work_dim; i++) {
	if (SI->last_global_work_offset[i] !=
	    (global_work_offset ? global_work_offset[i] : 0) ||
	    SI->last_global_work_size[i] != global_work_size[i] ||
	    SI->last_local_work_size[i] != local_work_size[i]) {
	  needToInstantiateAnalysis = true;
	  break;
	}
      }
    }
    SI->last_work_dim = work_dim;
    for (cl_uint i=0; i<work_dim; i++) {
      SI->last_global_work_offset[i] =
	(global_work_offset ? global_work_offset[i] : 0);
      SI->last_global_work_size[i] = global_work_size[i];
      SI->last_local_work_size[i] = local_work_size[i];
    }

    // Instantiate analysis if needed.
    if (needToInstantiateAnalysis) {
      SI->dataRequired.clear();
      SI->dataWritten.clear();
      for (unsigned i=0; i<SI->subkernels.size(); i++) {
	delete SI->subkernels[i];
      }
      SI->subkernels.clear();

      unsigned dimOrder[work_dim];
      getSortedDim(work_dim, global_work_size, local_work_size, dimOrder);
      bool canSplit = false;


      // Copy requested granularity to real granularity.
      std::copy(SI->req_granu_dscr, SI->req_granu_dscr+SI->req_size_gr,
		SI->real_granu_dscr);
      SI->real_size_gr = SI->req_size_gr;

      for (unsigned i=0; i<work_dim; i++) {
	canSplit =
	  instantiateAnalysis(k, work_dim, global_work_offset, global_work_size,
			      local_work_size, dimOrder[i], SI->real_granu_dscr,
			      &SI->real_size_gr, SI->subkernels,
			      SI->dataRequired, SI->dataWritten);
	if (canSplit)
	  break;
      }

      if (!canSplit) {
	std::cerr << "Error: cannot split kernel !\n";
	exit(EXIT_FAILURE);
      }
    }

    DEBUG("granu", printPartition(SI));

    // Increment call count.
    count++;

    // Set output.
    for (unsigned i=0; i<SI->subkernels.size(); i++)
      subkernels.push_back(SI->subkernels[i]);
    dataRequired = SI->dataRequired;
    dataWritten = SI->dataWritten;
  }

  void
  Scheduler::setH2DEvents(unsigned kerId,
			  unsigned devId,
			  std::vector<Event> &events) {
    assert(kerID2InfoMap.find(kerId) != kerID2InfoMap.end());
    SubKernelSchedInfo *SI = kerID2InfoMap[kerId];
    SI->H2DEvents[devId].insert(SI->H2DEvents[devId].end(),
				events.begin(), events.end());
  }

  void
  Scheduler::setD2HEvents(unsigned kerId,
			  unsigned devId,
			  std::vector<Event> &events) {
    assert(kerID2InfoMap.find(kerId) != kerID2InfoMap.end());
    SubKernelSchedInfo *SI = kerID2InfoMap[kerId];
    SI->D2HEvents[devId].insert(SI->D2HEvents[devId].end(),
				events.begin(), events.end());
  }


  void
  Scheduler::printTimers(SubKernelSchedInfo *SI) {
    for (unsigned d=0; d<nbDevices; d++) {
      std::cerr << "dev " << d << " H2D="
		<< SI->H2DTimes[d] << " D2H="
		<< SI->D2HTimes[d] << " kernel="
		<< SI->kernelTimes[d] << "\n";
    }
  }

  void
  Scheduler::printPartition(SubKernelSchedInfo *SI) {
    std::cerr << "<";
    for (int i=0; i<SI->real_size_gr-1; i++)
      std::cerr << SI->real_granu_dscr[i] << ",";
    std::cerr << SI->real_granu_dscr[SI->real_size_gr-1] << ">\n";
  }

  void
  Scheduler::updateTimers(SubKernelSchedInfo *SI) {
    for (unsigned d=0; d<nbDevices; d++) {
      SI->H2DTimes[d] = 0;
      SI->D2HTimes[d] = 0;
      SI->kernelTimes[d] = 0;

      // H2D timers
      for (unsigned i=0; i<SI->H2DEvents[d].size(); ++i) {
	cl_ulong start, end;
	cl_int err;
	err = real_clGetEventProfilingInfo(SI->H2DEvents[d][i]->event,
					   CL_PROFILING_COMMAND_START,
					   sizeof(start), &start, NULL);
	clCheck(err, __FILE__, __LINE__);
	err = real_clGetEventProfilingInfo(SI->H2DEvents[d][i]->event,
					   CL_PROFILING_COMMAND_END,
					   sizeof(end), &end, NULL);
	clCheck(err, __FILE__, __LINE__);
	SI->H2DEvents[d][i]->release();
	SI->H2DTimes[d] += (end - start) * 1e-6;
      }
      SI->H2DEvents[d].clear();

      // D2H timers
      for (unsigned i=0; i<SI->D2HEvents[d].size(); ++i) {
	cl_ulong start, end;
	cl_int err;
	err = real_clGetEventProfilingInfo(SI->D2HEvents[d][i]->event,
					   CL_PROFILING_COMMAND_START,
					   sizeof(start), &start, NULL);
	clCheck(err, __FILE__, __LINE__);
	err = real_clGetEventProfilingInfo(SI->D2HEvents[d][i]->event,
					   CL_PROFILING_COMMAND_END,
					   sizeof(end), &end, NULL);
	clCheck(err, __FILE__, __LINE__);
	SI->D2HEvents[d][i]->release();
	SI->D2HTimes[d] += (end - start) * 1e-6;
      }
      SI->D2HEvents[d].clear();
    }

    // Subkernels timers
    for (unsigned i=0; i<SI->subkernels.size(); i++) {
      cl_ulong start, end;
      cl_int err;
      unsigned dev = SI->subkernels[i]->device;
      err = real_clGetEventProfilingInfo(SI->subkernels[i]->event->event,
					 CL_PROFILING_COMMAND_START,
					 sizeof(start), &start, NULL);
      clCheck(err, __FILE__, __LINE__);
      err = real_clGetEventProfilingInfo(SI->subkernels[i]->event->event,
					 CL_PROFILING_COMMAND_END,
					 sizeof(end), &end, NULL);
      clCheck(err, __FILE__, __LINE__);
      SI->subkernels[i]->event->release();
      SI->kernelTimes[dev] += (end - start) * 1e-6;
    }

    DEBUG("timers", printTimers(SI));
  }

  void
  Scheduler::updatePerfDescr(SubKernelSchedInfo *SI) {
    SI->size_perf_dscr = SI->real_size_gr;
    int nbSplits = SI->real_size_gr / 3;
    for (int i=0; i<nbSplits; i++) {
      unsigned dev = SI->real_granu_dscr[i*3];
      SI->kernel_perf_dscr[i*3] = dev;
      SI->kernel_perf_dscr[i*3+1] = SI->real_granu_dscr[i*3+2];
      SI->kernel_perf_dscr[i*3+2] = SI->kernelTimes[dev];
    }
  }

  // Compute the array of dimension id from the one with
  // the maximum number of splits.
  void
  Scheduler::getSortedDim(cl_uint work_dim,
			  const size_t *global_work_size,
			  const size_t *local_work_size,
			  unsigned *order) {
    unsigned dimSplitMax[work_dim];
    for (unsigned i=0; i<work_dim; i++) {
      dimSplitMax[i] = global_work_size[i] / local_work_size[i];
      order[i] = i;
    }

    bool changed;

    do {
      changed = false;

      for (unsigned i=0; i<work_dim-1; i++) {
	if (dimSplitMax[i+1] > dimSplitMax[i]) {
	  unsigned tmpMax = dimSplitMax[i+1];
	  dimSplitMax[i+1] = dimSplitMax[i];
	  dimSplitMax[i] = tmpMax;

	  unsigned tmpId = order[i+1];
	  order[i+1] = order[i];
	  order[i] = tmpId;

	  changed = true;
	}
      }
    } while (changed);
  }

  bool
  Scheduler::instantiateAnalysis(KernelHandle *k,
				 cl_uint work_dim,
				 const size_t *global_work_offset,
				 const size_t *global_work_size,
				 const size_t *local_work_size,
				 unsigned splitDim,
				 double *granu_dscr,
				 int *size_gr,
				 std::vector<SubKernelExecInfo *> &subkernels,
				 std::vector<DeviceBufferRegion> &dataRequired,
				 std::vector<DeviceBufferRegion> &dataWritten) {
    KernelAnalysis *analysis = k->getAnalysis();
    if (analysis->hasAtomicOrBarrier())
      return false;

    // Check if there if the number of workgroups is sufficient.
    int nbSplits = *size_gr / 3;
    if (global_work_size[splitDim] / local_work_size[splitDim] <
	(size_t) nbSplits)
      return false;

    // Adapat granu descriptor depending on the minimum granularity
    // (global/local).
    adaptGranudscr(granu_dscr, size_gr, global_work_size[splitDim],
		   local_work_size[splitDim]);

    nbSplits = *size_gr / 3;

    // Create original NDRange.
    NDRange origNDRange = NDRange(work_dim, global_work_size,
				  global_work_offset, local_work_size);
    // Create the sub-NDRanges.
    std::vector<NDRange> subNDRanges;
    origNDRange.splitDim(splitDim, *size_gr, granu_dscr, &subNDRanges);

    // for (unsigned i=0; i<subNDRanges.size(); i++)
    //   subNDRanges[i].dump();

    // Instantiate analysis for each global argument.
    bool canSplit = true;
    std::vector<int> argsValues = k->getArgsValuesAsInt();
    unsigned nbGlobals = analysis->getNbGlobalArguments();
    for (unsigned i=0; i<nbGlobals; i++) {
      ArgumentAnalysis *argAnalysis = analysis->getGlobalArgAnalysis(i);
      argAnalysis->performAnalysis(argsValues, origNDRange, subNDRanges);
      // argAnalysis->dump();
      canSplit = canSplit && argAnalysis->canSplit();
    }

    if (!canSplit)
      return false;

    // Fill subkernels vector
    for (int i=0; i<nbSplits; i++) {
      unsigned devId = granu_dscr[i*3];
      SubKernelExecInfo *subkernel = new SubKernelExecInfo();
      subkernel->device = devId;
      subkernel->work_dim = work_dim;
      for (unsigned dim=0; dim < work_dim; dim++) {
	subkernel->global_work_offset[dim] = subNDRanges[i].getOffset(dim);
	subkernel->global_work_size[dim] = subNDRanges[i].get_global_size(dim);
	subkernel->local_work_size[dim] = local_work_size[dim];
      }
      subkernel->numgroups = global_work_size[splitDim] /
	local_work_size[splitDim];
      subkernel->splitdim = splitDim;
      subkernels.push_back(subkernel);
    }

    // CHECK CODE
    unsigned totalGsize = origNDRange.getOffset(splitDim);
    for (unsigned i=0; i<subkernels.size(); i++) {
      assert(subkernels[i]->global_work_offset[splitDim] == totalGsize);
      totalGsize += subkernels[i]->global_work_size[splitDim];
    }
    totalGsize -= origNDRange.getOffset(splitDim);
    assert(totalGsize == global_work_size[splitDim]);
    // END CHECK CODE


    // Fill dataRequired and dataWritten vectors
    for (unsigned a=0; a<nbGlobals; a++) {
      ArgumentAnalysis *argAnalysis = analysis->getGlobalArgAnalysis(a);
      MemoryHandle *m = k->getGlobalArgHandle(a);

      for (int i=0; i<nbSplits; i++) {
	ListInterval readRegion, writtenRegion;
	unsigned devId = granu_dscr[i*3];

	// Compute read region
	if (!argAnalysis->readBoundsComputed()) {
	  readRegion.setUndefined();
	}

	if (argAnalysis->isReadBySplitNo(i)) {
	  readRegion.myUnion(argAnalysis->getReadSubkernelRegion(i));
	}

	// Compute written region
	writtenRegion.myUnion(argAnalysis->getWrittenSubkernelRegion(i));

	// The data written have to be send to the device.
	readRegion.myUnion(argAnalysis->getWrittenSubkernelRegion(i));

	dataRequired.push_back(DeviceBufferRegion(m, devId, readRegion));
	dataWritten.push_back(DeviceBufferRegion(m, devId, writtenRegion));
      }
    }

    return true;
  }

  bool instantiateSingleDeviceAnalysis(KernelHandle *k,
				       unsigned dev,
				       cl_uint work_dim,
				       const size_t *global_work_offset,
				       const size_t *global_work_size,
				       const size_t *local_work_size,
				       std::vector<SubKernelExecInfo *>
				       &subkernels,
				       std::vector<DeviceBufferRegion> &dataRequired,
				       std::vector<DeviceBufferRegion> &dataWritten) {
    (void) k;
    (void) dev;
    (void) work_dim;
    (void) global_work_offset;
    (void) global_work_size;
    (void) local_work_size;
    (void) subkernels;
    (void) dataRequired;
    (void) dataWritten;

    std::cerr << "Error instantiateSingleDeviceAnalysis not implemented yet !"
	      << "\n";
    exit(EXIT_FAILURE);
  }

  void
  Scheduler::adaptGranudscr(double *granu_dscr, int *size_gr,
			    size_t global_work_size, size_t local_work_size) {
    int nbSplits = *size_gr / 3;

    // Compact granus (only one kernel per device)
    for (int i=0; i<nbSplits; i++) {
      granu_dscr[i*3+2] = granu_dscr[i*3+1] * granu_dscr[i*3+2];
      granu_dscr[i*3+1] = 1;
    }

    // Adapt granularity depending on the minimum granularity (global/local).
    for (int i=0; i<nbSplits; i++) {
      granu_dscr[i*3+2] *= global_work_size;
      granu_dscr[i*3+2] /= local_work_size;
      granu_dscr[i*3+2] = ((int) granu_dscr[i*3+2]) * local_work_size;
      granu_dscr[i*3+2] /= global_work_size;
      granu_dscr[i*3+2] = granu_dscr[i*3+2] < 0 ? 0 : granu_dscr[i*3+2];
    }

    // Adapt granularity depending on the number of work groups.
    // The total granularity has to be equal to 1.0 and granu_dscr cannot
    // contain a kernel with a null granularity.
    int idx = 0;
    double totalGranu = 0;
    for (int i=0; i<nbSplits; i++) {
      granu_dscr[idx*3] = granu_dscr[i*3]; // device
      granu_dscr[idx*3+2] = granu_dscr[i*3+2]; // granu

      totalGranu += granu_dscr[i*3+2];

      if (granu_dscr[i*3+2] > 0)
	idx++;

      if (totalGranu >= 1) {
	granu_dscr[i*3+2] = 1 - (totalGranu - granu_dscr[i*3+2]);
	totalGranu = 1;
	break;
      }
    }

    if (totalGranu < 1)
      granu_dscr[(idx-1)*3+2] += 1.0 - totalGranu;

    *size_gr = idx*3;
  }

};
