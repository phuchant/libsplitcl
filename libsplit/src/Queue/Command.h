#ifndef COMMAND_H
#define COMMAND_H

#include <Handle/KernelHandle.h>
#include <Queue/DeviceQueue.h>
#include <Queue/Event.h>

#include <CL/cl.h>

namespace libsplit {

  class Command {
  public:
    Command(Event *event);
    virtual ~Command();
    virtual void execute(DeviceQueue *queue) = 0;
    unsigned id;

    Event *event;

  private:
    static unsigned count;
  };

  class CommandWrite : public Command {
  public:
    CommandWrite(cl_mem buffer,
		 size_t offset,
		 size_t cb,
		 const void *ptr,
		 Event *event);

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
		size_t offset,
		size_t cb,
		const void *ptr,
		Event *event);

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
		Event *event);

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
		Event *event);

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
