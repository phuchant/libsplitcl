#include <Queue/Command.h>
#include <Queue/DeviceQueue.h>
#include <Utils/Utils.h>

#include <cstring>

namespace libsplit {

  unsigned
  Command::count = 0;

  Command::Command(Event *event)
    : event(event) {

    do {
      id = count;
    } while (!__sync_bool_compare_and_swap(&count, id, id + 1));
  }

  Command::~Command() {}


  CommandWrite::CommandWrite(cl_mem buffer,
			     size_t offset,
			     size_t cb,
			     const void *ptr,
			     Event *event) :
    Command(event),
    buffer(buffer), offset(offset), cb(cb), ptr(ptr) {}

  CommandWrite::~CommandWrite() {}

  void
  CommandWrite::execute(DeviceQueue *queue) {
    cl_int err;

    err = real_clEnqueueWriteBuffer(queue->cl_queue,
				    buffer,
				    CL_FALSE /* non blocking */,
				    offset,
				    cb,
				    ptr,
				    0,
				    NULL,
				    &event->event);

    clCheck(err, __FILE__, __LINE__);

    event->setSubmitted();
  }

  CommandRead::CommandRead(cl_mem buffer,
			   size_t offset,
			   size_t cb,
			   const void *ptr,
			   Event *event) :
    Command(event),
    buffer(buffer),  offset(offset), cb(cb), ptr(ptr) {}

  CommandRead::~CommandRead() {}

  void
  CommandRead::execute(DeviceQueue *queue) {
    cl_int err;

    err = real_clEnqueueReadBuffer(queue->cl_queue,
				   buffer,
				   CL_FALSE /* non blocking */,
				   offset,
				   cb,
				   (void *) ptr,
				   0,
				   NULL,
				   &event->event);
    clCheck(err, __FILE__, __LINE__);

    event->setSubmitted();
  }

  CommandExec::CommandExec(cl_kernel kernel,
			   cl_uint work_dim,
			   const size_t *global_work_offset,
			   const size_t *global_work_size,
			   const size_t *local_work_size,
			   KernelArgs &args,
			   Event *event) :
    Command(event),
    kernel(kernel), work_dim(work_dim), global_work_offset(NULL),
    global_work_size(NULL), local_work_size(NULL), args(args) {
    this->global_work_size = new size_t[work_dim];
    this->local_work_size = new size_t[work_dim];
    memcpy(this->global_work_size, global_work_size,
	   work_dim * sizeof(size_t));
    memcpy(this->local_work_size, local_work_size,
	   work_dim * sizeof(size_t));

    if (global_work_offset) {
      this->global_work_offset = new size_t[work_dim];
      memcpy(this->global_work_offset, global_work_offset,
	     work_dim * sizeof(size_t));
    }
  }

  CommandExec::~CommandExec() {
    delete[] global_work_offset;
    delete[] global_work_size;
    delete[] local_work_size;
  }

  void
  CommandExec::execute(DeviceQueue *queue) {
    cl_int err;

    for (KernelArgs::iterator it=args.begin();
	 it != args.end(); ++it) {
      KernelArg value = it->second;
      err = real_clSetKernelArg(kernel,
				it->first,
				value.size,
				value.local ? NULL : value.value);
      clCheck(err, __FILE__, __LINE__);
    }

    err = real_clEnqueueNDRangeKernel(queue->cl_queue,
				      kernel,
				      work_dim,
				      global_work_offset,
				      global_work_size,
				      local_work_size,
				      0,
				      NULL,
				      &event->event);

    clCheck(err, __FILE__, __LINE__);

    event->setSubmitted();
  }

  CommandFill::CommandFill(cl_mem buffer,
			   const void *pattern,
			   size_t pattern_size,
			   size_t offset,
			   size_t size,
			   Event *event)
    : Command(event),
      buffer(buffer), pattern(pattern), pattern_size(pattern_size),
      offset(offset), size(size) {}

  CommandFill::~CommandFill() {}

  void
  CommandFill::execute(DeviceQueue *queue) {
    cl_int err;

    err = real_clFinish(queue->cl_queue);
    clCheck(err, __FILE__, __LINE__);

    err = real_clEnqueueFillBuffer(queue->cl_queue,
				   buffer,
				   pattern,
				   pattern_size,
				   offset,
				   size,
				   0,
				   NULL,
				   &event->event);
    clCheck(err, __FILE__, __LINE__);

    event->setSubmitted();
  }

};
