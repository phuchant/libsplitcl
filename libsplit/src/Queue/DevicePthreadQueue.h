#ifndef DEVICEPTHREADQUEUE_H
#define DEVICEPTHREADQUEUE_H

#include <Handle/KernelHandle.h>
#include <Queue/DeviceQueue.h>
#include <Queue/Event.h>
#include <Queue/LockFreeQueue.h>

#include <CL/cl.h>

#include <list>

#include <pthread.h>

class Command;

namespace libsplit {

  class DevicePthreadQueue : public DeviceQueue {
  public:
    DevicePthreadQueue(cl_context context, cl_device_id dev, unsigned dev_id);
    virtual ~DevicePthreadQueue();

    virtual void run();

  private:
    virtual void enqueue(Command *command);

    std::list<Command *> threadQueue;

    pthread_t thread;
    pthread_mutex_t queueLock;
    pthread_cond_t wakeupCond;
    bool running;
  };

};

#endif /* DEVICEPTHREADQUEUE_H */
