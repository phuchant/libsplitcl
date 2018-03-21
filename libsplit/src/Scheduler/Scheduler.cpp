#include <Handle/KernelHandle.h>
#include <Options.h>
#include <Scheduler/Scheduler.h>
#include <Utils/Debug.h>

#include <cassert>
#include <cstring>

namespace libsplit {


  void
  Scheduler::SubKernelSchedInfo::updateTimers() {
    for (unsigned d=0; d<nbDevices; d++) {
      H2DTimes[d] = 0;
      D2HTimes[d] = 0;
      kernelTimes[d] = 0;

      // H2D timers
      for (auto IT : src2H2DEvents[d]) {
	for (unsigned i=0; i<IT.second.size(); ++i) {
	  cl_ulong start, end;
	  cl_int err;

	  IT.second[i]->wait();

	  err = real_clGetEventProfilingInfo(IT.second[i]->event,
	  				     CL_PROFILING_COMMAND_START,
	  				     sizeof(start), &start, NULL);
	  clCheck(err, __FILE__, __LINE__);
	  err = real_clGetEventProfilingInfo(IT.second[i]->event,
	  				     CL_PROFILING_COMMAND_END,
	  				     sizeof(end), &end, NULL);
	  clCheck(err, __FILE__, __LINE__);
	  IT.second[i]->release();

	  double t = (end - start) * 1e-6;
	  H2DTimes[d] += t;

	  int src = IT.first;
	  if (src2H2DTimes[d].find(src) == src2H2DTimes[d].end())
	    src2H2DTimes[d][src] = t;
	  else
	    src2H2DTimes[d][src] += t;
	}


      }
      src2H2DEvents[d].clear();

      // D2H timers
      for (auto IT : src2D2HEvents[d]) {
	for (unsigned i=0; i<IT.second.size(); ++i) {
	  cl_ulong start, end;
	  cl_int err;

	  IT.second[i]->wait();

	  err = real_clGetEventProfilingInfo(IT.second[i]->event,
	  				     CL_PROFILING_COMMAND_START,
	  				     sizeof(start), &start, NULL);
	  clCheck(err, __FILE__, __LINE__);
	  err = real_clGetEventProfilingInfo(IT.second[i]->event,
	  				     CL_PROFILING_COMMAND_END,
	  				     sizeof(end), &end, NULL);
	  clCheck(err, __FILE__, __LINE__);
	  IT.second[i]->release();
	  double t = (end - start) * 1e-6;
	  D2HTimes[d] += t;

	  int src = IT.first;
	  if (src2D2HTimes[d].find(src) == src2D2HTimes[d].end())
	    src2D2HTimes[d][src] = t;
	  else
	    src2D2HTimes[d][src] += t;
	}
      }
      src2D2HEvents[d].clear();
    }

    // Subkernels timers
    for (unsigned i=0; i<subkernels.size(); i++) {
      cl_ulong start, end;
      cl_int err;
      unsigned dev = subkernels[i]->device;
      subkernels[i]->event->wait();
      err = real_clGetEventProfilingInfo(subkernels[i]->event->event,
      					 CL_PROFILING_COMMAND_START,
      					 sizeof(start), &start, NULL);
      clCheck(err, __FILE__, __LINE__);
      err = real_clGetEventProfilingInfo(subkernels[i]->event->event,
      					 CL_PROFILING_COMMAND_END,
      					 sizeof(end), &end, NULL);
      clCheck(err, __FILE__, __LINE__);
      subkernels[i]->event->release();
      kernelTimes[dev] += (end - start) * 1e-6;
    }
  }

  void
  Scheduler::SubKernelSchedInfo::clearEvents() {
    for (unsigned d=0; d<nbDevices; d++) {
      // TODO: release events
      src2H2DEvents[d].clear();
      src2D2HEvents[d].clear();
    }
  }

  void
  Scheduler::SubKernelSchedInfo::clearTimers() {
    for (unsigned d=0; d<nbDevices; d++) {
      src2H2DTimes[d].clear();
      src2D2HTimes[d].clear();
    }
  }

  void
  Scheduler::SubKernelSchedInfo::updatePerfDescr() {
    size_perf_dscr = real_size_gr;
    int nbSplits = real_size_gr / 3;
    for (int i=0; i<nbSplits; i++) {
      unsigned dev = real_granu_dscr[i*3];
      kernel_perf_dscr[i*3] = dev;
      kernel_perf_dscr[i*3+1] = real_granu_dscr[i*3+2];
      kernel_perf_dscr[i*3+2] = kernelTimes[dev];
    }
  }

  void
  Scheduler::SubKernelSchedInfo::printTimers() const {
    double totalPerDevice[nbDevices] = {0};

    for (unsigned d=0; d<nbDevices; d++) {
      std::cerr << "dev " << d << " H2D="
		<< H2DTimes[d] << " D2H="
		<< D2HTimes[d] << " kernel="
		<< kernelTimes[d] << "\n";

      for (auto IT : src2H2DTimes[d])
	std::cerr << "H2D" << d << " from k" << IT.first
		  << ": " << IT.second << "\n";
      for (auto IT : src2D2HTimes[d])
	std::cerr << "D" << d << "2H from k" << IT.first
		  << ": " << IT.second << "\n";

      totalPerDevice[d] = H2DTimes[d] + D2HTimes[d] + kernelTimes[d];
      std::cerr << "total on device " << d << " " << totalPerDevice[d] << "\n";
    }

    for (unsigned d=1; d<nbDevices; d++)
      totalPerDevice[0] = totalPerDevice[d] > totalPerDevice[0] ?
	totalPerDevice[d] : totalPerDevice[0];
    std::cerr << "total for kernel: " << totalPerDevice[0] << "\n";
  }

  Scheduler::Scheduler(BufferManager *buffManager, unsigned nbDevices) :
    buffManager(buffManager), nbDevices(nbDevices), count(0) {}

  Scheduler::~Scheduler() {}


  bool
  Scheduler::paramHaveChanged(const SubKernelSchedInfo *SI,
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
  Scheduler::updateParamValues(SubKernelSchedInfo *SI,
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

      updateParamValues(SI, k);
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
	// Increment iteration counter.
	SI->iterno++;

	// Get next partition.
	getNextPartition(SI, id, &SI->needOtherExecToComplete,
			 &SI->needToInstantiateAnalysis);
      }

      // Check if scalar parameters or buffer parameters have changed.
      // For buffers we consider its address as a long value.
      if (paramHaveChanged(SI, k)) {
	SI->needToInstantiateAnalysis = true;
	updateParamValues(SI, k);
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

    // Fail case
    if (SI->currentDim >= work_dim) {
      std::cerr << "Cannot split kernel " << k->getName() << "\n";
      SI->real_size_gr = 3;
      SI->real_granu_dscr[0] = 0;
      SI->real_granu_dscr[1] = 1.0;
      SI->real_granu_dscr[2] = 1.0;
      SI->currentDim = 0;
    }

    // Shifting
    if (!SI->partitionUnchanged && SI->shiftingPartition) {
      SI->partitionUnchanged = true;

      // Shift sub ndranges
      unsigned shiftingDevice = SI->shiftingDevice;
      unsigned shiftingWgs = SI->shiftingWgs;
      NDRange *origNDRange = SI->origNDRange;
      std::vector<NDRange> subNDRanges = SI->requiredSubNDRanges;
      unsigned nbDevices = subNDRanges.size();
      assert(nbDevices >= 2);
      if (shiftingDevice == 0) {
	subNDRanges[shiftingDevice].shiftRight(SI->currentDim, shiftingWgs);
	subNDRanges[shiftingDevice+1].shiftLeft(SI->currentDim, -shiftingWgs);
      } else if (shiftingDevice == nbDevices-1) {
	subNDRanges[shiftingDevice].shiftLeft(SI->currentDim, shiftingWgs);
	subNDRanges[shiftingDevice-1].shiftRight(SI->currentDim, -shiftingWgs);
      } else {
	subNDRanges[shiftingDevice].shiftLeft(SI->currentDim, shiftingWgs);
	subNDRanges[shiftingDevice].shiftRight(SI->currentDim, shiftingWgs);
	subNDRanges[shiftingDevice-1].shiftRight(SI->currentDim, -shiftingWgs);
	subNDRanges[shiftingDevice+1].shiftLeft(SI->currentDim, -shiftingWgs);
      }

      DEBUG("shift",
	    std::cerr << "shifting dev " << shiftingDevice << "\n";
	    for (unsigned i=0; i<nbDevices; i++) {
	      std::cerr << "dev " << i << ":\n";
	      subNDRanges[i].dump();
	    });
      // Save current device shifted NDRange
      SI->shiftedSubNDRanges[shiftingDevice] = subNDRanges[shiftingDevice];

      // Set partition
      k->getAnalysis()->setPartition(*origNDRange, subNDRanges,
				     k->getArgsValues());
    }


    // New scheduler partition
    if (!SI->partitionUnchanged && !SI->shiftingPartition) {
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
      delete SI->origNDRange;
      SI->origNDRange = new NDRange(work_dim, global_work_size,
				    global_work_offset, local_work_size);
      std::vector<NDRange> subNDRanges;
      SI->origNDRange->splitDim(SI->dimOrder[currentDim],
			    SI->real_size_gr, SI->real_granu_dscr,
			    &subNDRanges);
      SI->requiredSubNDRanges = subNDRanges;

      // Set partition to analysis
      k->getAnalysis()->setPartition(*SI->origNDRange, subNDRanges,
				     k->getArgsValues());
    }

    // Get indirection regions.
    if (!optEnableIndirections)
      return;

    k->getAnalysis()->computeIndirections();

    unsigned nbSplits = SI->real_size_gr / 3;
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
	  IndexExprValue *lbValue = (IndexExprValue *) regions[i].lbValue->clone();
	  IndexExprValue *hbValue = (IndexExprValue *) regions[i].hbValue->clone();
	  regionValues[subkernelId].push_back(IndirectionValue(indirectionId,
							       lbValue, hbValue));
	}
	for (unsigned i=0; i<nbSplit; i++) {
	  k->getAnalysis()->setSubkernelIndirectionsValues(i, regionValues[i]);
	}
      }

      // Check if there is missing indirection values.
      if (optEnableIndirections) {
	if (k->getAnalysis()->indirectionMissing()) {
	  SI->partitionUnchanged = true;
	  return false;
	} else {
	  SI->partitionUnchanged = false;
	}
      }

      // Perform analysis with current partition
      ArgumentAnalysis::status st = k->getAnalysis()->performAnalysis();

      DEBUG("dynanalysis", k->getAnalysis()->debug(););

      // Single device, we don't care about the analysis status.
      if (nbSplit == 1) {
	fillSubkernelInfoSingle(k,
				SI->real_granu_dscr, &SI->real_size_gr,
				SI->dimOrder[SI->currentDim], SI->subkernels,
				SI->dataRequired, SI->dataWritten,
				SI->dataWrittenMerge,
				SI->dataWrittenOr,
				SI->dataWrittenAtomicSum,
				SI->dataWrittenAtomicMin,
				SI->dataWrittenAtomicMax);
	return true;
      }

      switch(st) {
      case ArgumentAnalysis::SUCCESS:
	DEBUG("status", std::cerr << k->getName() << " success\n");
	break;
      case ArgumentAnalysis::MERGE:
	DEBUG("status", std::cerr << k->getName() << " merge\n";);
	break;
      case ArgumentAnalysis::FAIL:
	DEBUG("status", std::cerr << k->getName() << " fail\n";);
	break;
      }

      // Multi device Shifting
      if (SI->shiftingPartition) {

	// Save shifted device analyse
	unsigned shiftingDevice = SI->shiftingDevice;
	KernelAnalysis *analysis = k->getAnalysis();
	unsigned nbGlobals = analysis->getNbGlobalArguments();
	for (unsigned a=0; a<nbGlobals; a++) {
	  SI->shiftDataRequired[shiftingDevice][a] =
	    analysis->getArgReadSubkernelRegion(a, shiftingDevice);
	  if (!analysis->argReadBoundsComputed(a))
	    SI->shiftDataRequired[shiftingDevice][a].setUndefined();
	  SI->shiftDataWritten[shiftingDevice][a] =
	    analysis->getArgWrittenSubkernelRegion(a, shiftingDevice);
	  SI->shiftDataWrittenOr[shiftingDevice][a] =
	    analysis->getArgWrittenOrSubkernelRegion(a, shiftingDevice);
	  SI->shiftDataWrittenAtomicSum[shiftingDevice][a] =
	    analysis->getArgWrittenAtomicSumSubkernelRegion(a, shiftingDevice);
	  if (!analysis->argWrittenAtomicSumBoundsComputed(a)) {
	    SI->shiftDataRequired[shiftingDevice][a].setUndefined();
	    SI->shiftDataWrittenAtomicSum[shiftingDevice][a].setUndefined();
	  }
	  SI->shiftDataWrittenAtomicMin[shiftingDevice][a] =
	    analysis->getArgWrittenAtomicMinSubkernelRegion(a, shiftingDevice);
	  SI->shiftDataWrittenAtomicMax[shiftingDevice][a] =
	    analysis->getArgWrittenAtomicMaxSubkernelRegion(a, shiftingDevice);
	}

	unsigned nbDevices = SI->real_size_gr / 3;

	// If the device shifted is the last one, test if the union of the
	// the for each merge argument if the union of the single written
	// region in included in the full written region.
	if (SI->shiftingDevice == nbDevices - 1) {
	  bool shiftDone = true;

	  DEBUG("shifted",
		std::cerr << "shifted workgroups = " << SI->shiftingWgs << "\n";
	  	for (unsigned i=0; i<nbDevices; i++) {
	  	  std::cerr << "shift ndRange dev " << i << ": ";
	  	  SI->shiftedSubNDRanges[i].dump();
	  	});

	  for (unsigned a=0; a<SI->nbMergeArgs; a++) {
	    ListInterval fullSingleWrittenRegion;
	    unsigned globalPos = SI->mergeArg2GlobalPos[a];
	    for (unsigned i=0; i<nbDevices; i++) {
	      fullSingleWrittenRegion.myUnion(SI->shiftDataWritten[i][globalPos]);
	      DEBUG("shift",
		    std::cerr << "single written region global " << globalPos << " dev " << i << ": ";
		    SI->shiftDataWritten[i][globalPos].debug();
		    std::cerr << "\n";);
	    }
	    DEBUG("shift",
		  std::cerr << "full written region global " << globalPos << ": ";
		  SI->fullWrittenRegion[globalPos]->debug();
		  std::cerr << "\n";);

	    ListInterval *difference = ListInterval::difference(*SI->fullWrittenRegion[globalPos],
								fullSingleWrittenRegion);
	    if (difference->total() > 0)
	      shiftDone = false;
	    delete difference;
	  }


	  /* Fix a bug in SOTL when shifting.
	   *
	   * The valid data written cannot overlap between devices.
	   */
	  if (shiftDone && !strcmp(k->getName(), "box_sort")) {
	    for (unsigned a=0; a<SI->nbMergeArgs; a++) {
	      unsigned globalPos = SI->mergeArg2GlobalPos[a];

	      for (unsigned i=0; i<nbDevices; i++) {
		for (unsigned j=i+1; j<nbDevices; j++) {
		  if (i == j)
		    continue;
		  ListInterval *intersection =
		    ListInterval::intersection(SI->shiftDataWritten[i][globalPos],
					       SI->shiftDataWritten[j][globalPos]);
		  if (intersection->total() > 0)
		    SI->shiftDataWritten[i][globalPos].difference(*intersection);
		  delete intersection;
		}
	      }
	    }
	  }

	  if (shiftDone) {
	    SI->partitionUnchanged = false;
	    SI->shiftingPartition = false;
	    fillSubkernelInfoShift(k,
				   SI,
				   *SI->origNDRange,
				   SI->shiftedSubNDRanges,
				   SI->real_granu_dscr,
				   &SI->real_size_gr,
				   SI->dimOrder[SI->currentDim], SI->subkernels,
				   k->getAnalysis()->getNbGlobalArguments(),
				   SI->dataRequired, SI->dataWritten,
				   SI->dataWrittenMerge,
				   SI->dataWrittenOr,
				   SI->dataWrittenAtomicSum,
				   SI->dataWrittenAtomicMin,
				   SI->dataWrittenAtomicMax);

	    DEBUG("shiftmax",
		  static unsigned shiftingMax = 0;
		  shiftingMax = shiftingMax > SI->shiftingWgs ? shiftingMax : SI->shiftingWgs;
		  std::cerr << "shiftingMax = " << shiftingMax << "\n";
		  );
	    DEBUG("currentshift",
		  std::cerr << "shift: " << SI->shiftingWgs << "\n";);

	    return true;
	  } else {
	    SI->shiftingDevice = 0;
	    if (optShiftStep > 0) {
	      SI->shiftingWgs += optShiftStep;
	    } else {
	      SI->shiftingWgs *= 2;
	    }

	    SI->needToInstantiateAnalysis = true;

	    SI->shiftDataRequired.clear(); SI->shiftDataRequired.resize(nbDevices);
	    SI->shiftDataWritten.clear(); SI->shiftDataWritten.resize(nbDevices);
	    SI->shiftDataWrittenOr.clear(); SI->shiftDataWrittenOr.resize(nbDevices);
	    SI->shiftDataWrittenAtomicSum.clear(); SI->shiftDataWrittenAtomicSum.resize(nbDevices);
	    SI->shiftDataWrittenAtomicMin.clear(); SI->shiftDataWrittenAtomicMin.resize(nbDevices);
	    SI->shiftDataWrittenAtomicMax.clear(); SI->shiftDataWrittenAtomicMax.resize(nbDevices);
	    unsigned nbGlobals = analysis->getNbGlobalArguments();
	    for (unsigned i=0; i<nbDevices; i++) {
	      SI->shiftDataRequired[i].resize(nbGlobals);
	      SI->shiftDataWritten[i].resize(nbGlobals);
	      SI->shiftDataWrittenOr[i].resize(nbGlobals);
	      SI->shiftDataWrittenAtomicSum[i].resize(nbGlobals);
	      SI->shiftDataWrittenAtomicMin[i].resize(nbGlobals);
	      SI->shiftDataWrittenAtomicMax[i].resize(nbGlobals);
	    }

	    return false;
	  }
	} else {
	  SI->shiftingDevice++;
	  SI->needToInstantiateAnalysis = true;
	  return false;
	}
      }

      // Multi device
      switch (st) {
      case ArgumentAnalysis::FAIL:
	std::cerr << "cannot split dim " << SI->dimOrder[SI->currentDim] << "\n";
	SI->currentDim++;
	SI->needToInstantiateAnalysis = true;
	return false;

      case ArgumentAnalysis::MERGE:
	{
	  assert(!SI->shiftingPartition);
	  SI->shiftingPartition = true;
	  SI->shiftingDevice = 0;
	  unsigned splitDim = SI->dimOrder[SI->currentDim];
	  if (optShiftInit > 0) {
	    SI->shiftingWgs = optShiftInit;
	  } else {
	    unsigned nbWgs = SI->origNDRange->get_global_size(splitDim) /
	      SI->origNDRange->get_local_size(splitDim);
	    SI->shiftingWgs = nbWgs / 100;
	    SI->shiftingWgs = SI->shiftingWgs > 0 ? SI->shiftingWgs : 1;
	    assert(SI->shiftingWgs > 0);
	  }

	  SI->shiftedSubNDRanges = SI->requiredSubNDRanges;
	  SI->nbMergeArgs = k->getAnalysis()->getNbMergeArguments();
	  SI->mergeArg2GlobalPos.clear();


	  SI->shiftDataRequired.clear(); SI->shiftDataRequired.resize(nbDevices);
	  SI->shiftDataWritten.clear(); SI->shiftDataWritten.resize(nbDevices);
	  SI->shiftDataWrittenOr.clear(); SI->shiftDataWrittenOr.resize(nbDevices);
	  SI->shiftDataWrittenAtomicSum.clear(); SI->shiftDataWrittenAtomicSum.resize(nbDevices);
	  SI->shiftDataWrittenAtomicMin.clear(); SI->shiftDataWrittenAtomicMin.resize(nbDevices);
	  SI->shiftDataWrittenAtomicMax.clear(); SI->shiftDataWrittenAtomicMax.resize(nbDevices);
	  unsigned nbGlobals = k->getAnalysis()->getNbGlobalArguments();
	  for (unsigned i=0; i<nbDevices; i++) {
	    SI->shiftDataRequired[i].resize(nbGlobals);
	    SI->shiftDataWritten[i].resize(nbGlobals);
	    SI->shiftDataWrittenOr[i].resize(nbGlobals);
	    SI->shiftDataWrittenAtomicSum[i].resize(nbGlobals);
	    SI->shiftDataWrittenAtomicMin[i].resize(nbGlobals);
	    SI->shiftDataWrittenAtomicMax[i].resize(nbGlobals);
	  }

	  // Save full written region for merge arguments.
	  for (unsigned a=0; a<SI->nbMergeArgs; a++) {
	    unsigned globalPos = k->getAnalysis()->getMergeArgGlobalPos(a);
	    SI->mergeArg2GlobalPos[a] = globalPos;
	    SI->fullWrittenRegion[globalPos] =
	      k->getAnalysis()->getArgFullWrittenRegion(globalPos);
	  }

	  SI->needToInstantiateAnalysis = true;
	  return false;
	}
      case ArgumentAnalysis::SUCCESS:
	SI->partitionUnchanged = false;
	fillSubkernelInfoMulti(k,
				SI->real_granu_dscr, &SI->real_size_gr,
				SI->dimOrder[SI->currentDim], SI->subkernels,
				SI->dataRequired, SI->dataWritten,
				SI->dataWrittenMerge,
				SI->dataWrittenOr,
				SI->dataWrittenAtomicSum,
				SI->dataWrittenAtomicMin,
				SI->dataWrittenAtomicMax);

	return true;
      };
    }

    SI->partitionUnchanged = false;

    return true;
  }

  void
  Scheduler::getPartition(KernelHandle *k, /* IN */
			  bool *needOtherExecutionToComplete, /* OUT */
			  std::vector<SubKernelExecInfo *> &subkernels, /* OUT */
			  std::vector<DeviceBufferRegion> &dataRequired, /* OUT */
			  std::vector<DeviceBufferRegion> &dataWritten, /* OUT */
			  std::vector<DeviceBufferRegion> &dataWrittenMerge, /* OUT */
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
    dataWrittenMerge = SI->dataWrittenMerge;
    dataWrittenOr = SI->dataWrittenOr;
    dataWrittenAtomicSum = SI->dataWrittenAtomicSum;
    dataWrittenAtomicMin = SI->dataWrittenAtomicMin;
    dataWrittenAtomicMax = SI->dataWrittenAtomicMax;
    *needOtherExecutionToComplete = SI->needOtherExecToComplete;

    // Increment call count.
    count++;
  }

  void
  Scheduler::setH2DEvent(unsigned srcId,
			 unsigned dstId,
			 unsigned devId,
			 Event event) {
    assert(kerID2InfoMap.find(dstId) != kerID2InfoMap.end());
    SubKernelSchedInfo *SI = kerID2InfoMap[dstId];
    SI->src2H2DEvents[devId][srcId].push_back(event);
  }

  void
  Scheduler::setD2HEvent(unsigned srcId,
			 unsigned dstId,
			 unsigned devId,
			 Event event) {
    assert(kerID2InfoMap.find(dstId) != kerID2InfoMap.end());
    SubKernelSchedInfo *SI = kerID2InfoMap[dstId];
    SI->src2D2HEvents[devId][srcId].push_back(event);
  }

  void
  Scheduler::printPartition(SubKernelSchedInfo *SI) {
    std::cerr << "<";
    for (int i=0; i<SI->real_size_gr-1; i++)
      std::cerr << SI->real_granu_dscr[i] << ",";
    std::cerr << SI->real_granu_dscr[SI->real_size_gr-1] << ">\n";
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
  Scheduler::fillSubkernelInfoMulti(KernelHandle *k,
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
				    std::vector<DeviceBufferRegion> &dataWrittenAtomicMax) {
    dataRequired.clear();
    dataWritten.clear();
    dataWrittenMerge.clear();
    dataWrittenOr.clear();
    dataWrittenAtomicSum.clear();
    dataWrittenAtomicMin.clear();
    dataWrittenAtomicMax.clear();
    dataWrittenMerge.clear();
    for (unsigned i=0; i<subkernels.size(); i++)
      delete subkernels[i];
    subkernels.clear();


    KernelAnalysis *analysis = k->getAnalysis();

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

      // Compute written merge region
      ListInterval writtenMergeRegion;
      writtenMergeRegion.myUnion(analysis->getArgWrittenMergeRegion(a));

      for (int i=0; i<nbSplits; i++) {
	ListInterval readRegion, writtenRegion,
	  writtenOrRegion, writtenAtomicSumRegion, writtenAtomicMinRegion,
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
	readRegion.myUnion(writtenMergeRegion);

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
	if (writtenMergeRegion.total() > 0)
	  dataWrittenMerge.
	    push_back(DeviceBufferRegion(m, devId, writtenMergeRegion));
      }
    }

    return true;
  }

  bool
  Scheduler::fillSubkernelInfoShift(KernelHandle *k,
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
				    std::vector<DeviceBufferRegion> &dataWrittenAtomicMax) {
    dataRequired.clear();
    dataWritten.clear();
    dataWrittenMerge.clear();
    dataWrittenOr.clear();
    dataWrittenAtomicSum.clear();
    dataWrittenAtomicMin.clear();
    dataWrittenAtomicMax.clear();
    dataWrittenMerge.clear();
    for (unsigned i=0; i<subkernels.size(); i++)
      delete subkernels[i];
    subkernels.clear();

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

    // Fill dataRequired and dataWritten vectors
    for (unsigned a=0; a<nbGlobals; a++) {
      MemoryHandle *m = k->getGlobalArgHandle(a);
      assert(m);

      for (int i=0; i<nbSplits; i++) {
	unsigned devId = granu_dscr[i*3];

	// The data written have to be send to the device.
	SI->shiftDataRequired[i][a].myUnion(SI->shiftDataWritten[i][a]);
	SI->shiftDataRequired[i][a].myUnion(SI->shiftDataWrittenOr[i][a]);
	SI->shiftDataRequired[i][a].myUnion(SI->shiftDataWrittenAtomicSum[i][a]);
	SI->shiftDataRequired[i][a].myUnion(SI->shiftDataWrittenAtomicMin[i][a]);
	SI->shiftDataRequired[i][a].myUnion(SI->shiftDataWrittenAtomicMax[i][a]);

	if (SI->shiftDataRequired[i][a].total() > 0 ||
	    SI->shiftDataRequired[i][a].isUndefined())
	  dataRequired.push_back(DeviceBufferRegion(m, devId,
						    SI->shiftDataRequired[i][a]));
	if (SI->shiftDataWritten[i][a].total() > 0)
	  dataWritten.push_back(DeviceBufferRegion(m, devId, SI->shiftDataWritten[i][a]));
	if (SI->shiftDataWrittenOr[i][a].total() > 0)
	  dataWrittenOr.
	    push_back(DeviceBufferRegion(m, devId, SI->shiftDataWrittenOr[i][a]));
	if (SI->shiftDataWrittenAtomicSum[i][a].total() > 0 ||
	    SI->shiftDataWrittenAtomicSum[i][a].isUndefined())
	  dataWrittenAtomicSum.
	    push_back(DeviceBufferRegion(m, devId, SI->shiftDataWrittenAtomicSum[i][a]));
	if (SI->shiftDataWrittenAtomicMin[i][a].total() > 0)
	  dataWrittenAtomicMin.
	    push_back(DeviceBufferRegion(m, devId, SI->shiftDataWrittenAtomicMin[i][a]));
	if (SI->shiftDataWrittenAtomicMax[i][a].total() > 0)
	  dataWrittenAtomicMax.
	    push_back(DeviceBufferRegion(m, devId, SI->shiftDataWrittenAtomicMax[i][a]));
      }
    }

    return true;
  }

  bool
  Scheduler::fillSubkernelInfoSingle(KernelHandle *k,
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
				     std::vector<DeviceBufferRegion> &dataWrittenAtomicMax) {
    dataRequired.clear();
    dataWritten.clear();
    dataWrittenMerge.clear();
    dataWrittenOr.clear();
    dataWrittenAtomicSum.clear();
    dataWrittenAtomicMin.clear();
    dataWrittenAtomicMax.clear();
    for (unsigned i=0; i<subkernels.size(); i++)
      delete subkernels[i];
    subkernels.clear();


    KernelAnalysis *analysis = k->getAnalysis();

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
