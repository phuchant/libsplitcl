#include "Utils/Timeline.h"

#include <fstream>
#include <limits>
#include <sstream>
#include <iomanip>

using namespace std;

namespace libsplit {

  static string
  longToHexString(cl_ulong intValue) {
    string hexStr;

    /// integer value to hex-string
    std::stringstream sstream;
    sstream
            << std::setfill ('0') << std::setw(2)
	    << std::uppercase << std::hex << intValue;

    hexStr= sstream.str();
    sstream.clear();    //clears out the stream-string

    return hexStr;
  }


  Timeline::TimelineEvent::TimelineEvent(Event *event, std::string method)
    : event(event), method(method) {}

  Timeline::TimelineEvent::~TimelineEvent() {}

  void
  Timeline::pushEvent(Event *event, std::string &method, int devId) {
    devices.insert(devId);
    timelineEvents[devId].push_back(TimelineEvent(event, method));
  }

  void
  Timeline:: pushH2DEvent(Event *event, int devId) {
    devices.insert(devId);
    timelineEvents[devId].push_back(TimelineEvent(event, "memcpyHtoDasync"));
  }

  void
  Timeline::pushD2HEvent(Event *event, int devId) {
    devices.insert(devId);
    timelineEvents[devId].push_back(TimelineEvent(event, "memcpyDtoHasync"));
  }

  void
  Timeline::pushPartition(double *partition) {
    partitions.insert(partitions.end(), &partition[0], &partition[nbDevices+1]);
  }

  void
  Timeline::writeTrace(std::string &filename) const {
    ofstream outfile;
    outfile.open(filename);

    // Header
    outfile << "# CUDA_PROFILE_LOG_VERSION 2.0\n"
	    << "# CUDA_DEVICE 0 Tesla M2075\n"
	    << "# CUDA_CONTEXT 1\n"
	    << "# CUDA_PROFILE_CSV 1\n";
    outfile << "gpustarttimestamp,gpuendtimestamp,method,gputime,streamid\n";


    cl_ulong timestart, timeend;
    for (auto IT : timelineEvents) {
      int devId = IT.first;

      cl_int err;
      err = real_clGetEventProfilingInfo(IT.second[0].event->event,
					 CL_PROFILING_COMMAND_START,
					 sizeof(timestart),
					 &timestart, NULL);
      clCheck(err, __FILE__, __LINE__);

      cl_ulong offset = timestart-1;
      cl_ulong prevStart = timestart;

      for (const TimelineEvent &e : IT.second) {
	err = real_clGetEventProfilingInfo(e.event->event,
					   CL_PROFILING_COMMAND_START,
					   sizeof(timestart),
					   &timestart, NULL);
	clCheck(err, __FILE__, __LINE__);
	err = real_clGetEventProfilingInfo(e.event->event,
					   CL_PROFILING_COMMAND_END,
					   sizeof(timeend),
					      &timeend, NULL);
	clCheck(err, __FILE__, __LINE__);

	if (timestart < prevStart) {
      	  offset -= CL_ULONG_MAX;
      	}
      	prevStart = timestart;

	outfile << longToHexString(timestart - offset) << ","
		<< longToHexString(timeend - offset) << ","
		<< e.method << ","
		<< (timeend - timestart) * 1.0e-3 << ","
		<< devId << "\n";
      }
    }


    outfile.close();
  }

  void
  Timeline::writePartitions(std::string &filename) const {
    ofstream outfile;
    outfile.open(filename);

    int rowSize = nbDevices+1;
    int offset = 0;
    while (offset + nbDevices < partitions.size()) {
      for (unsigned i=0; i<nbDevices+1; i++)
	outfile << partitions[offset+i] << " ";
      outfile << "\n";

      offset += rowSize;
    }

    outfile.close();
  }

};
