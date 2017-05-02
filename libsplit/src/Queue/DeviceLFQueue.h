#ifndef DEVICELFQUEUE_H
#define DEVICELFQUEUE_H

#include <Handle/KernelHandle.h>
#include <Queue/DeviceQueue.h>
#include <Queue/Event.h>
#include <Queue/LockFreeQueue.h>

#include <CL/cl.h>

#include <pthread.h>

class Command;

namespace libsplit {

  class DeviceLFQueue : public DeviceQueue {
  public:
    DeviceLFQueue(cl_context context, cl_device_id dev, unsigned dev_id);
    virtual ~DeviceLFQueue();

    virtual void run();

  private:
    virtual void enqueue(Command *command, Event *event);

    LockFreeQueue *threadQueue;

    pthread_t thread;
    bool running;
  };

};

#endif /* DEVICELFQUEUE_H */
