
#include <Handle/KernelHandle.h>
#include <Options.h>
#include <Scheduler/Scheduler.h>
#include <Utils/Debug.h>

#include <cassert>
#include <cstring>

namespace libsplit {


  Scheduler::Scheduler(BufferManager *buffManager, unsigned nbDevices) :
    buffManager(buffManager), nbDevices(nbDevices), count(0) {}

  Scheduler::~Scheduler() {}


  bool
  Scheduler::scalarParamChanged(const SubKernelSchedInfo *SI,
				const KernelHandle *k) {
    for (unsigned i=0; i<k->getArgsValues().size(); i++) {
      if (SI->argsValues[i]) {
	switch(SI->argsValues[i]->type) {
	case IndexExpr::LONG:
	  if (SI->argsValues[i]->getLongValue() !=
	      k->getArgsValues()[i]->getLongValue())
	    return true;
	  break;
	case IndexExpr::FLOAT:
	  if (SI->argsValues[i]->getFloatValue() !=
	      k->getArgsValues()[i]->getFloatValue())
	    return true;
	  break;
	case IndexExpr::DOUBLE:
	  if (SI->argsValues[i]->getDoubleValue() !=
	      k->getArgsValues()[i]->getDoubleValue())
	    return true;
	  break;
	};
      } else {
	if (k->getArgsValues()[i])
	  return true;
      }
    }

    return false;
  }

  void
  Scheduler::updateScalarValues(SubKernelSchedInfo *SI,
				const KernelHandle *k) {
    for (unsigned i=0; i< SI->argsValues.size(); i++)
      delete SI->argsValues[i];
    SI->argsValues.clear();

    for (unsigned i=0; i<k->getArgsValues().size(); i++) {
      if (k->getArgsValues()[i])
	SI->argsValues.
	  push_back(static_cast<IndexExprValue *>
		    (k->getArgsValues()[i]->clone()));
      else
	SI->argsValues.push_back(nullptr);
    }
  }


  void
  Scheduler::getIndirectionRegions(KernelHandle *k,
				   size_t work_dim,
				   const size_t *global_work_offset,
				   const size_t *global_work_size,
				   const size_t *local_work_size,
				   std::vector<BufferIndirectionRegion> &
				   regions) {
    SubKernelSchedInfo *SI = NULL;

    // Get kernel id
    unsigned id = getKernelID(k);

    // Create schedinfo if it doesn't exists yet.
    if (kerID2InfoMap.find(id) == kerID2InfoMap.end()) {
      SI = new SubKernelSchedInfo(nbDevices);
      SI->last_work_dim = work_dim;
      for (cl_uint i=0; i<work_dim; i++) {
	SI->last_global_work_offset[i] = (global_work_offset ?
					  global_work_offset[i] : 0);
	SI->last_global_work_size[i] = global_work_size[i];
	SI->last_local_work_size[i] = local_work_size[i];
      }

      updateScalarValues(SI, k);
      kerID2InfoMap[id] = SI;
    } else {
      SI = kerID2InfoMap[id];
    }

    // Get partition if needed (first call to getIndirectionRegions for the
    // current iteration.
    if (!SI->hasPartition) {
      SI->hasPartition = true;

      if (!SI->hasInitPartition) {
	getInitialPartition(SI, id, &SI->needOtherExecToComplete);
	SI->hasInitPartition = true;
	// Analysis needs to be instantiated
	SI->needToInstantiateAnalysis = true;
      } else {
	// Update timers.
	updateTimers(SI);

	// Update perf descriptor.
	updatePerfDescr(SI);

	// Increment iteration counter.
	SI->iterno++;

	// Get next partition.
	getNextPartition(SI, id, &SI->needOtherExecToComplete,
			 &SI->needToInstantiateAnalysis);
      }

      // Check if scalar parameters have changed.
      if (scalarParamChanged(SI, k)) {
	SI->needToInstantiateAnalysis = true;
	updateScalarValues(SI, k);
      }

      // Check if original NDRange has changed.
      if (SI->last_work_dim != work_dim) {
	SI->needToInstantiateAnalysis = true;
      } else {
	for (cl_uint i=0; i<work_dim; i++) {
	  if (SI->last_global_work_offset[i] !=
	      (global_work_offset ? global_work_offset[i] : 0) ||
	      SI->last_global_work_size[i] != global_work_size[i] ||
	      SI->last_local_work_size[i] != local_work_size[i]) {
	    SI->needToInstantiateAnalysis = true;
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

      // Sort the NDRange dimensions if analysis needs to be instantiated.
      getSortedDim(work_dim, global_work_size, local_work_size,
		   SI->dimOrder);

      // Hack to split the second dimension for clHeat
      if (!strcmp(k->getName(), "getMaxDerivIntel")) {
	SI->dimOrder[0] = 1; SI->dimOrder[1] = 0;
      }
      SI->currentDim = 0;

      // Copy requested granularity to real granularity.
      std::copy(SI->req_granu_dscr, SI->req_granu_dscr+SI->req_size_gr,
		SI->real_granu_dscr);
      SI->real_size_gr = SI->req_size_gr;
    }

    // Analysis needs to be instantiated every time if the kernel has indirections.
    if (k->getAnalysis()->hasIndirection()) {
      SI->needToInstantiateAnalysis = true;
    }

    if (!SI->needToInstantiateAnalysis)
      return;

    if (SI->currentDim >= work_dim) {
      std::cerr << "Cannot split kernel " << k->getName() << "\n";
      SI->real_size_gr = 3;
      SI->real_granu_dscr[0] = 0;
      SI->real_granu_dscr[1] = 1.0;
      SI->real_granu_dscr[2] = 1.0;
      SI->currentDim = 0;
    }

    // Adapt partition.
    unsigned nbSplits = SI->real_size_gr / 3;
    unsigned currentDim = SI->currentDim;
    unsigned nbWgs = global_work_size[SI->dimOrder[currentDim]] /
      local_work_size[SI->dimOrder[currentDim]];
    if (nbWgs < nbSplits) {
      SI->real_size_gr = nbWgs * 3;
      for (unsigned w=0; w<nbWgs; w++)
	SI->real_granu_dscr[w*3+2] = 1.0 / nbWgs;

      DEBUG("granu",
	    std::cerr << "kernel " << k->getName()
	    << ": insufficient number of workgroups !"
	    << " (" << nbWgs << ")\n";
	    std::cerr << "requested partition : ";
	    for (int ii=0; ii<SI->req_size_gr; ii++)
	      std::cerr << SI->req_granu_dscr[ii] << " ";
	    std::cerr << "\n";
	    std::cerr << "adapted partition : ";
	    for (int ii=0; ii<SI->real_size_gr; ii++)
	      std::cerr << SI->real_granu_dscr[ii] << " ";
	    std::cerr << "\n";);
    }
    adaptGranudscr(SI->real_granu_dscr, &SI->real_size_gr,
		   global_work_size[SI->dimOrder[currentDim]],
		   local_work_size[SI->dimOrder[currentDim]]);
    nbSplits = SI->real_size_gr / 3;

    // Create original and sub NDRanges.
    NDRange origNDRange = NDRange(work_dim, global_work_size,
				  global_work_offset, local_work_size);
    std::vector<NDRange> subNDRanges;
    origNDRange.splitDim(SI->dimOrder[currentDim],
			 SI->real_size_gr, SI->real_granu_dscr,
			 &subNDRanges);

    // Set partition to analysis
    k->getAnalysis()->setPartition(origNDRange, subNDRanges,
				   k->getArgsValues());

    // Get indirection regions.
    if (!optEnableIndirections)
      return;
    for (unsigned s=0; s<nbSplits; s++) {
      const std::vector<ArgIndirectionRegion *> argRegions =
    	k->getAnalysis()->getSubkernelIndirectionsRegions(s);
      for (unsigned i=0; i<argRegions.size(); i++) {
    	unsigned argGlobalId =
    	  k->getAnalysis()->getGlobalArgId(argRegions[i]->pos);
    	MemoryHandle *m = k->getGlobalArgHandle(argGlobalId);
    	regions.push_back(BufferIndirectionRegion(s,
    						  argRegions[i]->id,
    						  m,
						  argRegions[i]->ty,
    						  argRegions[i]->cb,
    						  argRegions[i]->lb,
    						  argRegions[i]->hb));
      }
    }
  }

  bool
  Scheduler::setIndirectionValues(KernelHandle *k,
				  const std::vector<BufferIndirectionRegion> &
				  regions) {
    // Get kernel id
    unsigned id = getKernelID(k);

    // Get schedinfo
    SubKernelSchedInfo *SI = kerID2InfoMap[id];

    // Instantiate analysis if needed.
    if (SI->needToInstantiateAnalysis) {

      // Set indirections values to analysis.
      unsigned nbSplit = SI->real_size_gr / 3;
      if (regions.size() > 0) {
	std::vector<IndirectionValue> regionValues[nbSplit];
	for (unsigned i=0; i<regions.size(); i++) {
	  unsigned subkernelId = regions[i].subkernelId;
	  unsigned indirectionId = regions[i].indirectionId;
	  IndexExprValue *lbValue = regions[i].lbValue;
	  IndexExprValue *hbValue = regions[i].hbValue;
	  regionValues[subkernelId].push_back(IndirectionValue(indirectionId,
							       lbValue, hbValue));
	}
	for (unsigned i=0; i<nbSplit; i++) {
	  k->getAnalysis()->setSubkernelIndirectionsValues(i, regionValues[i]);
	}
      }

      if (nbSplit == 1) {
	instantiateSingleDeviceAnalysis(k,
					SI->real_granu_dscr, &SI->real_size_gr,
					SI->dimOrder[SI->currentDim], SI->subkernels,
					SI->dataRequired, SI->dataWritten,
					SI->dataWrittenOr,
					SI->dataWrittenAtomicSum,
					SI->dataWrittenAtomicMin,
					SI->dataWrittenAtomicMax);
	return true;
      }

      if (!instantiateAnalysis(k,
			       SI->real_granu_dscr, &SI->real_size_gr,
			       SI->dimOrder[SI->currentDim], SI->subkernels,
			       SI->dataRequired, SI->dataWritten,
			       SI->dataWrittenOr,
			       SI->dataWrittenAtomicSum,
			       SI->dataWrittenAtomicMin,
			       SI->dataWrittenAtomicMax)) {
	std::cerr << "cannot split dim " << SI->dimOrder[SI->currentDim] << "\n";
	SI->currentDim++;
	SI->needToInstantiateAnalysis = true;
	return false;
      }
    }

    return true;
  }

  void
  Scheduler::getPartition(KernelHandle *k, /* IN */
			  bool *needOtherExecutionToComplete, /* OUT */
			  std::vector<SubKernelExecInfo *> &subkernels, /* OUT */
			  std::vector<DeviceBufferRegion> &dataRequired, /* OUT */
			  std::vector<DeviceBufferRegion> &dataWritten, /* OUT */
			  std::vector<DeviceBufferRegion> &dataWrittenOr, /* OUT */
			  std::vector<DeviceBufferRegion> &dataWrittenAtomicSum, /* OUT */
			  std::vector<DeviceBufferRegion> &dataWrittenAtomicMin, /* OUT */
			  std::vector<DeviceBufferRegion> &dataWrittenAtomicMax, /* OUT */
			  unsigned *id) /* OUT */ {
    // Get kernel id
    *id = getKernelID(k);

    // Get schedinfo
    SubKernelSchedInfo *SI = kerID2InfoMap[*id];
    SI->hasPartition = false;

    DEBUG("granu", printPartition(SI));

    // Set output.
    for (unsigned i=0; i<SI->subkernels.size(); i++)
      subkernels.push_back(SI->subkernels[i]);
    dataRequired = SI->dataRequired;
    dataWritten = SI->dataWritten;
    dataWrittenOr = SI->dataWrittenOr;
    dataWrittenAtomicSum = SI->dataWrittenAtomicSum;
    dataWrittenAtomicMin = SI->dataWrittenAtomicMin;
    dataWrittenAtomicMax = SI->dataWrittenAtomicMax;
    *needOtherExecutionToComplete = SI->needOtherExecToComplete;

    // Increment call count.
    count++;
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
			  unsigned order[3]) {
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
				 double *granu_dscr,
				 int *size_gr,
				 unsigned splitDim,
				 std::vector<SubKernelExecInfo *> &subkernels,
				 std::vector<DeviceBufferRegion> &dataRequired,
				 std::vector<DeviceBufferRegion> &dataWritten,
				 std::vector<DeviceBufferRegion> &dataWrittenOr,
				 std::vector<DeviceBufferRegion> &dataWrittenAtomicSum,
				 std::vector<DeviceBufferRegion> &dataWrittenAtomicMin,
				 std::vector<DeviceBufferRegion> &dataWrittenAtomicMax) {
    dataRequired.clear();
    dataWritten.clear();
    dataWrittenOr.clear();
    dataWrittenAtomicSum.clear();
    dataWrittenAtomicMin.clear();
    dataWrittenAtomicMax.clear();
    for (unsigned i=0; i<subkernels.size(); i++)
      delete subkernels[i];
    subkernels.clear();


    KernelAnalysis *analysis = k->getAnalysis();
    if (!analysis->performAnalysis())
      return false;

    const NDRange &origNDRange = analysis->getKernelNDRange();
    const std::vector<NDRange> &subNDRanges = analysis->getSubNDRanges();
    int nbSplits = *size_gr / 3;
    unsigned work_dim = origNDRange.get_work_dim();
    unsigned num_groups = origNDRange.get_global_size(splitDim) /
      origNDRange.get_local_size(splitDim);

    // Fill subkernels vector
    for (int i=0; i<nbSplits; i++) {
      unsigned devId = granu_dscr[i*3];
      SubKernelExecInfo *subkernel = new SubKernelExecInfo();
      subkernel->device = devId;
      subkernel->work_dim = work_dim;
      for (unsigned dim=0; dim < work_dim; dim++) {
	subkernel->global_work_offset[dim] = subNDRanges[i].getOffset(dim);
	subkernel->global_work_size[dim] = subNDRanges[i].get_global_size(dim);
	subkernel->local_work_size[dim] = subNDRanges[i].get_local_size(dim);
      }
      subkernel->numgroups = num_groups;
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
    // END CHECK CODE


    // Fill dataRequired and dataWritten vectors
    unsigned nbGlobals = analysis->getNbGlobalArguments();
    for (unsigned a=0; a<nbGlobals; a++) {
      MemoryHandle *m = k->getGlobalArgHandle(a);
      assert(m);

      for (int i=0; i<nbSplits; i++) {
	ListInterval readRegion, writtenRegion, writtenOrRegion,
	  writtenAtomicSumRegion, writtenAtomicMinRegion,
	  writtenAtomicMaxRegion;
	unsigned devId = granu_dscr[i*3];

	// Compute read region
	if (!analysis->argReadBoundsComputed(a)) {
	  readRegion.setUndefined();
	}

	if (!analysis->argWrittenAtomicSumBoundsComputed(a)) {
	  readRegion.setUndefined();
	  writtenAtomicSumRegion.setUndefined();
	}

	if (analysis->argIsReadBySubkernel(a, i)) {
	  readRegion.myUnion(analysis->getArgReadSubkernelRegion(a, i));
	}

	// Compute written region
	if (analysis->argIsWrittenBySubkernel(a, i))
	  writtenRegion.myUnion(analysis->getArgWrittenSubkernelRegion(a, i));

	// Compute written_or region
	if (analysis->argIsWrittenOrBySubkernel(a, i))
	  writtenOrRegion.myUnion(analysis->
				  getArgWrittenOrSubkernelRegion(a, i));

	// Compute written_atomic_sum region
	if (analysis->argIsWrittenAtomicSumBySubkernel(a, i))
	  writtenAtomicSumRegion.
	    myUnion(analysis->getArgWrittenAtomicSumSubkernelRegion(a, i));

	// Compute written_atomic_min region
	if (analysis->argIsWrittenAtomicMinBySubkernel(a, i))
	  writtenAtomicMinRegion.
	    myUnion(analysis->getArgWrittenAtomicMinSubkernelRegion(a, i));

	// Compute written_atomic_max region
	if (analysis->argIsWrittenAtomicMaxBySubkernel(a, i))
	  writtenAtomicMaxRegion.
	    myUnion(analysis->getArgWrittenAtomicMaxSubkernelRegion(a, i));

	// The data written have to be send to the device.
	readRegion.myUnion(analysis->getArgWrittenSubkernelRegion(a, i));
	readRegion.myUnion(analysis->getArgWrittenOrSubkernelRegion(a, i));
	readRegion.myUnion(analysis->
			   getArgWrittenAtomicSumSubkernelRegion(a, i));
	readRegion.myUnion(analysis->
			   getArgWrittenAtomicMinSubkernelRegion(a, i));
	readRegion.myUnion(analysis->
			   getArgWrittenAtomicMaxSubkernelRegion(a, i));

	if (readRegion.total() > 0 || readRegion.isUndefined())
	  dataRequired.push_back(DeviceBufferRegion(m, devId, readRegion));
	if (writtenRegion.total() > 0)
	  dataWritten.push_back(DeviceBufferRegion(m, devId, writtenRegion));
	if (writtenOrRegion.total() > 0)
	  dataWrittenOr.
	    push_back(DeviceBufferRegion(m, devId, writtenOrRegion));
	if (writtenAtomicSumRegion.total() > 0 || writtenAtomicSumRegion.isUndefined())
	  dataWrittenAtomicSum.
	    push_back(DeviceBufferRegion(m, devId, writtenAtomicSumRegion));
	if (writtenAtomicMinRegion.total() > 0)
	  dataWrittenAtomicMin.
	    push_back(DeviceBufferRegion(m, devId, writtenAtomicMinRegion));
	if (writtenAtomicMaxRegion.total() > 0)
	  dataWrittenAtomicMax.
	    push_back(DeviceBufferRegion(m, devId, writtenAtomicMaxRegion));
      }
    }

    return true;
  }

  bool
  Scheduler::instantiateSingleDeviceAnalysis(KernelHandle *k,
					     double *granu_dscr,
					     int *size_gr,
					     unsigned splitDim,
					     std::vector<SubKernelExecInfo *> &subkernels,
					     std::vector<DeviceBufferRegion> &dataRequired,
					     std::vector<DeviceBufferRegion> &dataWritten,
					     std::vector<DeviceBufferRegion> &dataWrittenOr,
					     std::vector<DeviceBufferRegion> &dataWrittenAtomicSum,
					     std::vector<DeviceBufferRegion> &dataWrittenAtomicMin,
					     std::vector<DeviceBufferRegion> &dataWrittenAtomicMax) {
    dataRequired.clear();
    dataWritten.clear();
    dataWrittenOr.clear();
    dataWrittenAtomicSum.clear();
    dataWrittenAtomicMin.clear();
    dataWrittenAtomicMax.clear();
    for (unsigned i=0; i<subkernels.size(); i++)
      delete subkernels[i];
    subkernels.clear();


    KernelAnalysis *analysis = k->getAnalysis();
    analysis->performAnalysis();

    const NDRange &origNDRange = analysis->getKernelNDRange();
    const std::vector<NDRange> &subNDRanges = analysis->getSubNDRanges();
    int nbSplits = *size_gr / 3;

    assert(nbSplits == 1);

    unsigned work_dim = origNDRange.get_work_dim();
    unsigned num_groups = origNDRange.get_global_size(splitDim) /
      origNDRange.get_local_size(splitDim);

    // Fill subkernels vector
    for (int i=0; i<nbSplits; i++) {
      unsigned devId = granu_dscr[i*3];
      SubKernelExecInfo *subkernel = new SubKernelExecInfo();
      subkernel->device = devId;
      subkernel->work_dim = work_dim;
      for (unsigned dim=0; dim < work_dim; dim++) {
	subkernel->global_work_offset[dim] = subNDRanges[i].getOffset(dim);
	subkernel->global_work_size[dim] = subNDRanges[i].get_global_size(dim);
	subkernel->local_work_size[dim] = subNDRanges[i].get_local_size(dim);
      }
      subkernel->numgroups = num_groups;
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
    // END CHECK CODE


    // Fill dataRequired and dataWritten vectors
    unsigned nbGlobals = analysis->getNbGlobalArguments();
    for (unsigned a=0; a<nbGlobals; a++) {
      MemoryHandle *m = k->getGlobalArgHandle(a);
      assert(m);

      for (int i=0; i<nbSplits; i++) {
	ListInterval readRegion, writtenRegion;
	unsigned devId = granu_dscr[i*3];

	// Case where written region is unknown
	if (!analysis->argWrittenBoundsComputed(a) ||
	    !analysis->argWrittenOrBoundsComputed(a) ||
	    !analysis->argWrittenAtomicSumBoundsComputed(a) ||
	    !analysis->argWrittenAtomicMinBoundsComputed(a) ||
	    !analysis->argWrittenAtomicMaxBoundsComputed(a)) {
	  writtenRegion.setUndefined();
	  readRegion.setUndefined();
	}

	// Case where writtenRegion is known.
	else {

	  // Read Region
	  if (!analysis->argReadBoundsComputed(a)) {
	    readRegion.setUndefined();
	  } else {
	    readRegion.myUnion(analysis->getArgReadSubkernelRegion(a, i));
	  }

	  // Written Region
	  writtenRegion.myUnion(analysis->getArgWrittenSubkernelRegion(a, i));
	  writtenRegion.myUnion(analysis->getArgWrittenOrSubkernelRegion(a, i));
	  writtenRegion.myUnion(analysis->getArgWrittenAtomicSumSubkernelRegion(a, i));
	  writtenRegion.myUnion(analysis->getArgWrittenAtomicMinSubkernelRegion(a, i));
	  writtenRegion.myUnion(analysis->getArgWrittenAtomicMaxSubkernelRegion(a, i));
	  readRegion.myUnion(writtenRegion);
	}

	if (readRegion.total() > 0 || readRegion.isUndefined())
	  dataRequired.push_back(DeviceBufferRegion(m, devId, readRegion));
	if (writtenRegion.total() > 0 || writtenRegion.isUndefined()) {
	  dataWritten.push_back(DeviceBufferRegion(m, devId, writtenRegion));
	}
      }
    }

    return true;
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
