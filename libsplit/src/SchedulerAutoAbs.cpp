#include <Debug.h>
#include <Options.h>
#include "SchedulerAutoAbs.h"
#include "Utils.h"

#include <iostream>
#include <limits>

void SchedulerAutoAbs::updatePerfDescrWithComm()
{
  double longest = 0;
  int nbSplits = size_gr / 3;

  for (int i=0; i<nbSplits; i++) {
    double time = kernel_time[(int) granu_dscr[i*3]] +
      comm_time[(int) granu_dscr[i*3]];
    kernel_perf_descr[i * 3 + 2] = time;
    longest = time > longest ? time : longest;
  }

  if (longest < bestTime) {
    bestTime = longest;
    best_size_gr = size_gr;
    std::copy(granu_dscr, granu_dscr + size_gr, best_granu_dscr);
  } else {
    bestReached = true;

    if (stopBest) {
      for (int i=0; i<size_gr; i++) {
	std::cerr << granu_dscr[i] << " ";
      }
      std::cerr << "\n";
      exit(0);
    }
  }
}

void SchedulerAutoAbs::updatePerfDescrWithoutComm()
{
  double longest = 0;
  int nbSplits = size_gr / 3;

  for (int i=0; i<nbSplits; i++) {
    double time = kernel_time[(int) granu_dscr[i*3]];
    kernel_perf_descr[i * 3 + 2] = time;
    longest = time > longest ? time : longest;
  }

  if (longest < bestTime) {
    bestTime = longest;
    best_size_gr = size_gr;
    std::copy(granu_dscr, granu_dscr + size_gr, best_granu_dscr);
  } else {
    bestReached = true;
  }
}

SchedulerAutoAbs::SchedulerAutoAbs(unsigned nbDevices, DeviceInfo *devInfo,
				   unsigned numArgs,
				   unsigned nbGlobals,
				   KernelAnalysis *analysis)
  : Scheduler(nbDevices, devInfo, numArgs, nbGlobals, analysis),
    mCanSplit(FAIL),
    mExecuted(false),
    stopBest(false) {

  kernel_time = new double[nbDevices];
  comm_time = new double[nbDevices];

  bestReached = false;
  bestTime = std::numeric_limits<double>::max();
  best_size_gr = 0;
  best_granu_dscr = new double[3 * nbDevices];

  if (optNoComm)
    updatePerfDescr = &SchedulerAutoAbs::updatePerfDescrWithoutComm;
  else
    updatePerfDescr = &SchedulerAutoAbs::updatePerfDescrWithComm;

  if (optBest)
    getNextGranu = &SchedulerAutoAbs::getNextGranuBest;
  else
    getNextGranu = &SchedulerAutoAbs::getNextGranuFull;

  if (optStop)
    stopBest = true;
}

SchedulerAutoAbs::~SchedulerAutoAbs() {
  delete[] kernel_time;
  delete[] comm_time;

  delete[] best_granu_dscr;
}

void
SchedulerAutoAbs::initGranu() {
  char granustartEnv[64];
  sprintf(granustartEnv, "GRANUSTART%u", id);
  char *granustart = getenv(granustartEnv);

  if (!optGranustart.empty()) {
    size_gr = optGranustart.size();
    delete[] granu_dscr;
    granu_dscr = new double[size_gr];
    for (int i=0; i<size_gr; i++)
      granu_dscr[i] = optGranustart[i];

    size_perf_descr = size_gr;
  }

  //  GRANUSTART X option
  else if (granustart) {
    std::vector<double> optGranuStartX;
    std::string s(granustart);
    std::istringstream is(s);
    double n;
    while (is >> n)
      optGranuStartX.push_back(n);

    size_gr = optGranuStartX.size();
    delete[] granu_dscr;
    granu_dscr = new double[size_gr];
    for (int i=0; i<size_gr; ++i)
      granu_dscr[i] = optGranuStartX[i];

    size_perf_descr = size_gr;
  }

  // Homogeneous splitting
  else {
    size_gr = nbDevices * 3;
    size_perf_descr = nbDevices * 3;
    for (unsigned i=0; i<nbDevices; i++) {
      kernel_perf_descr[i*3] = i;
      granu_dscr[i*3] = i; // dev id
      granu_dscr[i*3+1] = 1; // nb kernels
      granu_dscr[i*3+2] = 1.0 / nbDevices; // granu
    }
  }
}

int SchedulerAutoAbs::startIter(cl_uint work_dim,
				const size_t *global_work_offset,
				const size_t *global_work_size,
				const size_t *local_work_size,
				unsigned *splitDim) {
  // Clear comm events;
  for (unsigned d=0; d<nbDevices; d++)
    devInfo[d].commEvents.clear();

  // First execution, init solver
  // Try to launch the solver for each dimension.
  if (!mExecuted) {
    mExecuted = true;
    mNeedToInstantiate = false;
    mCanSplit = FAIL;

    // Get sorted dim
    unsigned dimOrder[work_dim];
    getSortedDim(work_dim, global_work_size, local_work_size, dimOrder);

    // Try to split each dim
    for (unsigned i=0; i<work_dim; i++) {
      // Compute granularity min
      unsigned dim = dimOrder[i];
      int nbSplitMax = global_work_size[dim] / local_work_size[dim];

      if (nbSplitMax == 1)
	continue;

      globalSize = global_work_size[dim];
      localSize = local_work_size[dim];

      initGranu();

      bool ret =
	instantiateAnalysis(work_dim, global_work_offset, global_work_size,
			    local_work_size, dim);

      if (ret) {
	initScheduler(global_work_size, local_work_size, dim);
	mCanSplit = SPLIT;
	*splitDim = dim;

	return mCanSplit;
      }
    }

    // Try to merge each dim
    for (unsigned i=0; i<work_dim; i++) {
      // Compute granularity min
      unsigned dim = dimOrder[i];
      int nbSplitMax = global_work_size[dim] / local_work_size[dim];

      if (nbSplitMax == 1)
	continue;

      globalSize = global_work_size[dim];
      localSize = local_work_size[dim];

      initGranu();

      bool ret =
	instantiateMergeAnalysis(work_dim, global_work_offset, global_work_size,
				 local_work_size, dim);

      if (ret) {
	initScheduler(global_work_size, local_work_size, dim);
	mCanSplit = MERGE;
	*splitDim = dim;

	return mCanSplit;
      }
    }

    return mCanSplit;
  }

  // Kernel has already been executed
  if (mCanSplit == FAIL)
    return false;

  // Get next granularity and instantiate
  return (this->*getNextGranu)(work_dim, global_work_offset, global_work_size,
  			       local_work_size, splitDim);
}

bool
SchedulerAutoAbs::getNextGranuFull(cl_uint work_dim,
				   const size_t *global_work_offset,
				   const size_t *global_work_size,
				   const size_t *local_work_size,
				   unsigned *splitDim) {
  // Get next granularity and instantiate
  getGranu();

  if (instantiateAnalysis(work_dim, global_work_offset,
			  global_work_size,
			  local_work_size, *splitDim)) {
    mCanSplit = SPLIT;
  } else if (instantiateMergeAnalysis(work_dim, global_work_offset,
				      global_work_size,
				      local_work_size, *splitDim)) {
    mCanSplit = MERGE;
  } else {
    mCanSplit = FAIL;
  }

  return mCanSplit;
}

bool
SchedulerAutoAbs::getNextGranuBest(cl_uint work_dim,
				   const size_t *global_work_offset,
				   const size_t *global_work_size,
				   const size_t *local_work_size,
				   unsigned *splitDim) {
  if (!bestReached) {
    // Get next granularity
    getGranu();
  } else {
    getNextGranu = &SchedulerAutoAbs::getNextGranuNil;

    size_gr = best_size_gr;
    std::copy(granu_dscr, granu_dscr + size_gr, best_granu_dscr);
  }

  if (instantiateAnalysis(work_dim, global_work_offset,
			  global_work_size,
			  local_work_size, *splitDim)) {
    mCanSplit = SPLIT;
  } else if (instantiateMergeAnalysis(work_dim, global_work_offset,
				      global_work_size,
				      local_work_size, *splitDim)) {
    mCanSplit = MERGE;
  } else {
    mCanSplit = FAIL;
  }

  return mCanSplit;
}

bool
SchedulerAutoAbs::getNextGranuNil(cl_uint work_dim,
				  const size_t *global_work_offset,
				  const size_t *global_work_size,
				  const size_t *local_work_size,
				  unsigned *splitDim) {
  if (mNeedToInstantiate) {
    if (instantiateAnalysis(work_dim, global_work_offset,
			    global_work_size,
			    local_work_size, *splitDim)) {
      mCanSplit = SPLIT;
    } else if (instantiateMergeAnalysis(work_dim, global_work_offset,
					global_work_size,
					local_work_size, *splitDim)) {
      mCanSplit = MERGE;
    } else {
      mCanSplit = FAIL;
    }
  }

  return mCanSplit;
}

void SchedulerAutoAbs::endIter() {
  if (mCanSplit == FAIL)
    return;

  // Get perfs from events
  cl_int err;

  for (unsigned d=0; d<nbDevices; d++) {
    comm_time[d] = 0;
    kernel_time[d] = 0;

    for (unsigned c=0; c<devInfo[d].commEvents.size(); c++) {
      cl_ulong start, end;
      err = clGetEventProfilingInfo(devInfo[d].commEvents[c],
				    CL_PROFILING_COMMAND_START,
				    sizeof(start),
				    &start,
				    NULL);
      clCheck(err, __FILE__, __LINE__);
      err = clGetEventProfilingInfo(devInfo[d].commEvents[c],
				    CL_PROFILING_COMMAND_END,
				    sizeof(end),
				    &end,
				    NULL);
      clCheck(err, __FILE__, __LINE__);

      comm_time[d] += ((double) end - start) * 1e-6;
    }

    for (unsigned k=0; k<devInfo[d].nbKernels; k++) {
      cl_ulong start, end;
      err = clGetEventProfilingInfo(devInfo[d].kernelsEvent[k],
				    CL_PROFILING_COMMAND_START,
				    sizeof(start),
				    &start,
				    NULL);
      clCheck(err, __FILE__, __LINE__);
      err = clGetEventProfilingInfo(devInfo[d].kernelsEvent[k],
				    CL_PROFILING_COMMAND_END,
				    sizeof(end),
				    &end,
				    NULL);
      clCheck(err, __FILE__, __LINE__);

      kernel_time[d] += ((double) end - start) * 1e-6;
    }
  }

  DEBUG("schedtime",
	for (unsigned d=0; d<nbDevices; d++) {
	  std::cerr << "dev " << d << " kernel time "
		    << kernel_time[d] << " ms\n";
	  std::cerr << "dev " << d << " comm time "
		    << comm_time[d] << " ms\n";
	}
	);

  (this->*updatePerfDescr)();
}
