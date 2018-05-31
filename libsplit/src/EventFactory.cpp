#include <EventFactory.h>
#include <Dispatch/OpenCLFunctions.h>
#include <Handle/MemoryHandle.h>

#include <cassert>

#define MAXEVENTS 1000000

namespace libsplit {
  EventFactory::EventFactory() {
    events = new Event[MAXEVENTS];
    nextEventId = 0;
  }

  EventFactory::~EventFactory() {
    delete events;
  }

  Event *
  EventFactory::getNewEvent() {
    assert(nextEventId < MAXEVENTS);
    return &events[nextEventId++];
  }

  void
  EventFactory::setMemoryHandleDep(MemoryHandle *m) {
    memoryDeps.insert(m);
  }

  void
  EventFactory::freeEvents() {
    // for (MemoryHandle *m : memoryDeps) {
    //   for (unsigned d=0; d<m->mNbBuffers; d++) {
    // 	m->lastDeviceWritterEvent[d] = nullptr;
    //   }
    // }
    // for (unsigned i=0; i<nextEventId; i++)
    //   cl_int err = real_clReleaseEvent(events[i]);
    // nextEventId = 0;
  }
};
