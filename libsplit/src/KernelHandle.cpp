#include <Debug.h>
#include "Define.h"
#include "KernelHandle.h"
#include <Options.h>
#include "Globals.h"
#include "SchedulerAKGR.h"
#include "SchedulerMKGR.h"
#include "SchedulerBroyden.h"
#include "SchedulerBadBroyden.h"
#include "SchedulerFixPoint.h"
#include "SchedulerEnv.h"
#include "Utils.h"

#include <fcntl.h>
#include <malloc.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

unsigned KernelHandle::numKernels = 0;

KernelHandle::KernelHandle(ProgramHandle *p, const char *kernel_name)
  : mExecuted(false), mCanSplit(Scheduler::FAIL), mNbSkipIter(0), mIteration(0),
    mProgram(p), mergeDeviceId(0), mSplitDim(0),
    mSingleDeviceID(0)
{
  NDRangeTime = 0;
  computeReadTime = 0;
  writeDeviceTime = 0;
  subkernelsTime = 0;
  readDevicesTime = 0;

  // Retain program
  p->retain();

  // Get name
  mName = strdup(kernel_name);

  id = ++numKernels;

  DEBUG("kernelstats",
	std::cerr << "kernel " << mName << " id " << id << "\n";
	);

  // Get context
  mContext = mProgram->getContext();

  // Create one DeviceInfo object per device
  mNbDevices = mContext->getNbDevices();
  mDevicesInfo = new DeviceInfo[mNbDevices];
  for (unsigned d=0; d<mNbDevices; d++) {
    mDevicesInfo[d].queue = mContext->getQueueNo(d);
    mDevicesInfo[d].deviceIndex = d;
  }

  // Create one cl_kernel per device
  cl_int err;

  mNbKernels = mContext->getNbDevices();
  mKernels = new cl_kernel[mNbKernels];
  for (unsigned i=0; i<mNbKernels; i++) {
    mKernels[i] = real_clCreateKernel(p->getProgram(i), kernel_name, &err);
    clCheck(err, __FILE__, __LINE__);
  }

  // Arguments
  err = real_clGetKernelInfo(mKernels[0], CL_KERNEL_NUM_ARGS, sizeof(mNumArgs),
			     &mNumArgs, NULL);
  clCheck(err, __FILE__, __LINE__);
  mNumArgs -= 2;

  mGlobalArgsPos = NULL;
  mAnalysis = NULL;
  mNbGlobalArgs = 0;
  mNbWrittenArgs = 0;
  mWrittenArgsPos = new unsigned[mNumArgs];
  mNbMergeArgs = 0;
  mMergeArgsPos = NULL;

  if (mNumArgs) {
    mMemArgs = new MemoryHandle *[mNumArgs];
    mArgIsGlobal = new bool[mNumArgs];
    mArgIsMerge = new bool[mNumArgs];
    for (unsigned i=0; i<mNumArgs; ++i) {
      mMemArgs[i] = NULL;
      mArgIsGlobal[i] = false;
      mArgIsMerge[i] = false;
    }
  } else {
    mMemArgs = NULL;
    mArgIsGlobal = NULL;
  }

  // Launch analysis
  launchAnalysis();

  // Get environment variables SINGLEID, DONTSPLIT and SCHED and instantiate
  // the scheduler.
  getEnvOptions();

  // {
  //   char command[1024];
  //   sprintf(command, "echo \"%s \" >> bilan-auto.txt",
  // 	    mName);
  //   system(command);
  // }

  if (//!strcmp(mName, "create_seq") || // bug SCEV MAX (if k2 < NUM_KEYS) fixed ?
      false)
    mDontSplit = true;

  if (!strcmp(mName, "reduce_kernel") ||
      !strcmp(mName, "normalize_weights_kernel") ||
      false)
    mDontSplit = true;

  // SOTL
  if (!strcmp(mName, "scan") ||
      !strcmp(mName, "scan_down_step") ||
      !strcmp(mName, "box_sort") ||
      !strcmp(mName, "reset_int_buffer") ||
      !strcmp(mName, "box_count"))
    mDontSplit = true;

}



KernelHandle::~KernelHandle() {
  DEBUG("timer",
	std::cerr << "NDRangeTime : " << NDRangeTime << "\n";
	std::cerr << "subkernelsTime : " << subkernelsTime << "\n";
	std::cerr << "computeReadTime : " << computeReadTime << "\n";
	std::cerr << "writeDeviceTime : " << writeDeviceTime << "\n";
	std::cerr << "readDevicesTime : " << readDevicesTime << "\n";
	);

  mProgram->release();

  delete[] mMemArgs;
  delete[] mArgIsGlobal;
  delete[] mGlobalArgsPos;
  delete[] mWrittenArgsPos;
  delete[] mKernels;

  delete[] mDevicesInfo;

  delete mAnalysis;

  free(mName);

  delete sched;
}

void
KernelHandle::setKernelArg(cl_uint arg_index, size_t arg_size,
			   const void *arg_value) {
  cl_int err = CL_SUCCESS;

  // Store argument value as an integer
  int intValue;
  if (arg_value)
    intValue = *((int *) arg_value);
  else
    intValue= 0;

  // mArgs[arg_index] = intValue;
  sched->storeArgValue(arg_index, intValue);

  // If it is a global argument or a constant argument, update mMemArgs
  // and set different membuffer for each split kernel
  if (mArgIsGlobal[arg_index]) {
    MemoryHandle *mem = *((MemoryHandle **) arg_value);
    mMemArgs[arg_index] = mem;

    // Call the real clSetKernelArg function for each kernel.
    for (unsigned i=0; i<mNbKernels; i++)
      err |= real_clSetKernelArg(mKernels[i], arg_index, arg_size,
				 &mem->mBuffers[i]);
    clCheck(err, __FILE__, __LINE__);
  } else { // Not global address space nor constant
    // Call the real clSetKernelArg function for each kernel.
    for (unsigned i=0; i<mNbKernels; i++)
      err |= real_clSetKernelArg(mKernels[i], arg_index, arg_size,
				 arg_value);
    clCheck(err, __FILE__, __LINE__);
  }
}

void
KernelHandle::enqueueNDRange(cl_command_queue command_queue,
			     cl_uint work_dim,
			     const size_t *global_work_offset,
			     const size_t *global_work_size,
			     const size_t *local_work_size,
			     cl_uint num_events_in_wait_list,
			     const cl_event *event_wait_list,
			     cl_event *event) {
  double t1, t2;
  t1 = t2 = 0;

  DEBUG("timer", t1 = mytime(););

  if (local_work_size == NULL) {
    std::cerr << "local work size null not handled\n";
    exit(EXIT_FAILURE);
  }
  cl_int err = CL_SUCCESS;

  // Instantiate analysis with split parameters
  if (!mDontSplit && mIteration >= mNbSkipIter) {
    mCanSplit = sched->startIter(work_dim, global_work_offset, global_work_size,
				 local_work_size, &mSplitDim);

    DEBUG("kernelstats",
	  if (mCanSplit == Scheduler::SPLIT)
	    std::cerr << "\x1b[34mCan split kernel " << mName << "\x1b[0m\n";
	  else if (mCanSplit == Scheduler::MERGE)
	    std::cerr << "\x1b[34mCan merge kernel " << mName << "\x1b[0m\n";
	  else
	    std::cerr << "\x1b[34mCannot split kernel " << mName << "\x1b[0m\n";
	  );
  } else {
    DEBUG("kernelstats",
	  std::cerr << "\x1b[34mDont split kernel " << mName << "\x1b[0m\n";
	  );
  }

  DEBUG("log",
	if (mIteration == 0)
	  logger->step(mNbDevices);
	for (unsigned d=0; d<mNbDevices; d++) {
	  for (unsigned k=0; k<mDevicesInfo[d].nbKernels; k++) {
	    logger->logKernel(d, id,
			      (double) mDevicesInfo[d].
			      mGlobalWorkSize[k][mSplitDim]
			      / global_work_size[mSplitDim]);
	  }
	}
	);

  // Set the numgroups and splitdim argument for each sub-kernel
  for (unsigned i=0; i<mNbKernels; i++) {
    unsigned numGroups = global_work_size[mSplitDim] / local_work_size[mSplitDim];

    err |= real_clSetKernelArg(mKernels[i], mNumArgs,
  			       sizeof(numGroups), &numGroups);
    err |= real_clSetKernelArg(mKernels[i], mNumArgs+1,
  			       sizeof(mSplitDim), &mSplitDim);
    clCheck(err, __FILE__, __LINE__);
  }

  if (event_wait_list) {
    cl_int err = clWaitForEvents(num_events_in_wait_list, event_wait_list);
    clCheck(err, __FILE__, __LINE__);
  }

  // If the kernel can be split enqueue sub kernels, otherwise enqueue a single
  // kernel
  if (mDontSplit || mCanSplit == Scheduler::FAIL || mIteration < mNbSkipIter)
    enqueueSingleKernel(work_dim, global_work_offset, global_work_size,
			local_work_size);
  else if (mCanSplit == Scheduler::SPLIT)
    enqueueSubKernels(work_dim, local_work_size);
  else // MERGE
    enqueueMergeKernels(work_dim, local_work_size);
  //enqueueFakeSubKernels(work_dim, local_work_size);

  if (!mDontSplit && mIteration >= mNbSkipIter)
    sched->endIter();

  mIteration++;

  DEBUG("log",
	if ((int) mIteration == optLogMaxIter) {
	  logger->toDot("graph.dot");
	  std::cerr << "Log graph written to file graph.dot\n";
	  exit(0);
	}
	logger->step(mNbDevices);
	);

  mExecuted = true;

  if (event) {
    err = clEnqueueMarker(command_queue, event);
    clCheck(err, __FILE__, __LINE__);
  }

  DEBUG("timer",
	t2 = mytime();
	NDRangeTime += t2 - t1;
	);
}

void
KernelHandle::enqueueSingleKernel(cl_uint work_dim,
				  const size_t *global_work_offset,
				  const size_t *global_work_size,
				  const size_t *local_work_size) {
  cl_int err;

  unsigned curDev = mSingleDeviceID;
  cl_command_queue curQueue = mContext->getQueueNo(curDev);

  // Write datas
  bufferMgr->writeSingle(mDevicesInfo, curDev,
			 mNbGlobalArgs, mGlobalArgsPos,
			 mMemArgs);

  // Enqueue kernel
  err = real_clEnqueueNDRangeKernel(curQueue,
				    mKernels[curDev],
				    work_dim, global_work_offset,
				    global_work_size,
				    local_work_size,
				    0,
				    NULL, NULL);
  clCheck(err, __FILE__, __LINE__);

  err = clFlush(curQueue);
  clCheck(err, __FILE__, __LINE__);


  // Read datas
  bufferMgr->readSingle(mDevicesInfo, curDev,
			mNbWrittenArgs, mWrittenArgsPos,
			mMemArgs);

  err = clFinish(curQueue);
  clCheck(err, __FILE__, __LINE__);
}

void
KernelHandle::enqueueSubKernels(cl_uint work_dim, const size_t *local_work_size)
{
  double t_start, t_end, t1, t2;
  t_start = t_end = t1 = t2 = 0;

  DEBUG("granu",
	for (unsigned d=0; d<mNbDevices; d++) {
	  for (unsigned k=0; k<mDevicesInfo[d].nbKernels; k++) {
	    std::cerr << "dev "<<d<<" ";
	    for (cl_uint w=0; w<work_dim; w++) {
	      std::cerr << mDevicesInfo[d].mGlobalWorkSize[k][w] << " ";
	    }
	  }
	  std::cerr << "\n";
	}
	);

  DEBUG("timer",
	t_start = mytime();
	t1 = mytime();
	);

  // Compute communications and read local missing data.
  size_t totalToSend =
    bufferMgr->computeAndReadMissing(mDevicesInfo, mNbDevices, mNbGlobalArgs,
				     mGlobalArgsPos, mMemArgs);

  DEBUG("timer",
	t2 = mytime();
	computeReadTime += t2 - t1;
	);

  // Enqueue comms and kernels
  if (totalToSend > 256000)
    enqueueSubKernelsOMP(work_dim, local_work_size);
  else
    enqueueSubKernelsNOOMP(work_dim, local_work_size);

  DEBUG("timer", t1 = mytime(););

  // Read datas
  bufferMgr->readDevices(mDevicesInfo, mNbDevices,
			 mNbGlobalArgs, mGlobalArgsPos,
			 mArgIsMerge,
			 mMemArgs);

  DEBUG("timer",
	t2 = mytime();
	readDevicesTime += t2 - t1;
	);

  for (unsigned d=0; d<mNbDevices; d++) {
    cl_int err = clFinish(mDevicesInfo[d].queue);
    clCheck(err, __FILE__, __LINE__);
  }

  DEBUG("timer",
	t_end = mytime();
	subkernelsTime += t_end - t_start;
	);
}

void
KernelHandle::enqueueFakeSubKernels(cl_uint work_dim,
				    const size_t *local_work_size) {
  // Compute communications and read local missing data.
  size_t totalToSend =
    bufferMgr->computeAndReadMissing(mDevicesInfo, mNbDevices, mNbGlobalArgs,
				     mGlobalArgsPos, mMemArgs);

  // Enqueue comms and kernels
  if (totalToSend > 256000)
    enqueueSubKernelsOMP(work_dim, local_work_size);
  else
    enqueueSubKernelsNOOMP(work_dim, local_work_size);

  // Read datas
  bufferMgr->fakeReadDevices(mDevicesInfo, mNbDevices,
			     mNbGlobalArgs, mGlobalArgsPos,
			     mArgIsMerge,
			     mMemArgs);

  for (unsigned d=0; d<mNbDevices; d++) {
    cl_int err = clFinish(mDevicesInfo[d].queue);
    clCheck(err, __FILE__, __LINE__);
  }
}

void
KernelHandle::enqueueSubKernelsNOOMP(cl_uint work_dim,
				     const size_t *local_work_size) {
  double t1, t2;
  t1 = t2 = 0;

  for (unsigned d=0; d<mNbDevices; d++) {
    cl_int err;
    cl_command_queue queue = mDevicesInfo[d].queue;
    unsigned devId = mDevicesInfo[d].deviceIndex;

    DEBUG("timer", t1 = mytime(););

    // Write data
    bufferMgr->writeDeviceNo(mDevicesInfo, d, mNbGlobalArgs, mGlobalArgsPos,
			     mMemArgs);

    DEBUG("timer",
	  t2 = mytime();
	  writeDeviceTime += t2 - t1;
	  );

    // Enqueue subkernels
    for (unsigned k=0; k<mDevicesInfo[d].nbKernels; k++) {
      err = real_clEnqueueNDRangeKernel(queue,
      					mKernels[devId],
      					work_dim,
      					mDevicesInfo[d].mGlobalWorkOffset[k],
      					mDevicesInfo[d].mGlobalWorkSize[k],
      					local_work_size,
      					0, NULL,
					&mDevicesInfo[d].kernelsEvent[k]);
      clCheck(err, __FILE__, __LINE__);
    }

    err = clFlush(queue);
    clCheck(err, __FILE__, __LINE__);
  }
}

void
KernelHandle::enqueueSubKernelsOMP(cl_uint work_dim,
				   const size_t *local_work_size) {
#pragma omp parallel for
  for (unsigned d=0; d<mNbDevices; d++) {
    cl_int err;
    cl_command_queue queue = mDevicesInfo[d].queue;
    unsigned devId = mDevicesInfo[d].deviceIndex;

    // Write data
    bufferMgr->writeDeviceNo(mDevicesInfo, d, mNbGlobalArgs, mGlobalArgsPos,
			     mMemArgs);

    // Enqueue subkernels
    for (unsigned k=0; k<mDevicesInfo[d].nbKernels; k++) {
      err = real_clEnqueueNDRangeKernel(queue,
      					mKernels[devId],
      					work_dim,
      					mDevicesInfo[d].mGlobalWorkOffset[k],
      					mDevicesInfo[d].mGlobalWorkSize[k],
      					local_work_size,
      					0, NULL,
					&mDevicesInfo[d].kernelsEvent[k]);
      clCheck(err, __FILE__, __LINE__);
    }

    err = clFlush(queue);
    clCheck(err, __FILE__, __LINE__);
  }
}

// Compute for each device the id of its subkernel and for each subkernel the id
// of its device.
//
// devSplitIdx is an array of size nbDevices :
// < splitId dev0, splitId dev1, ..., splitId devn >
// splitDevIdx is an a array of size nbSplits
// < devId split0, devId split1, ..., devId splitN >
static void computeDevSplitIdx(unsigned **devSplitIdx,
			       unsigned **splitDevIdx,
			       unsigned *nbSplits,
			       DeviceInfo *devInfo,
			       unsigned nbDevices)
{
  *nbSplits = 0;
  *devSplitIdx = (unsigned *) malloc(nbDevices * sizeof(unsigned));

  for (unsigned i=0; i<nbDevices; i++) {
    if (devInfo[i].nbKernels > 0) {
      (*devSplitIdx)[i] = *nbSplits;
      (*nbSplits)++;
    }
  }

  *splitDevIdx = (unsigned *) malloc(*nbSplits * sizeof(unsigned));

  unsigned tmpIdx = 0;
  for (unsigned i=0; i<nbDevices; i++) {
    if (devInfo[i].nbKernels > 0) {
      (*splitDevIdx)[tmpIdx++] = i;
    }
  }
}

// Create an output buffer and input buffers for each written argument
// and set kernel arguments.
// Input buffers are only created if the subkernel doesn't belong the
// the mergedevice's context.
void
KernelHandle::createMergeBuffersAndSetArgs(unsigned nbSplits,
					   unsigned *splitDevIdx,
					   cl_kernel mergeKernel,
					   cl_context mergeCtxt,
					   cl_mem *mergeBuffers,
					   cl_mem *devBuffers,
					   int *maxLen)
{
  *maxLen = 0;

  int nbBuffersCreated = 0;

  for (unsigned i=0; i<mNbMergeArgs; i++) {
    cl_int err;

    // Get Handle
    MemoryHandle *memHandle = mMemArgs[mMergeArgsPos[i]];
    size_t bufferSize = memHandle->mSize;

    // Update maxlen
    int argLen = (int) ((bufferSize + sizeof(MERGEKERNELS_TYPE) - 1) /
			sizeof(MERGEKERNELS_TYPE));
    *maxLen = argLen > *maxLen ? argLen : *maxLen;

    // Create merge buffer for this argument.
    mergeBuffers[i] = real_clCreateBuffer(mergeCtxt,
					  CL_MEM_READ_WRITE,
					  bufferSize,
					  NULL,
					  &err);
    clCheck(err, __FILE__, __LINE__);
    nbBuffersCreated++;

    // Set merge buffer argument
    err = real_clSetKernelArg(mergeKernel,
			      i * (nbSplits+2),
			      sizeof(cl_mem),
			      &mergeBuffers[i]);
    clCheck(err, __FILE__, __LINE__);

    // Create buffers for each split.
    // Only if context if different from mergedevice context
    for (unsigned s=0; s<nbSplits; s++) {

      cl_context curSplitCtxt = mContext->getContext(splitDevIdx[s]);

      if (curSplitCtxt != mergeCtxt) {
	devBuffers[s*mNbMergeArgs + i] =
	  real_clCreateBuffer(mergeCtxt,
			      CL_MEM_READ_ONLY,
			      bufferSize,
			      NULL,
			      &err);
	clCheck(err, __FILE__, __LINE__);
	nbBuffersCreated++;
      } else {
	devBuffers[s*mNbMergeArgs + i] = memHandle->mBuffers[splitDevIdx[s]];
      }
    }

    // Set buffer argument for each split
    for (unsigned s=0; s<nbSplits; s++) {
      err = real_clSetKernelArg(mergeKernel,
				i * (nbSplits+2) + s+1,
				sizeof(cl_mem),
				&devBuffers[s*mNbMergeArgs + i]);
      clCheck(err, __FILE__, __LINE__);
    }

    // Set merge length for the current argument.
    err = real_clSetKernelArg(mergeKernel,
			      i * (nbSplits+2) + nbSplits+1,
			      sizeof(int),
			      &argLen);
    clCheck(err, __FILE__, __LINE__);
  }

  std::cerr << nbBuffersCreated << " created\n";
}

// Read the necessary data from devices and host and write then
// to the merge context.
void
KernelHandle::prepareMergeBuffers(unsigned nbSplits,
				  unsigned *splitDevIdx,
				  cl_context mergeCtxt,
				  cl_mem *devBuffers,
				  cl_mem *mergeBuffers) {
  // Read dev buffers and write to merge context
#pragma omp parallel for
  for (unsigned a=0; a<mNbMergeArgs; a++) {
    MemoryHandle *memHandle = mMemArgs[mMergeArgsPos[a]];
    size_t bufferSize = memHandle->mSize;

    char *tmpBuffer = (char *) memalign(4096, bufferSize);
    if (tmpBuffer == NULL) {
      std::cerr << "tmpBuffer allocation failure\n";
      exit(EXIT_FAILURE);
    }

    for (unsigned s=0; s<nbSplits; s++) {

      unsigned devId = splitDevIdx[s];
      cl_context curDevCtxt = mContext->getContext(devId);

      if (curDevCtxt == mergeCtxt)
	continue;

      cl_int err;

      // Read back to host
      err = real_clEnqueueReadBuffer(mDevicesInfo[devId].queue,
				     memHandle->mBuffers[devId],
				     CL_TRUE,
				     0,
				     bufferSize,
				     (void *) tmpBuffer,
				     0, NULL, NULL);
      clCheck(err, __FILE__, __LINE__);

      // Write to merge dev
      err = real_clEnqueueWriteBuffer(mDevicesInfo[mergeDeviceId].queue,
				      devBuffers[s*mNbMergeArgs + a],
				      CL_TRUE,
				      0,
				      bufferSize,
				      (void *) tmpBuffer,
				      0, NULL, NULL);
      clCheck(err, __FILE__, __LINE__);
    }

    free(tmpBuffer);
  }

  // Write local buffers to merge buffers

  double writeMergeStart = mytime();

#pragma omp parallel for
  for (unsigned a=0; a<mNbMergeArgs; a++) {
    MemoryHandle *memHandle = mMemArgs[mMergeArgsPos[a]];
    size_t bufferSize = memHandle->mSize;

    cl_int err =
      real_clEnqueueWriteBuffer(mDevicesInfo[mergeDeviceId].queue,
				mergeBuffers[a],
				CL_TRUE,
				0,
				bufferSize,
				memHandle->mLocalPtr,
				0, NULL, NULL);
    clCheck(err, __FILE__, __LINE__);
  }

  double writeMergeEnd = mytime();
  std::cerr << "write merge time : " << (writeMergeEnd -
					 writeMergeStart) * 1e3 << " ms\n";
}

// Read output merged buffers back to the host.
void
KernelHandle::readMergeBuffers(DeviceInfo *devInfo,
			       cl_mem *mergeBuffers) {
#pragma omp parallel for
  for (unsigned a=0; a<mNbMergeArgs; a++) {
    MemoryHandle *memHandle = mMemArgs[mMergeArgsPos[a]];
    size_t bufferSize = memHandle->mSize;
    cl_int err;

    err = real_clEnqueueReadBuffer(devInfo[mergeDeviceId].queue,
				   mergeBuffers[a],
				   CL_TRUE,
				   0,
				   bufferSize,
				   memHandle->mLocalPtr,
				   0, NULL, NULL);
    clCheck(err, __FILE__, __LINE__);
  }
}

void
KernelHandle::computeMergeArguments() {
  mNbMergeArgs = 0;
  delete mMergeArgsPos;
  for (unsigned a=0; a<mNbGlobalArgs; a++) {
    ArgumentAnalysis *argAnalysis = mAnalysis->getArgAnalysis(a);
    if (!argAnalysis->canSplit()) {
      mArgIsMerge[mGlobalArgsPos[a]] = true;
      mNbMergeArgs++;
    } else {
      mArgIsMerge[mGlobalArgsPos[a]] = false;
    }
  }

  mMergeArgsPos = new unsigned[mNbMergeArgs];
  unsigned id = 0;
  for (unsigned a=0; a<mNbGlobalArgs; a++) {
    ArgumentAnalysis *argAnalysis = mAnalysis->getArgAnalysis(a);
    if (!argAnalysis->canSplit())
      mMergeArgsPos[id++] = argAnalysis->getPos();
  }
}

// Enqueue subkernels then merge output.
void
KernelHandle::enqueueMergeKernels(cl_uint work_dim,
				  const size_t *local_work_size) {
  // Compute communications and read local missing data.
  size_t totalToSend =
    bufferMgr->computeAndReadMissing(mDevicesInfo, mNbDevices, mNbGlobalArgs,
				     mGlobalArgsPos, mMemArgs);

  // Enqueue comms and kernels
  if (totalToSend > 256000)
    enqueueSubKernelsOMP(work_dim, local_work_size);
  else
    enqueueSubKernelsNOOMP(work_dim, local_work_size);

  for (unsigned d=0; d<mNbDevices; d++) {
    clFinish(mDevicesInfo[d].queue);
  }

  // Compute nb splits and split indexes;
  unsigned *devSplitIdx, *splitDevIdx;
  unsigned nbSplits;
  computeDevSplitIdx(&devSplitIdx, &splitDevIdx, &nbSplits, mDevicesInfo,
		     mNbDevices);

  computeMergeArguments();

  std::cerr << "nbSplits : " << nbSplits << "\n";
  std::cerr << "nbWritten : " << mNbWrittenArgs << "\n";
  std::cerr << "nbMerged : " << mNbMergeArgs << "\n";

  // Get merge kernel
  cl_kernel mergeKernel = mProgram->getMergeKernel(mergeDeviceId, nbSplits,
						   mNbMergeArgs);
  // Get mergeContext
  cl_context mergeCtxt = mContext->getContext(mergeDeviceId);

  // Create one merge buffer per merge argument
  // and one buffer per split for each merge argument
  cl_mem *mergeBuffers = new cl_mem[mNbMergeArgs];
  cl_mem *devBuffers = new cl_mem[mNbMergeArgs * nbSplits];
  int maxLen;

  createMergeBuffersAndSetArgs(nbSplits, splitDevIdx, mergeKernel, mergeCtxt,
			       mergeBuffers, devBuffers, &maxLen);
  double merge_start = mytime();
  prepareMergeBuffers(nbSplits, splitDevIdx, mergeCtxt, devBuffers,
		      mergeBuffers);
  double merge_end = mytime();
  std::cerr << "prepare buffer time : " << (merge_end - merge_start) * 1e3
	    << " ms\n";

  // Compute NDRange
  cl_uint workDim=1;
  size_t globalSize = (size_t) maxLen;
  size_t localSize = (size_t) 32;
  size_t rem = globalSize % localSize;
  if (rem != 0)
    globalSize += 32-rem;

  // Enqueue merge kernel
  cl_int err;
  cl_event merge_event;
  double merge_time;
  cl_ulong start, end;
  err = real_clEnqueueNDRangeKernel(mDevicesInfo[mergeDeviceId].queue,
				    mergeKernel,
				    workDim,
				    NULL,
				    &globalSize,
				    &localSize,
				    0, NULL, &merge_event);
  clCheck(err, __FILE__, __LINE__);
  err = clFinish(mDevicesInfo[mergeDeviceId].queue);
  clCheck(err, __FILE__, __LINE__);
  err = clGetEventProfilingInfo(merge_event,
				CL_PROFILING_COMMAND_START,
				sizeof(start),
				&start,
				NULL);
  clCheck(err, __FILE__, __LINE__);
  err = clGetEventProfilingInfo(merge_event,
				CL_PROFILING_COMMAND_END,
				sizeof(end),
				&end,
				NULL);
  clCheck(err, __FILE__, __LINE__);

  merge_time = ((double) end - start) * 1e-6;
  std::cerr << "merge time : " << merge_time << " ms\n";

  // Read merge buffers
  merge_start = mytime();
  readMergeBuffers(mDevicesInfo, mergeBuffers);
  merge_end = mytime();
  std::cerr << "read buffer time : " << (merge_end - merge_start) * 1e3
	    << " ms\n";

  // Release merge buffers
  for (unsigned i=0; i<mNbMergeArgs; i++) {
    err = real_clReleaseMemObject(mergeBuffers[i]);
    clCheck(err, __FILE__, __LINE__);
  }

  // Release buffers
  for (unsigned s=0; s<nbSplits; s++) {
    unsigned curDev = splitDevIdx[s];
    cl_context curDevCtxt = mContext->getContext(curDev);
    if (curDevCtxt == mergeCtxt)
      continue;

    for (unsigned a = 0; a<mNbMergeArgs; a++) {
      cl_int err = real_clReleaseMemObject(devBuffers[s*mNbMergeArgs + a]);
      clCheck(err, __FILE__, __LINE__);
    }
  }

  // valid local buffer and invalid devices buffers
  bufferMgr->readDevices(mDevicesInfo,
			 mNbDevices,
			 mNbGlobalArgs,
			 mGlobalArgsPos,
			 mArgIsMerge,
			 mMemArgs);

  for (unsigned a=0; a<mNbGlobalArgs; a++)
    mArgIsMerge[a] = false;

  delete[] mergeBuffers;
  delete[] devBuffers;
  free(devSplitIdx);
  free(splitDevIdx);
}

bool
KernelHandle::release() {
  if (--ref_count > 0)
    return false;

  for (unsigned i=0; i<mNbKernels; i++) {
    cl_int err = real_clReleaseKernel(mKernels[i]);
    clCheck(err, __FILE__, __LINE__);
  }

  DEBUG("kernelstats",
	std::cerr << "\x1b[33m"
	<< "release kernel " << mName << " nb iterations = " << mIteration
	<< " can split = " << mCanSplit << "\x1b[0m\n";
	);

  return true;
}

void
KernelHandle::getKernelWorkgroupInfo(cl_device_id device,
				     cl_kernel_work_group_info param_name,
				     size_t param_value_size, void *param_value,
				     size_t *param_value_size_ret) {
  (void) device;
  cl_int err;

  err =  real_clGetKernelWorkGroupInfo(mKernels[0],
				       mContext->getDevice(0),
				       param_name,
				       param_value_size, param_value,
				       param_value_size_ret);
  clCheck(err, __FILE__, __LINE__);
}

void
KernelHandle::getKernelInfo(cl_kernel_info param_name,
			    size_t param_value_size,
			    void *param_value,
			    size_t *param_value_size_ret) {
  cl_int err;

  err =  real_clGetKernelInfo(mKernels[0], param_name, param_value_size,
			      param_value, param_value_size_ret);
  clCheck(err, __FILE__, __LINE__);
}

void
KernelHandle::launchAnalysis() {
  // Launch KernelAnalysis Pass over the SPIR binary
  char opt_command[1024];
  sprintf(opt_command, "%s -load %s %s -kernelname=%s .spir%d.bc > /dev/null",
	  OPTPATH, KERNELANALYSIS_SO_PATH, PASSARG, mName, mProgram->getId());

  system(opt_command);

  // Get results from analysis using mmap
  int fd;
  char *data;

  if ((fd = open("analysis.txt", O_RDONLY)) == -1) {
    perror("libhook open");
    exit(EXIT_FAILURE);
  }

  size_t len = 64 * getpagesize();
  data = (char *) mmap((caddr_t) 0, len, PROT_READ, MAP_SHARED, fd, 0);
  if (data == (char *) (caddr_t) -1) {
    perror("libhook mmap");
    exit(EXIT_FAILURE);
  }

  close(fd);

  std::string str(data, len);
  std::stringstream ss;
  ss << str;

  mAnalysis = KernelAnalysis::open(ss);

  mNbGlobalArgs = mAnalysis->getNbArguments();
  for (unsigned i=0; i<mNbGlobalArgs; ++i)
    mArgIsGlobal[mAnalysis->getArgAnalysis(i)->getPos()] = true;

  // Store global args position, compute number of written args and store
  // their positions.
  mGlobalArgsPos = new unsigned[mNbGlobalArgs];
  for (unsigned a=0; a<mNbGlobalArgs; a++) {
    ArgumentAnalysis *argAnalysis = mAnalysis->getArgAnalysis(a);
    mGlobalArgsPos[a] = argAnalysis->getPos();
    if (argAnalysis->isWritten()) {
      mWrittenArgsPos[mNbWrittenArgs] = argAnalysis->getPos();
      mNbWrittenArgs++;
    }
  }

  if (mNbWrittenArgs > MAXMERGEARGS) {
    std::cerr << "Error: " << mNbWrittenArgs << " written arguments "
	      << "and MAXMERGEARGS is " << MAXMERGEARGS << "\n";
    exit(EXIT_FAILURE);
  }

  for (unsigned i=0; i<mNbDevices; i++) {
    mDevicesInfo[i].dataRequired = new ListInterval[mNbGlobalArgs];
    mDevicesInfo[i].dataWritten = new ListInterval[mNbGlobalArgs];
    mDevicesInfo[i].needToRead = new ListInterval[mNbGlobalArgs];
    mDevicesInfo[i].needToWrite = new ListInterval *[mNbGlobalArgs];
  }
}

void
KernelHandle::getEnvOptions() {
  // Get single device ID
  mSingleDeviceID = optSingleDeviceID;

  // Get DONTSPLIT option
  mDontSplit = optDontSplit;

  // Get MERGEDEV option
  mergeDeviceId = optMergeDeviceID;

  // Get NBSKIPITER option
  mNbSkipIter = optNbSkipIter;

  // Get scheduler option
  switch(optScheduler) {
  case Scheduler::AKGR:
    sched = new SchedulerAKGR(mNbDevices, mDevicesInfo, mNumArgs, mNbGlobalArgs,
			      mAnalysis);
    break;
  case Scheduler::MKGR:
    sched = new SchedulerMKGR(mNbDevices, mDevicesInfo, mNumArgs, mNbGlobalArgs,
			      mAnalysis);
    break;
  case Scheduler::FIXPOINT:
    sched = new SchedulerFixPoint(mNbDevices, mDevicesInfo, mNumArgs,
				  mNbGlobalArgs, mAnalysis);
    break;
  case Scheduler::BROYDEN:
    sched = new SchedulerBroyden(mNbDevices, mDevicesInfo, mNumArgs,
				 mNbGlobalArgs, mAnalysis);
    break;
  case Scheduler::BADBROYDEN:
    sched = new SchedulerBadBroyden(mNbDevices, mDevicesInfo, mNumArgs,
				    mNbGlobalArgs, mAnalysis);
    break;
  default:
    sched = new SchedulerEnv(mNbDevices, mDevicesInfo, mNumArgs, mNbGlobalArgs,
			     mAnalysis, id);
  };
}

cl_kernel
KernelHandle::getMergeKernel(unsigned dev, unsigned nbsplits) {
  return mProgram->getMergeKernel(dev, nbsplits, mNbWrittenArgs);
}
