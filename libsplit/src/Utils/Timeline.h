#ifndef TIMELINE_H
#define TIMELINE_H

#include <CL/cl.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace libsplit {

  class Timeline {
  private:
    struct TimelineEvent {
      TimelineEvent(cl_ulong timestart, cl_ulong timeend, std::string method);
      ~TimelineEvent();

      cl_ulong timestart;
      cl_ulong timeend;
      std::string method;
    };

  public:
    Timeline(unsigned nbDevices) : nbDevices(nbDevices) {}
    ~Timeline() {}

    void pushKernelEvent(cl_ulong timestart, cl_ulong timeend, std::string &method, int devId);
    void pushH2DEvent(cl_ulong timestart, cl_ulong timeend, int devId);
    void pushD2HEvent(cl_ulong timestart, cl_ulong timeend, int devId);
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
