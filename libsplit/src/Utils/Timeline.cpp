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


  Timeline::TimelineEvent::TimelineEvent(cl_ulong timestart, cl_ulong timeend,
					 std::string method)
    : timestart(timestart), timeend(timeend), method(method) {}

  Timeline::TimelineEvent::~TimelineEvent() {}

  void
  Timeline::pushKernelEvent(cl_ulong timestart, cl_ulong timeend, std::string &method, int devId) {
    devices.insert(devId);
    timelineEvents[devId].push_back(TimelineEvent(timestart, timeend, method));
  }

  void
  Timeline:: pushH2DEvent(cl_ulong timestart, cl_ulong timeend, int devId) {
    devices.insert(devId);
    timelineEvents[devId].push_back(TimelineEvent(timestart, timeend, "memcpyHtoDasync"));
  }

  void
  Timeline::pushD2HEvent(cl_ulong timestart, cl_ulong timeend, int devId) {
    devices.insert(devId);
    timelineEvents[devId].push_back(TimelineEvent(timestart, timeend, "memcpyDtoHasync"));
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


    for (auto IT : timelineEvents) {
      int devId = IT.first;

      cl_ulong offset = IT.second[0].timestart-1;

      cl_ulong prevStart = IT.second[0].timestart;

      for (const TimelineEvent &e : IT.second) {
      	if (e.timestart < prevStart) {
      	  offset -= CL_ULONG_MAX;
      	}
      	prevStart = e.timestart;

	outfile << longToHexString(e.timestart - offset) << ","
		<< longToHexString(e.timeend - offset) << ","
		<< e.method << ","
		<< (e.timeend - e.timestart) * 1.0e-3 << ","
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
