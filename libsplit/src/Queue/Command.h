#ifndef COMMAND_H
#define COMMAND_H

#include <Handle/KernelHandle.h>
#include <Queue/DeviceQueue.h>
#include <Queue/Event.h>

#include <CL/cl.h>

namespace libsplit {

  class Command {
  public:
    Command(cl_bool blocking, unsigned wait_list_size, const Event *wait_list);
    virtual ~Command();

    Event getEvent();
    virtual void execute(DeviceQueue *queue) = 0;

    cl_bool blocking;
    unsigned id;

  protected:
    Event event;
    unsigned wait_list_size;
    Event *wait_list;

  private:
    static unsigned count;
  };

  class CommandWrite : public Command {
  public:
    CommandWrite(cl_mem buffer,
		 cl_bool blocking,
		 size_t offset,
		 size_t cb,
		 const void *ptr,
		 unsigned wait_list_size,
		 const Event *wait_list);

    virtual ~CommandWrite();

    virtual void execute(DeviceQueue *queue);

    cl_mem buffer;
    size_t offset;
    size_t cb;
    const void *ptr;
  };

  class CommandRead : public Command {
  public:
    CommandRead(cl_mem buffer,
		cl_bool blocking,
		size_t offset,
		size_t cb,
		const void *ptr,
		unsigned wait_list_size,
		const Event *wait_list);

    virtual ~CommandRead();

    virtual void execute(DeviceQueue *queue);

    cl_mem buffer;
    size_t offset;
    size_t cb;
    const void *ptr;
  };

  class CommandExec : public Command {
  public:
    CommandExec(cl_kernel kernel,
		cl_uint work_dim,
		const size_t *global_work_offset,
		const size_t *global_work_size,
		const size_t *local_work_size,
		KernelArgs &args,
		unsigned wait_list_size,
		const Event *wait_list);

    virtual ~CommandExec();

    virtual void execute(DeviceQueue *queue);

    void setArg(cl_uint index, KernelArg *value);

    cl_kernel kernel;
    cl_uint work_dim;
    size_t *global_work_offset;
    size_t *global_work_size;
    size_t *local_work_size;
    KernelArgs args;
  };

  class CommandFill : public Command {
  public:
    CommandFill(cl_mem buffer,
		const void *pattern,
		size_t pattern_size,
		size_t offset,
		size_t size,
		unsigned wait_list_size,
		const Event *wait_list);

    virtual ~CommandFill();

    virtual void execute(DeviceQueue *queue);

    cl_mem buffer;
    const void *pattern;
    size_t pattern_size;
    size_t offset;
    size_t size;
  };

};

#endif /* COMMAND_H */
