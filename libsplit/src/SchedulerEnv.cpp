#include <Debug.h>
#include <Options.h>
#include "SchedulerEnv.h"
#include "Utils.h"

#include <iostream>

SchedulerEnv::SchedulerEnv(unsigned nbDevices, DeviceInfo *devInfo,
			   unsigned numArgs,
			   unsigned nbGlobals,
			   KernelAnalysis *analysis, unsigned id)
  : Scheduler(nbDevices, devInfo, numArgs, nbGlobals, analysis),
    mCanSplit(FAIL), iter(0), id(id) {

  keep_granu_dscr = new double[nbDevices * 3];
  keep_granu_size = nbDevices * 3;

  // First iteration
  // Default: one kernel per device.
  size_gr = nbDevices * 3;
  size_perf_descr = nbDevices * 3;
  for (unsigned i=0; i<nbDevices; i++) {
    kernel_perf_descr[i*3] = i;
    granu_dscr[i*3] = i; // dev id
    keep_granu_dscr[i*3] = i; // dev id
    granu_dscr[i*3+1] = 1; // nb kernels
    keep_granu_dscr[i*3+1] = 1; // nb kernels
    granu_dscr[i*3+2] = 1.0 / nbDevices; // granu
    keep_granu_dscr[i*3+2] = 1.0 / nbDevices; // granu
  }

  startParams = 1;
  if (optSkip)
    startParams += optSkip;
}

SchedulerEnv::~SchedulerEnv() {}

int SchedulerEnv::startIter(cl_uint work_dim,
			    const size_t *global_work_offset,
			    const size_t *global_work_size,
			    const size_t *local_work_size,
			    unsigned *splitDim) {
  iter++;

  // Second iteration use environment split parameters
  if (iter == startParams) {

    // Get split parameters through environment variables.

    // Try GRANUDSCR first
    if (!optGranudscr.empty()) {
      size_gr = optGranudscr.size();
      keep_granu_size = size_gr;

      delete[] granu_dscr;
      delete[] keep_granu_dscr;
      granu_dscr = new double[size_gr];
      keep_granu_dscr = new double[size_gr];
      for (int i=0; i<size_gr; i++) {
	granu_dscr[i] = optGranudscr[i];
	keep_granu_dscr[i] = granu_dscr[i];
      }
    } else {
      // Then try PARTITION<kerID> and PARTITION
      std::vector<unsigned> *vecParams = &optPartition;
      std::vector<unsigned> vecPartition;
      char envPartition[64];
      sprintf(envPartition, "PARTITION%u", id);
      char *partition = getenv(envPartition);
      if (partition) {
	std::string s(partition);
	std::istringstream is(s);
	int n;
	while (is >> n)
	  vecPartition.push_back(n);
	vecParams = &vecPartition;
      }

      int nbSplits = ((*vecParams).size() - 1) / 2;

      size_gr = nbSplits * 3;
      keep_granu_size = nbSplits * 3;
      delete[] granu_dscr;
      delete[] keep_granu_dscr;
      granu_dscr = new double[size_gr];
      keep_granu_dscr = new double[size_gr];
      size_perf_descr = size_gr;
      delete[] kernel_perf_descr;
      kernel_perf_descr = new double[size_gr];

      int denominator = (*vecParams)[0];
      int total = 0;
      for (int i=0; i<nbSplits; i++) {
	int nominator = (*vecParams)[1 + 2*i];
	int deviceno = (*vecParams)[2 + 2*i];
	double granu = (double) nominator / denominator;

	keep_granu_dscr[i*3] = deviceno;
	keep_granu_dscr[i*3+1] = 1;
	keep_granu_dscr[i*3+2] = granu;

	kernel_perf_descr[i*3] = deviceno;
	kernel_perf_descr[i*3+1] = granu;

	total += nominator;
      }

      if (total != denominator) {
	std::cerr << "Error bad split parameters !\n";
	exit(EXIT_FAILURE);
      }

      mNeedToInstantiate = true;
    }

    DEBUG("granu",
	  std::cerr << "granu : ";
	  for (int i=0; i<size_gr; i++)
	    std::cerr << keep_granu_dscr[i] << " ";
	  std::cerr << "\n";
	  );
  }

  std::copy(keep_granu_dscr, keep_granu_dscr + keep_granu_size,
	    granu_dscr);
  size_gr = keep_granu_size;

  // TODO: handle case where granu changed

  if (!mNeedToInstantiate)
    return mCanSplit;

  mNeedToInstantiate = false;
  mCanSplit = FAIL;

  // Get sorted dim
  unsigned dimOrder[work_dim];
  getSortedDim(work_dim, global_work_size, local_work_size, dimOrder);

  // Try to split each dim
  for (unsigned i=0; i<work_dim; i++) {
    if (instantiateAnalysis(work_dim, global_work_offset, global_work_size,
			    local_work_size, dimOrder[i])) {
      mCanSplit = SPLIT;
      *splitDim = dimOrder[i];
      return mCanSplit;
    }
  }

  // Try to merge each dim
  for (unsigned i=0; i<work_dim; i++) {
    if (instantiateMergeAnalysis(work_dim, global_work_offset, global_work_size,
				 local_work_size, dimOrder[i])) {
      mCanSplit = MERGE;
      *splitDim = dimOrder[i];
      return mCanSplit;
    }
  }

  return mCanSplit;
}

void SchedulerEnv::endIter() {
  DEBUG("schedtime",
	for (unsigned d=0; d<nbDevices; d++) {
	  for (unsigned k=0; k<devInfo[d].nbKernels; k++) {

	    cl_ulong start;
	    cl_ulong end;
	    cl_int err;

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

	    std::cerr << "k" << d+1 << id << " "
		      << (end - start) * 1e-6 << " ms\n";
	    std::cerr << "kertime " << d << " "
		      << (end - start) * 1e-6 << " ms\n";
	  }
	}

	for (unsigned d=0; d<nbDevices; d++) {
	  cl_ulong start;
	  cl_ulong end;
	  cl_ulong total = 0;
	  cl_int err;
	  for (unsigned c=0; c<devInfo[d].commEvents.size(); c++) {
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

	    total += end - start;
	  }

	  std::cerr << "c" << d+1 << "k" << id << " "
		    << total * 1e-6 << " ms\n";
	}
	);

  for (unsigned d=0; d<nbDevices; d++)
    devInfo[d].commEvents.clear();
}
