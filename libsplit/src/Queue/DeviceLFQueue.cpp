#include <Queue/Command.h>
#include <Queue/DeviceLFQueue.h>
#include <Utils/Utils.h>

#include <cstdio>
#include <errno.h>
#include <unistd.h>

namespace libsplit {

  DeviceLFQueue::DeviceLFQueue(cl_context context, cl_device_id dev,
			       unsigned dev_id)
    : DeviceQueue(context, dev, dev_id), running(true) {
    int ret;

    threadQueue = new LockFreeQueue(4096);

    ret = pthread_create(&thread, NULL, &DeviceLFQueue::threadFunc, this);
    if (ret != 0) {
      std::cerr << "error: Failed to create DeviceQueue thread ("<<ret<<")\n";
      errno = ret;
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  DeviceLFQueue::~DeviceLFQueue() {
    running = false;
    finish();
    pthread_join(thread, NULL);

    delete threadQueue;
  }


  void
  DeviceLFQueue::enqueue(Command *command) {
    lastEvent = command->event;

    while (!threadQueue->Enqueue(command));
  }

  void
  DeviceLFQueue::run() {
    // Bind thread
    bindThread();

    // Create the command queue.
    cl_int err;
    cl_queue = real_clCreateCommandQueue(context, device,
					 CL_QUEUE_PROFILING_ENABLE,
					 &err);
    clCheck(err, __FILE__, __LINE__);

    // Main loop
    while (running) {
      Command *cmd = NULL;

      // Dequeue one command
      if (threadQueue->Size() > 0) {
	while (!threadQueue->Dequeue(&cmd));
      } else {
	usleep(1000);
	continue;
      }

      // Execute it
      cmd->execute(this);
      delete cmd;
    }

    // Dequeue remaining commands before leaving.
    while (threadQueue->Size() > 0) {
      Command *cmd;
      while (!threadQueue->Dequeue(&cmd));
      cmd->execute(this);
      delete cmd;
    }
  }

};
