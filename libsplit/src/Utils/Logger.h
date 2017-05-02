#ifndef LOGGER_H
#define LOGGER_H

#include <ListInterval.h>

#include <map>

namespace libsplit {

  class LogIter;

  class Logger {
  public:
    Logger(unsigned nbDevices);
    ~Logger();

    void logHtoD(unsigned devId, unsigned bufferId, const Interval &I);
    void logDtoH(unsigned devId, unsigned bufferId, const Interval &I);
    void logKernel(unsigned devId, unsigned kerId, double gr);
    void step(unsigned nbDevices);
    void toDot(const char *filename);
    void debug();

    unsigned nbDevices;
    unsigned iterno;
    std::vector<LogIter *> trace;
  };

}

#endif /* LOGGER_H */
