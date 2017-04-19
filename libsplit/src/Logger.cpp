#include "Logger.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <cassert>

class LogIter {
public:
  LogIter(unsigned iterno, unsigned nbDevices);
  ~LogIter();

  void  logHtoD(unsigned devId, unsigned bufferId, const Interval &I);
  void  logDtoH(unsigned devId, unsigned bufferId, const Interval &I);
  void  logKernel(unsigned devId, unsigned kerId, double gr);

  unsigned iterno;
  unsigned kerId;
  unsigned nbDevices;

  double *gr; // one per device
  std::map<unsigned, ListInterval>  *hToDEdges; // one per device
  std::map<unsigned, ListInterval>  *dToHEdges; // one per device
};

Logger::Logger(unsigned nbDevices) : nbDevices(nbDevices), iterno(0) {}

Logger::~Logger() {
  for (unsigned i=0; i<trace.size(); i++)
    delete trace[i];
}

void
Logger::debug() {
  for (unsigned i=0; i<trace.size(); i++) {
    LogIter *log = trace[i];
    std::cout << "********  " << log->iterno << "  ********\n\n";

    for (unsigned d=0; d<nbDevices; d++) {
      for (std::map<unsigned, ListInterval>::iterator
	     I=log->dToHEdges[d].begin(),
	     E = log->dToHEdges[d].end(); I != E; ++I) {
	std::cout << "Buffer " << (char) ('A' + (*I).first)
		  << " D" << d << " to H ";
	(*I).second.debug();
	std::cout << "\n";
      }
    }

    std::cerr << "\n";

    for (unsigned d=0; d<nbDevices; d++) {
      for (std::map<unsigned, ListInterval>::iterator
	     I=log->hToDEdges[d].begin(),
	     E = log->hToDEdges[d].end(); I != E; ++I) {
	std::cout << "Buffer " << (char) ('A' + (*I).first)
		  << " H to D" << d << " ";
	(*I).second.debug();
	std::cout << "\n";
      }
    }

    std::cout << "\n";

    for (unsigned i=0; i<nbDevices; i++)
      std::cout << "<ker_" << i << "-" << log->kerId << " " << log->gr[i]
		<< "> ";
    std::cout << "\n\n";
  }
}

void
Logger::logHtoD(unsigned devId, unsigned bufferId, const Interval &I) {
  assert(!trace.empty());
  trace[trace.size()-1]->logHtoD(devId, bufferId, I);
}

void
Logger::logDtoH(unsigned devId, unsigned bufferId, const Interval &I) {
  assert(!trace.empty());
  trace[trace.size()-1]->logDtoH(devId, bufferId, I);
}

void
Logger::logKernel(unsigned devId, unsigned kerId, double gr) {
  assert(!trace.empty());
  if (kerId > 1000)
    abort();

  trace[trace.size()-1]->logKernel(devId, kerId, gr);
}

void
Logger::step(unsigned nbDevices) {
  iterno++;
  trace.push_back(new LogIter(iterno, nbDevices));
}

void Logger::toDot(const char *filename) {
  std::stringstream stream;

  // Fonts
  stream << "digraph G {\n";
  stream << "graph [fontname=\"arial\"];\n";
  stream << "node [fontname=\"arial\"];\n";
  stream << "edge [fontname=\"arial\"];\n";

  // Nodes
  stream << "\n/* nodes */\n\n";
  for (unsigned i=1; i<=iterno; i++) {
    if (trace[i-1]->kerId > 1000)
      abort();
    stream << "{rank = same; i"<<i<<" [label=\"enqueueNDRange(k"
	   << trace[i-1]->kerId << ")\"] ";
    stream << "h" << i << " [label=\"Host\"] }\n\n";

    stream << "{rank = same;\n";
    for (unsigned d=0; d<nbDevices; d++) {
      stream << "d" << i << d << " [label=\"Device " << d
	     << "\\nk" << trace[i-1]->kerId << " " << trace[i-1]->gr[d]
	     << "\"]\n";
    }
    stream << "}\n\n";
  }

  // Edges
  stream << "\n/* edges */\n\n";

  for (unsigned i=1; i<iterno; i++) {
    // iter to iter arrow
    stream << "i"<<i<<" -> i"<<i+1<<"\n";

    // dev to host arrows
    for (unsigned d=0; d<nbDevices; d++)
      stream << "d"<<i<<d<<" -> h"<<i+1<<" [color=white]\n";
  }

  for (unsigned i=1; i<=iterno; i++) {
    // host to dev arrows
    for (unsigned d=0; d<nbDevices; d++)
      stream << "h"<<i<<" -> d"<<i<<d<<" [color=white]\n";
  }

  for (unsigned i=1; i<=iterno; i++) {
    LogIter *log = trace[i-1];

    // H to D edges
    for (unsigned d=0; d<nbDevices; d++) {
      for (std::map<unsigned, ListInterval>::iterator
	     I=log->hToDEdges[d].begin(),
	     E = log->hToDEdges[d].end(); I != E; ++I) {
	stream << "h"<<i<<" -> d"<<i<<d<<" [label=\""
	       << (char) ('A' + (*I).first)
	       << "\\n" << (*I).second.toString()
	       << "\"]\n";
      }
    }

    // D to H edges
    for (unsigned d=0; d<nbDevices; d++) {
      for (std::map<unsigned, ListInterval>::iterator
	     I=log->dToHEdges[d].begin(),
	     E = log->dToHEdges[d].end(); I != E; ++I) {
	stream << "d"<<i-1<<d<<" -> h"<<i<<" [label=\""
	       << (char) ('A' + (*I).first)
	       << "\\n" << (*I).second.toString()
	       << "\"]\n";
      }
    }
  }

  stream << "}\n";

  std::ofstream out;
  out.open(filename);
  out << stream.str();
  out.close();
}

LogIter::LogIter(unsigned iterno, unsigned nbDevices)
  : iterno(iterno), nbDevices(nbDevices) {
  gr = new double[nbDevices];
  for (unsigned i=0; i<nbDevices; i++)
    gr[i] = 0;
  hToDEdges = new std::map<unsigned, ListInterval>[nbDevices];
  dToHEdges = new std::map<unsigned, ListInterval>[nbDevices];
}

LogIter::~LogIter() {
  delete[] gr;
  delete[] hToDEdges;
  delete[] dToHEdges;
}

void
LogIter::logHtoD(unsigned devId, unsigned bufferId, const Interval &I) {
  std::map<unsigned, ListInterval>::iterator it;
  it = hToDEdges[devId].find(bufferId);
  if (it == hToDEdges[devId].end())
    hToDEdges[devId][bufferId] = ListInterval();

  hToDEdges[devId][bufferId].add(I);
}

void
LogIter::logDtoH(unsigned devId, unsigned bufferId, const Interval &I) {
  std::map<unsigned, ListInterval>::iterator it;
  it = dToHEdges[devId].find(bufferId);
  if (it == hToDEdges[devId].end())
    dToHEdges[devId][bufferId] = ListInterval();

  dToHEdges[devId][bufferId].add(I);
}

void
LogIter::logKernel(unsigned devId, unsigned kerId, double gr) {
  this->gr[devId] = gr;
  this->kerId = kerId;
}
