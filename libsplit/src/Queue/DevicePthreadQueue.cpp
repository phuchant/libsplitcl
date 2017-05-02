#include <Queue/DevicePthreadQueue.h>
#include <Queue/Command.h>
#include <Utils/Utils.h>

/* locking macros */
#define PTHREAD_LOCK(__lock , __counter)        \
  do {                                          \
    if ((__counter))                            \
      ++(*((unsigned*)(__counter)));            \
  } while (pthread_mutex_trylock((__lock)))

//#define PTHREAD_LOCK(__lock, __counter) do {pthread_mutex_lock((__lock__));} while (0)

#define PTHREAD_UNLOCK(__lock) do { pthread_mutex_unlock((__lock)); } while(0)

namespace libsplit {

  DevicePthreadQueue::DevicePthreadQueue(cl_context context, cl_device_id dev,
					 unsigned dev_id)
    : DeviceQueue(context, dev, dev_id), running(true) {
    int ret;

    // Lock and cond
    pthread_mutex_init(&queueLock, NULL);
    pthread_cond_init(&wakeupCond, NULL);

    ret = pthread_create(&thread, NULL, &DevicePthreadQueue::threadFunc, this);
    if (ret != 0) {
      std::cerr << "error: Failed to create DeviceQueue thread ("<<ret<<")\n";
      errno = ret;
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  DevicePthreadQueue::~DevicePthreadQueue() {
    running = false;
    pthread_cond_broadcast(&wakeupCond);
    finish();
    pthread_join(thread, NULL);
  }


  void
  DevicePthreadQueue::enqueue(Command *command, Event *event) {
    bool blocking = command->blocking;

    if (event)
      *event = command->getEvent();
    if (lastEvent)
      lastEvent->release();
    lastEvent = command->getEvent();

    PTHREAD_LOCK(&queueLock, NULL);
    threadQueue.push_back(command);
    pthread_cond_broadcast(&wakeupCond);
    PTHREAD_UNLOCK(&queueLock);

    if (blocking)
      lastEvent->wait();
  }

  void
  DevicePthreadQueue::run() {
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
      PTHREAD_LOCK(&queueLock, NULL);
      if (!threadQueue.empty()) {
	cmd = threadQueue.front();
	threadQueue.pop_front();
      }
      PTHREAD_UNLOCK(&queueLock);

      if (cmd) {
	cmd->execute(this);
	delete cmd;
      } else {
	static struct timespec time_to_wait = {0, 0};
	time_to_wait.tv_sec = time(NULL) + 5;

	PTHREAD_LOCK (&queueLock, NULL);
	if (threadQueue.empty())
	  pthread_cond_timedwait (&wakeupCond, &queueLock, &time_to_wait);
	PTHREAD_UNLOCK (&queueLock);
      }
    }

    // Dequeue remaining commands before leaving.
    while (!threadQueue.empty()) {
      Command *cmd = threadQueue.front();
      cmd->execute(this);
      delete cmd;
      threadQueue.pop_front();
    }
  }

};
