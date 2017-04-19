#include "Scheduler.h"

#include <ArgumentAnalysis.h>

#include <iostream>
#include <cassert>

static unsigned count = 0;

Scheduler::Scheduler(unsigned nbDevices, DeviceInfo *devInfo,
		     unsigned numArgs,
		     unsigned nbGlobals, KernelAnalysis *analysis)
  : nbDevices(nbDevices), devInfo(devInfo), numArgs(numArgs),
    nbGlobals(nbGlobals), mAnalysis(analysis),
    mNeedToInstantiate(true) {

  argValues = new int[numArgs];
  for (unsigned i=0; i<numArgs; i++)
    argValues[i] = 0;

  size_gr = size_perf_descr = nbDevices * 3;
  granu_dscr = new double[nbDevices * 3];
  kernel_perf_descr = new double[nbDevices * 3];

  id = count++;
  iterno = 0;
}

Scheduler::~Scheduler() {
  delete[] argValues;
  delete[] granu_dscr;
  delete[] kernel_perf_descr;
}

void
Scheduler::storeArgValue(unsigned argIndex, int value) {
  if (value != argValues[argIndex])
    mNeedToInstantiate = true;

  argValues[argIndex] = value;
}

bool
Scheduler::instantiateAnalysis(cl_uint work_dim,
			       const size_t *global_work_offset,
			       const size_t *global_work_size,
			       const size_t *local_work_size,
			       unsigned splitDim) {
  if (mAnalysis->hasAtomicOrBarrier())
    return false;

  int nbSplits = size_gr / 3;

  // Create original NDRange.
  NDRange origNDRange = NDRange(work_dim, global_work_size, global_work_offset,
				local_work_size);

  if (global_work_size[splitDim] / local_work_size[splitDim] <
      (size_t) nbSplits)
    return false;

  // Compact granus (only one kernel per device)
  for (int i=0; i<nbSplits; i++) {
    granu_dscr[i*3+2] = granu_dscr[i*3+1] * granu_dscr[i*3+2];
    granu_dscr[i*3+1] = 1;
  }

  // Adapt granularity depending on the minimum granularity (global/local).
  for (int i=0; i<nbSplits; i++) {
    granu_dscr[i*3+2] *= global_work_size[splitDim];
    granu_dscr[i*3+2] /= local_work_size[splitDim];
    granu_dscr[i*3+2] = ((int) granu_dscr[i*3+2]) * local_work_size[splitDim];
    granu_dscr[i*3+2] /= global_work_size[splitDim];
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

  nbSplits = idx;
  size_gr = idx*3;

  // Set adapted granu in perf_dscr.
  size_perf_descr = size_gr;
  for (int i=0; i<size_perf_descr/3; i++) {
    kernel_perf_descr[i*3] = granu_dscr[i*3];
    kernel_perf_descr[i*3+1] = granu_dscr[i*3+2];
  }


  // Create the sub-NDRanges.
  std::vector<NDRange> subNDRanges;
  origNDRange.splitDim(splitDim, size_gr, granu_dscr, &subNDRanges);

  // Instantiate analysis for each global argument.
  bool canSplit = true;

  std::vector<int> args(argValues, argValues + numArgs);

  for (unsigned i=0; i<nbGlobals; i++) {
    ArgumentAnalysis *argAnalysis = mAnalysis->getArgAnalysis(i);
    argAnalysis->performAnalysis(args, origNDRange, subNDRanges);

    canSplit = canSplit && argAnalysis->canSplit();
  }

  if (!canSplit)
    return false;

  // for (unsigned i=0; i<nbGlobals; i++) {
  //   mAnalysis->getArgAnalysis(i)->dump();
  // }

  // Update DeviceInfo structures
  updateDeviceInfo();

  // Compute global work size and global work offset for each subkernel
  unsigned devKerId[nbDevices];
  for (unsigned i=0; i<nbDevices; i++)
    devKerId[i] = 0;

  for (int i=0; i<nbSplits; i++) {
    unsigned devId = granu_dscr[i*3];
    unsigned kerId = devKerId[devId];

    for (unsigned dim=0; dim<work_dim; dim++) {

      devInfo[devId].mGlobalWorkSize[kerId][dim] =
	subNDRanges[i].get_global_size(dim);
      devInfo[devId].mGlobalWorkOffset[kerId][dim] =
	subNDRanges[i].getOffset(dim);
    }

    devKerId[devId]++;
  }

  // CHECK CODE
  unsigned totalGsize = origNDRange.getOffset(splitDim);
  for (unsigned i=0; i<nbDevices; i++) {
    for (unsigned k=0; k<devInfo[i].nbKernels; k++) {
      assert(devInfo[i].mGlobalWorkOffset[k][splitDim] == totalGsize);
      totalGsize += devInfo[i].mGlobalWorkSize[k][splitDim];
    }
  }
  totalGsize -= origNDRange.getOffset(splitDim);
  assert(totalGsize == global_work_size[splitDim]);

  // END CHECK CODE

  // Compute the data required and the data written for each device and each
  // argument
  for (unsigned d=0; d<nbDevices; d++) {

    for (unsigned a=0; a<nbGlobals; a++) {
      ArgumentAnalysis *argAnalysis = mAnalysis->getArgAnalysis(a);

      // Clear data required and data written for each global argument
      devInfo[d].dataRequired[a].clear();
      devInfo[d].dataWritten[a].clear();

      if (devInfo[d].nbKernels == 0)
	continue;

      // Compute data required
      if (!argAnalysis->readBoundsComputed()) {
	devInfo[d].dataRequired[a].setUndefined();
      } else {
	// Compute for each sub-kernel the data required and written
	for (unsigned k=0; k<devInfo[d].nbKernels; k++) {
	  unsigned kerIndex = devInfo[d].kernelsIndex[k];

	  if (!argAnalysis->isReadBySplitNo(kerIndex))
	    continue;

	  devInfo[d].dataRequired[a].
	    myUnion(argAnalysis->getReadSubkernelRegion(kerIndex));
	}
      }

      // Compute data written
      for (unsigned k=0; k<devInfo[d].nbKernels; k++) {
	unsigned kerIndex = devInfo[d].kernelsIndex[k];

	devInfo[d].dataWritten[a].
	  myUnion(argAnalysis->getWrittenSubkernelRegion(kerIndex));

	// The data written has to be send to the device
	devInfo[d].dataRequired[a].
	  myUnion(argAnalysis->getWrittenSubkernelRegion(kerIndex));
      }
    }
  }

  return true;
}

bool
Scheduler::instantiateMergeAnalysis(cl_uint work_dim,
				    const size_t *global_work_offset,
				    const size_t *global_work_size,
				    const size_t *local_work_size,
				    unsigned splitDim) {
  return false;

  if (mAnalysis->hasAtomicOrBarrier())
    return false;

  int nbSplits = size_gr / 3;

  // Create original NDRange.
  NDRange origNDRange = NDRange(work_dim, global_work_size, global_work_offset,
				local_work_size);

  if (global_work_size[splitDim] / local_work_size[splitDim] <
      (size_t) nbSplits) {
    return false;
  }

  // Compact granus (only one kernel per device)
  for (int i=0; i<nbSplits; i++) {
    granu_dscr[i*3+2] = granu_dscr[i*3+1] * granu_dscr[i*3+2];
    granu_dscr[i*3+1] = 1;
  }

  // Adapt granularity depending on the minimum granularity (global/local).
  for (int i=0; i<nbSplits; i++) {
    granu_dscr[i*3+2] *= global_work_size[splitDim];
    granu_dscr[i*3+2] /= local_work_size[splitDim];
    granu_dscr[i*3+2] = ((int) granu_dscr[i*3+2]) * local_work_size[splitDim];
    granu_dscr[i*3+2] /= global_work_size[splitDim];
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
      break;
    }
  }
  nbSplits = idx;
  size_gr = idx*3;

  // Set adapted granu in perf_dscr.
  size_perf_descr = size_gr;
  for (int i=0; i<size_perf_descr/3; i++) {
    kernel_perf_descr[i*3] = granu_dscr[i*3];
    kernel_perf_descr[i*3+1] = granu_dscr[i*3+2];
  }


  // Create the sub-NDRanges.
  std::vector<NDRange> subNDRanges;
  origNDRange.splitDim(splitDim, size_gr, granu_dscr, &subNDRanges);

  // Instantiate analysis for each global argument.
  std::vector<int> args(argValues, argValues + numArgs);

  for (unsigned i=0; i<nbGlobals; i++) {
    ArgumentAnalysis *argAnalysis = mAnalysis->getArgAnalysis(i);
    argAnalysis->performAnalysis(args, origNDRange, subNDRanges);
  }

  // Update DeviceInfo structures
  updateDeviceInfo();

  // Compute global work size and global work offset for each subkernel
  unsigned devKerId[nbDevices];
  for (unsigned i=0; i<nbDevices; i++)
    devKerId[i] = 0;

  for (int i=0; i<nbSplits; i++) {
    unsigned devId = granu_dscr[i*3];
    unsigned kerId = devKerId[devId];

    for (unsigned dim=0; dim<work_dim; dim++) {

      devInfo[devId].mGlobalWorkSize[kerId][dim] =
	subNDRanges[i].get_global_size(dim);
      devInfo[devId].mGlobalWorkOffset[kerId][dim] =
	subNDRanges[i].getOffset(dim);
    }

    devKerId[devId]++;
  }

  // CHECK CODE
  unsigned totalGsize = 0;
  for (unsigned i=0; i<nbDevices; i++) {
    for (unsigned k=0; k<devInfo[i].nbKernels; k++) {
      assert(devInfo[i].mGlobalWorkOffset[k][splitDim] == totalGsize);
      totalGsize += devInfo[i].mGlobalWorkSize[k][splitDim];
    }
  }
  assert(totalGsize == global_work_size[splitDim]);

  // END CHECK CODE

  // Compute the data required and the data written for each device and each
  // argument
  for (unsigned d=0; d<nbDevices; d++) {

    for (unsigned a=0; a<nbGlobals; a++) {
      ArgumentAnalysis *argAnalysis = mAnalysis->getArgAnalysis(a);

      // Clear data required and data written for each global argument
      devInfo[d].dataRequired[a].clear();
      devInfo[d].dataWritten[a].clear();

      if (devInfo[d].nbKernels == 0)
	continue;

      // Compute data required
      if (!argAnalysis->readBoundsComputed()) {
	devInfo[d].dataRequired[a].setUndefined();
      } else {
	// Compute for each sub-kernel the data required and written
	for (unsigned k=0; k<devInfo[d].nbKernels; k++) {
	  unsigned kerIndex = devInfo[d].kernelsIndex[k];

	  if (!argAnalysis->isReadBySplitNo(kerIndex))
	    continue;

	  devInfo[d].dataRequired[a].
	    myUnion(argAnalysis->getReadSubkernelRegion(kerIndex));
	}
      }

      // Compute data written

      // If write bounds are not computed, send the whole buffer
      if (!argAnalysis->writeBoundsComputed()) {
	devInfo[d].dataRequired[a].setUndefined();
	devInfo[d].dataWritten[a].setUndefined();

	continue;
      }

      // Case where analysis succeed but regions are not disjoint.
      if (!argAnalysis->canSplit()) {
	devInfo[d].dataRequired[a].setUndefined();
	devInfo[d].dataWritten[a].setUndefined();

	continue;
      }

      // Compute for each sub-kernel the data required and written
      for (unsigned k=0; k<devInfo[d].nbKernels; k++) {
	unsigned kerIndex = devInfo[d].kernelsIndex[k];

	if (!argAnalysis->isWrittenBySplitNo(kerIndex))
	  continue;

	devInfo[d].dataWritten[a].
	  myUnion(argAnalysis->getWrittenSubkernelRegion(kerIndex));

	devInfo[d].dataRequired[a].
	  myUnion(argAnalysis->getWrittenSubkernelRegion(kerIndex));
      }
    }
  }

  return true;
}

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

void
Scheduler::updateDeviceInfo() {
  // Delete GlobalWorkSize and GlobalWorkOffset
  for (unsigned d=0; d<nbDevices; d++) {
    for (unsigned k=0; k<devInfo[d].nbKernels; k++) {
      delete[] devInfo[d].mGlobalWorkSize[k];
      delete[] devInfo[d].mGlobalWorkOffset[k];
    }

    delete[] devInfo[d].mGlobalWorkSize;
    delete[] devInfo[d].mGlobalWorkOffset;
  }
  // 1) compute the number of sub-kernel per device.
  unsigned nbPerDevice[nbDevices];
  for (unsigned i=0; i<nbDevices; i++)
    nbPerDevice[i] = 0;

  int nbSplits = size_gr / 3;

  for (int i=0; i<nbSplits; ++i)
    nbPerDevice[(int) granu_dscr[i*3]]++;

  // 2) for each device compute the sub-kernels indexes.
  // (index in ArgumentAnalysis->getKernelLb()).
  unsigned deviceKernelId[nbDevices];
  for (unsigned i=0; i<nbDevices; i++)
    deviceKernelId[i] = 0;

  for (unsigned i=0; i<nbDevices; ++i) {
    devInfo[i].nbKernels = nbPerDevice[i];
    delete[] devInfo[i].kernelsIndex;
    delete[] devInfo[i].kernelsEvent;
    devInfo[i].kernelsIndex = new unsigned[nbPerDevice[i]];
    devInfo[i].kernelsEvent = new cl_event[nbPerDevice[i]];
  }

  for (int i=0; i<nbSplits; ++i) {
    unsigned curDev = granu_dscr[i*3];
    devInfo[curDev].kernelsIndex[deviceKernelId[curDev]++] = i;
  }

  // Update mGlobalWorkSize and mGlobalWorkOffset sizes
  for (unsigned d=0; d<nbDevices; d++) {
    devInfo[d].mGlobalWorkSize = new size_t *[nbSplits];
    devInfo[d].mGlobalWorkOffset = new size_t *[nbSplits];

    for (unsigned k=0; k<devInfo[d].nbKernels; k++) {
      devInfo[d].mGlobalWorkSize[k] = new size_t[3];
      devInfo[d].mGlobalWorkOffset[k] = new size_t[3];
    }
  }
}
