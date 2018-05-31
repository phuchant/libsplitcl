#ifndef EVENTFACTORY_H
#define EVENTFACTORY_H

#include <Queue/Event.h>

#include <set>

#include <CL/cl.h>


namespace libsplit {

  class MemoryHandle;

  class EventFactory {
  public:
    EventFactory();
    ~EventFactory();

    Event *getNewEvent();
    void freeEvents();
    void setMemoryHandleDep(MemoryHandle *m);

  private:
    unsigned nextEventId;
    Event *events;

    std::set<MemoryHandle *> memoryDeps;
  };

};
#endif /* EVENTFACTORY_H */
