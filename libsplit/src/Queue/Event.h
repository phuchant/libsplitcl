#ifndef EVENT_H
#define EVENT_H

#include <Utils/Retainable.h>
#include <Utils/Utils.h>

#include <CL/cl.h>

namespace libsplit {

  class Event_ : public Retainable {
  public:
    Event_() : submitted(false) {
      pthread_mutex_init(&mutex_submitted, NULL);
      pthread_cond_init(&cond_submitted, NULL);
    }

    ~Event_() {
      pthread_mutex_destroy(&mutex_submitted);
      pthread_cond_destroy(&cond_submitted);
    }

    void
    setSubmitted() {
      pthread_mutex_lock(&mutex_submitted);
      submitted = true;
      pthread_cond_broadcast(&cond_submitted);
      pthread_mutex_unlock(&mutex_submitted);
    }

    void
    wait() {
      pthread_mutex_lock(&mutex_submitted);
      if (!submitted)
	pthread_cond_wait(&cond_submitted, &mutex_submitted);


      cl_int status;

      // Hack
      do {
	cl_int err = real_clWaitForEvents(1, &event);
	clCheck(err, __FILE__, __LINE__);

	err = real_clGetEventInfo(event,
				  CL_EVENT_COMMAND_EXECUTION_STATUS,
				  sizeof(status),
				  &status, NULL);
	clCheck(err, __FILE__, __LINE__);
      } while (status != CL_COMPLETE);

      pthread_mutex_unlock(&mutex_submitted);
    }

    cl_event event;

  private:
    bool submitted;
    pthread_mutex_t mutex_submitted;
    pthread_cond_t cond_submitted;

  };

  typedef Event_ * Event;

};

#endif /* EVENT_H */
