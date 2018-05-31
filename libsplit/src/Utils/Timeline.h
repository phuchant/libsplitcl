#ifndef TIMELINE_H
#define TIMELINE_H

#include <Queue/Event.h>

#include <CL/cl.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace libsplit {

  class Timeline {
  private:
    struct TimelineEvent {
      TimelineEvent(Event *event, std::string method);
      ~TimelineEvent();

      Event *event;
      std::string method;
    };

  public:
    Timeline(unsigned nbDevices) : nbDevices(nbDevices) {}
    ~Timeline() {}

    void pushEvent(Event *event, std::string &method, int devId);
    void pushH2DEvent(Event *event, int devId);
    void pushD2HEvent(Event *event, int devId);
    void writeTrace(std::string &filename) const;
    void writePartitions(std::string &filename) const;
    void pushPartition(double *partition);

  private:
    unsigned nbDevices;
    std::set<int> devices;
    std::map<int, std::vector<Timeline::TimelineEvent> > timelineEvents;
    std::vector<double> partitions;
  };

};


#endif /* TIMELINE_H */
