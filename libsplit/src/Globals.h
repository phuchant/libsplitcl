#ifndef GLOBALS_H
#define GLOBALS_H

#include <Driver.h>
#include <Handle/ContextHandle.h>
#include <Utils/Timeline.h>
#include <EventFactory.h>

namespace libsplit {
  extern Driver *driver;
  extern ContextHandle *contextHandle;
  extern Timeline *timeline;
  extern EventFactory *eventFactory;
};

#endif /* GLOBALS_H */
