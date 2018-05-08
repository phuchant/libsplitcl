#ifndef EVENTFACTORY_H
#define EVENTFACTORY_H

#include <set>

#include <CL/cl.h>

namespace libsplit {

  class MemoryHandle;

  class EventFactory {
  public:
    EventFactory();
    ~EventFactory();

    cl_event *getNewEvent();
    void freeEvents();
    void setMemoryHandleDep(MemoryHandle *m);

  private:
    unsigned nextEventId;
    cl_event *events;

    std::set<MemoryHandle *> memoryDeps;
  };

};
#endif /* EVENTFACTORY_H */
