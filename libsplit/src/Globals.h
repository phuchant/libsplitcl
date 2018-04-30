#ifndef GLOBALS_H
#define GLOBALS_H

#include <Driver.h>
#include <Handle/ContextHandle.h>
#include <Utils/Timeline.h>

namespace libsplit {
  extern Driver *driver;
  extern ContextHandle *contextHandle;
  extern Timeline *timeline;
};

#endif /* GLOBALS_H */
