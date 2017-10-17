#include <Queue/Command.h>
#include <Queue/DeviceQueue.h>
#include <Utils/Utils.h>

#include <cstring>

namespace libsplit {

  unsigned
  Command::count = 0;

  Command::Command(cl_bool blocking, unsigned wait_list_size,
		   const Event *wait_list)
    : blocking(blocking), wait_list_size(wait_list_size) {

    event = new Event_();

    if (wait_list_size > 0) {
      this->wait_list = new Event[wait_list_size];
      memcpy(this->wait_list, wait_list, wait_list_size * sizeof(Event));
      for (unsigned i=0; i<this->wait_list_size; i++)
	this->wait_list[i]->retain();
    } else {
      this->wait_list = NULL;
    }

    do {
      id = count;
    } while (!__sync_bool_compare_and_swap(&count, id, id + 1));
  }

  Command::~Command() {
    event->release();
    for (unsigned i=0; i<wait_list_size; i++)
      wait_list[i]->release();
    delete[] wait_list;
  }

  Event
  Command::getEvent() {
    event->retain();
    return event;
  }

  CommandWrite::CommandWrite(cl_mem buffer,
			     cl_bool blocking,
			     size_t offset,
			     size_t cb,
			     const void *ptr,
			     unsigned wait_list_size,
			     const Event *wait_list) :
    Command(blocking, wait_list_size, wait_list),
    buffer(buffer), offset(offset), cb(cb), ptr(ptr) {}

  CommandWrite::~CommandWrite() {}

  void
  CommandWrite::execute(DeviceQueue *queue) {
    cl_int err;

    for (unsigned i=0; i<wait_list_size; i++)
      wait_list[i]->wait();

    err = real_clEnqueueWriteBuffer(queue->cl_queue,
				    buffer,
				    CL_TRUE /* non blocking */,
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
			   cl_bool blocking,
			   size_t offset,
			   size_t cb,
			   const void *ptr,
			   unsigned wait_list_size,
			   const Event *wait_list) :
    Command(blocking, wait_list_size, wait_list),
    buffer(buffer),  offset(offset), cb(cb), ptr(ptr) {}

  CommandRead::~CommandRead() {}

  void
  CommandRead::execute(DeviceQueue *queue) {
    cl_int err;

    for (unsigned i=0; i<wait_list_size; i++)
      wait_list[i]->wait();

    err = real_clEnqueueReadBuffer(queue->cl_queue,
				   buffer,
				   CL_TRUE /* non blocking */,
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
			   unsigned wait_list_size,
			   const Event *wait_list) :
    Command(CL_FALSE, wait_list_size, wait_list),
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

    for (unsigned i=0; i<wait_list_size; i++)
      wait_list[i]->wait();

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

    err = clFlush(queue->cl_queue);
    clCheck(err, __FILE__, __LINE__);

    event->setSubmitted();
  }

  CommandFill::CommandFill(cl_mem buffer,
			   const void *pattern,
			   size_t pattern_size,
			   size_t offset,
			   size_t size,
			   unsigned wait_list_size,
			   const Event *wait_list)
    : Command(CL_FALSE, wait_list_size, wait_list),
      buffer(buffer), pattern(pattern), pattern_size(pattern_size),
      offset(offset), size(size) {}

  CommandFill::~CommandFill() {}

  void
  CommandFill::execute(DeviceQueue *queue) {
    cl_int err;

    err = real_clFinish(queue->cl_queue);
    clCheck(err, __FILE__, __LINE__);


    for (unsigned i=0; i<wait_list_size; i++)
      wait_list[i]->wait();

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
