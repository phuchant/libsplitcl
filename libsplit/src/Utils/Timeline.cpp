#include <Dispatch/OpenCLFunctions.h>
#include "Utils/Timeline.h"
#include "Utils/Utils.h"

#include <fstream>
#include <limits>
#include <sstream>
#include <iomanip>

extern "C" {
#include <gsl/gsl_fit.h>
}

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


  Timeline::TimelineEvent::TimelineEvent(Event *event,
					 std::string method)
    : event(event), method(method) {}

  Timeline::TimelineEvent::~TimelineEvent() {}

  void
  Timeline::pushEvent(Event *event, std::string &method, int queueId) {
    devices.insert(queueId);
    timelineEvents[queueId].push_back(TimelineEvent(event, method));
  }

  void
  Timeline:: pushH2DEvent(Event *event, int queueId) {
    devices.insert(queueId);
    timelineEvents[queueId].push_back(TimelineEvent(event, "memcpyHtoDasync"));
  }

  void
  Timeline::pushD2HEvent(Event *event, int queueId) {
    devices.insert(queueId);
    timelineEvents[queueId].push_back(TimelineEvent(event, "memcpyDtoHasync"));
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
      int queueId = IT.first;

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
		<< queueId << "\n";
      }
    }


    outfile.close();
  }

  void
  Timeline::pushPartition(double *partition) {
    partitions.insert(partitions.end(), &partition[0], &partition[nbDevices+1]);
  }

  void
  Timeline::pushReqPartition(double *partition) {
    reqPartitions.insert(reqPartitions.end(), &partition[0], &partition[nbDevices+1]);
  }


  void
  Timeline::pushD2HTransfersWithoutSampling(unsigned iter, unsigned kerFrom, unsigned kerTo,
					    double *granuFrom, double *granuTo,
					    double *granuFromPrefix, double *granuToPrefix,
					    unsigned dev, unsigned size) {
    D2HTransfers.push_back(new TimelineTransfer(iter, kerFrom, kerTo, granuFrom,
						granuTo, granuFromPrefix, granuToPrefix, dev, size, nbDevices));
  }

  void
  Timeline::pushD2HTransfersWithSampling(unsigned iter, unsigned kerFrom, unsigned kerTo,
					 double *granuFrom, double *granuTo,
					 double *granuFromPrefix, double *granuToPrefix,
					 unsigned dev, unsigned size,
					 int *constraint, double X, double Y) {
    D2HTransfers.push_back(new TimelineTransfer(iter, kerFrom, kerTo, granuFrom,
						granuTo, granuFromPrefix, granuToPrefix, dev, size, nbDevices,
						constraint, X, Y));
  }

  void
  Timeline::pushH2DTransfersWithoutSampling(unsigned iter, unsigned kerFrom, unsigned kerTo,
					    double *granuFrom, double *granuTo,
					    double *granuFromPrefix, double *granuToPrefix,
					    unsigned dev, unsigned size) {
    H2DTransfers.push_back(new TimelineTransfer(iter, kerFrom, kerTo, granuFrom,
						granuTo, granuFromPrefix, granuToPrefix, dev, size, nbDevices));
  }

  void
  Timeline::pushH2DTransfersWithSampling(unsigned iter, unsigned kerFrom, unsigned kerTo,
					 double *granuFrom, double *granuTo,
					 double *granuFromPrefix, double *granuToPrefix,
					 unsigned dev, unsigned size,
					 int *constraint, double X, double Y) {
    H2DTransfers.push_back(new TimelineTransfer(iter, kerFrom, kerTo, granuFrom,
						granuTo, granuFromPrefix, granuToPrefix, dev, size, nbDevices,
						constraint, X, Y));
  }

  void
  Timeline::pushD2HPoint(unsigned kerFrom, unsigned kerTo, unsigned buffer,
			 double X, double Y) {
    kerto2Kerform2Buffer2D2HPoint[kerTo][kerFrom][buffer].push_back(std::make_pair(X, Y));
  }

  void
  Timeline::pushH2DPoint(unsigned kerFrom, unsigned kerTo, unsigned buffer,
			 double X, double Y) {
    kerto2Kerform2Buffer2H2DPoint[kerTo][kerFrom][buffer].push_back(std::make_pair(X, Y));
  }

  void
  Timeline::pushD2HPointForDevice(unsigned kerFrom, unsigned kerTo, unsigned device,
				  unsigned buffer, double X, double Y) {
    kerto2Kerform2Buffer2Device2D2HPoint[kerTo][kerFrom][buffer][device].push_back(std::make_pair(X, Y));
  }

  void
  Timeline::pushH2DPointForDevice(unsigned kerFrom, unsigned kerTo, unsigned device,
			 unsigned buffer, double X, double Y) {
    kerto2Kerform2Buffer2Device2H2DPoint[kerTo][kerFrom][buffer][device].push_back(std::make_pair(X, Y));
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

  void
  Timeline::writeReqPartitions(std::string &filename) const {
    ofstream outfile;
    outfile.open(filename);

    int rowSize = nbDevices+1;
    int offset = 0;
    while (offset + nbDevices < reqPartitions.size()) {
      for (unsigned i=0; i<nbDevices+1; i++)
	outfile << reqPartitions[offset+i] << " ";
      outfile << "\n";

      offset += rowSize;
    }

    outfile.close();
  }

  void
  Timeline::writeD2HTransfers() const {
    std::set<unsigned> kernelsTo;
    for (Timeline::TimelineTransfer *t : D2HTransfers)
      kernelsTo.insert(t->kerTo);

    for (unsigned kernelTo : kernelsTo) {
      std::string plotFilename = std::string("plot-D2H-ker-") + to_string(kernelTo) +
	".gnu";
      std::string dataFilename = std::string("data-D2H-ker-") + to_string(kernelTo) +
	".dat";
      std::string pdfFilename = std::string("D2H-ker-") + to_string(kernelTo) +
	".pdf";
      ofstream plotFile; plotFile.open(plotFilename);
      ofstream dataFile; dataFile.open(dataFilename);
      unsigned dataFileRowOffset = 1;

      // Plot file
      plotFile << "set terminal pdf\n"
	       << "set output \"" << pdfFilename << "\"\n"
	       << "set grid layerdefault   lt 0 linecolor 0 linewidth 0.500,  lt 0 linecolor 0 linewidth 0.500\n"
	       << "unset key\n"
	       << "set yrange [0.5:4.5] noreverse nowriteback\n"
	       << "set palette model RGB defined ( 1 'blue', 2 'red', 3 'green')\n"
	       << "unset colorbox\n";


      // Data file
      for (Timeline::TimelineTransfer *t : D2HTransfers) {
	if (t->kerTo != kernelTo)
	  continue;

	unsigned startRow = dataFileRowOffset;

	// ker from granu
	for (unsigned d=0; d<nbDevices; d++) {
	  dataFile << "fromK" << t->kerFrom << "\t"
		   << plotKerFromY << "\t"
		   << t->granuFromPrefix[d] << "\t"
		   << t->granuFrom[d] << "\t"
		   << plotKerColor << "\n";
	  dataFileRowOffset++;
	}
	// ker to granu
	for (unsigned d=0; d<nbDevices; d++) {
	  dataFile << "toK" << t->kerTo << "\t"
		   << plotKerToY << "\t"
		   << t->granuToPrefix[d] << "\t"
		   << t->granuTo[d] << "\t"
		   << plotKerColor << "\n";
	  dataFileRowOffset++;
	}

	if (t->sampled) {

	  // contraints from
	  for (unsigned d=0; d<nbDevices; d++) {
	    if (t->constraint[d] == 0)
	      continue;

	    if (t->constraint[d] == 1) {
	      dataFile << "constraint+" << "\t"
		       << plotKerFromConstraintPosY << "\t"
		       << t->granuFromPrefix[d] << "\t"
		       << t->granuFrom[d] << "\t"
		       << plotConstraintPosColor << "\n";
	      dataFileRowOffset++;
	    } else if (t->constraint[d] == -1) {
	      dataFile << "constraint-" << "\t"
		       << plotKerFromConstraintNegY << "\t"
		       << t->granuFromPrefix[d] << "\t"
		       << t->granuFrom[d] << "\t"
		       << plotConstraintNegColor << "\n";
	      dataFileRowOffset++;
	    } else {
	      std::cerr << t->constraint[d] << "\n";
	      abort();
	    }
	  }

	  // contraints to
	  for (unsigned d=0; d<nbDevices; d++) {
	    if (t->constraint[nbDevices+d] == 0)
	      continue;

	    if (t->constraint[nbDevices+d] == 1) {
	      dataFile << "constraint+" << "\t"
		       << plotKerToConstraintPosY << "\t"
		       << t->granuToPrefix[d] << "\t"
		       << t->granuTo[d] << "\t"
		       << plotConstraintPosColor << "\n";
	      dataFileRowOffset++;
	    }
	    else if (t->constraint[nbDevices+d] == -1) {
	      dataFile << "constraint-" << "\t"
		       << plotKerToConstraintNegY << "\t"
		       << t->granuToPrefix[d] << "\t"
		       << t->granuTo[d] << "\t"
		       << plotConstraintNegColor << "\n";
	      dataFileRowOffset++;
	    } else {
	      std::cerr << t->constraint[nbDevices+d] << "\n";
	      abort();
	    }
	  }
	}

	unsigned endRow = dataFileRowOffset-1;

	// Plot file
	if (t->sampled)
	  plotFile << "set title \"iter " << t->iter << " k" << t->kerFrom << " to k" << t->kerTo
		   << " on dev " << t->dev << " X="<<t->X << " Y=" <<t->Y << "\"\n";
	else
	  plotFile << "set title \"iter " << t->iter << " k" << t->kerFrom << " to k" << t->kerTo
		   << " on dev " << t->dev << " NOT SAMPLED\"\n";


	plotFile << "plot \"<(sed -n '"<<startRow<<","
		 <<endRow<<"p' "
		 << dataFilename << ")\" using 3 : 2 : 4 : (0.0) : 5 : yticlabel(1) with vectors palette heads, \\\n"
		 << "\"<(sed -n '"<<startRow<<","
		 <<endRow<<"p' "
		 << dataFilename << ")\" using ($3+($4)/2) : ($2+0.1) : 4 with labels left, \\\n"
		 << "\"<(sed -n '"<<startRow<<","
		 <<endRow<<"p' "
		 << dataFilename << ")\" using 3 : (0.0) : (0.0) : (4.5) with vectors nohead dt 2 lc \"blue\"\n";

      }
    }
  }

  void
  Timeline::writeH2DTransfers() const {
    std::set<unsigned> kernelsTo;
    for (Timeline::TimelineTransfer *t : H2DTransfers)
      kernelsTo.insert(t->kerTo);

    for (unsigned kernelTo : kernelsTo) {
      std::string plotFilename = std::string("plot-H2D-ker-") + to_string(kernelTo) +
	".gnu";
      std::string dataFilename = std::string("data-H2D-ker-") + to_string(kernelTo) +
	".dat";
      std::string pdfFilename = std::string("H2D-ker-") + to_string(kernelTo) +
	".pdf";
      ofstream plotFile; plotFile.open(plotFilename);
      ofstream dataFile; dataFile.open(dataFilename);
      unsigned dataFileRowOffset = 1;

      // Plot file
      plotFile << "set terminal pdf\n"
	       << "set output \"" << pdfFilename << "\"\n"
	       << "set grid layerdefault   lt 0 linecolor 0 linewidth 0.500,  lt 0 linecolor 0 linewidth 0.500\n"
	       << "unset key\n"
	       << "set yrange [0.5:4.5] noreverse nowriteback\n"
	       << "set palette model RGB defined ( 1 'blue', 2 'red', 3 'green')\n"
	       << "unset colorbox\n";


      // Data file
      for (Timeline::TimelineTransfer *t : H2DTransfers) {
	if (t->kerTo != kernelTo)
	  continue;

	unsigned startRow = dataFileRowOffset;

	// ker from granu
	for (unsigned d=0; d<nbDevices; d++) {
	  dataFile << "fromK" << t->kerFrom << "\t"
		   << plotKerFromY << "\t"
		   << t->granuFromPrefix[d] << "\t"
		   << t->granuFrom[d] << "\t"
		   << plotKerColor << "\n";
	  dataFileRowOffset++;
	}
	// ker to granu
	for (unsigned d=0; d<nbDevices; d++) {
	  dataFile << "toK" << t->kerTo << "\t"
		   << plotKerToY << "\t"
		   << t->granuToPrefix[d] << "\t"
		   << t->granuTo[d] << "\t"
		   << plotKerColor << "\n";
	  dataFileRowOffset++;
	}

	if (t->sampled) {

	  // contraints from
	  for (unsigned d=0; d<nbDevices; d++) {
	    if (t->constraint[d] == 0)
	      continue;

	    if (t->constraint[d] == 1) {
	      dataFile << "constraint+" << "\t"
		       << plotKerFromConstraintPosY << "\t"
		       << t->granuFromPrefix[d] << "\t"
		       << t->granuFrom[d] << "\t"
		       << plotConstraintPosColor << "\n";
	      dataFileRowOffset++;
	    } else if (t->constraint[d] == -1) {
	      dataFile << "constraint-" << "\t"
		       << plotKerFromConstraintNegY << "\t"
		       << t->granuFromPrefix[d] << "\t"
		       << t->granuFrom[d] << "\t"
		       << plotConstraintNegColor << "\n";
	      dataFileRowOffset++;
	    } else {
	      std::cerr << t->constraint[d] << "\n";
	      abort();
	    }
	  }

	  // contraints to
	  for (unsigned d=0; d<nbDevices; d++) {
	    if (t->constraint[nbDevices+d] == 0)
	      continue;

	    if (t->constraint[nbDevices+d] == 1) {
	      dataFile << "constraint+" << "\t"
		       << plotKerToConstraintPosY << "\t"
		       << t->granuToPrefix[d] << "\t"
		       << t->granuTo[d] << "\t"
		       << plotConstraintPosColor << "\n";
	      dataFileRowOffset++;
	    }
	    else if (t->constraint[nbDevices+d] == -1) {
	      dataFile << "constraint-" << "\t"
		       << plotKerToConstraintNegY << "\t"
		       << t->granuToPrefix[d] << "\t"
		       << t->granuTo[d] << "\t"
		       << plotConstraintNegColor << "\n";
	      dataFileRowOffset++;
	    } else {
	      std::cerr << t->constraint[nbDevices+d] << "\n";
	      abort();
	    }
	  }
	}

	unsigned endRow = dataFileRowOffset-1;

	// Plot file
	if (t->sampled)
	  plotFile << "set title \"iter " << t->iter << " k" << t->kerFrom << " to k" << t->kerTo
		   << " on dev " << t->dev << " X="<<t->X << " Y=" <<t->Y << "\"\n";
	else
	  plotFile << "set title \"iter " << t->iter << " k" << t->kerFrom << " to k" << t->kerTo
		   << " on dev " << t->dev << " NOT SAMPLED\"\n";


	plotFile << "plot \"<(sed -n '"<<startRow<<","
		 <<endRow<<"p' "
		 << dataFilename << ")\" using 3 : 2 : 4 : (0.0) : 5 : yticlabel(1) with vectors palette heads, \\\n"
		 << "\"<(sed -n '"<<startRow<<","
		 <<endRow<<"p' "
		 << dataFilename << ")\" using ($3+($4)/2) : ($2+0.1) : 4 with labels left, \\\n"
		 << "\"<(sed -n '"<<startRow<<","
		 <<endRow<<"p' "
		 << dataFilename << ")\" using 3 : (0.0) : (0.0) : (4.5) with vectors nohead dt 2 lc \"blue\"\n";

      }
    }
  }

  void
  Timeline::writeH2DTransfersSampling() const {
    std::set<unsigned> kernelsTo;
    for (Timeline::TimelineTransfer *t : H2DTransfers)
      kernelsTo.insert(t->kerTo);

    for (unsigned kernelTo : kernelsTo) {
      std::string plotFilename = std::string("plot-H2D-ker-") + to_string(kernelTo) +
	"-sampling.gnu";
      std::string dataFilename = std::string("data-H2D-ker-") + to_string(kernelTo) +
	"-sampling.dat";
      std::string pdfFilename = std::string("H2D-ker-") + to_string(kernelTo) +
	"-sampling.pdf";
      ofstream plotFile; plotFile.open(plotFilename);
      ofstream dataFile; dataFile.open(dataFilename);
      unsigned dataFileRowOffset = 1;

      // Plot file
      plotFile << "set terminal pdf\n"
	       << "set output \"" << pdfFilename << "\"\n"
	       << "set offset graph 0.1, 0.1, 0.1, 0.1\n"
	       << "unset key\n";

      // Data file
      std::set<unsigned> kernelsFrom;
      for (Timeline::TimelineTransfer *t : H2DTransfers) {
	if (!t->sampled || t->kerTo != kernelTo)
	  continue;
	kernelsFrom.insert(t->kerFrom);
      }

      for (unsigned kernelFrom : kernelsFrom) {
	unsigned startRow = dataFileRowOffset;

	for (Timeline::TimelineTransfer *t : H2DTransfers) {
	  if (!t->sampled || t->kerTo != kernelTo || t->kerFrom != kernelFrom)
	    continue;

	  dataFile << t->X << "\t" << t->Y << "\n";
	  dataFileRowOffset++;
	}

	unsigned endRow = dataFileRowOffset-1;

	// Plot file
	plotFile << "set title \"k" << kernelFrom << " to k" << kernelTo << "\"\n";
	plotFile << "plot \"<(sed -n '"<<startRow<<","
		 <<endRow<<"p' "
		 << dataFilename << ")\" using 1 : 2 with linespoints\n";
      }
    }
  }

  void
  Timeline::writeD2HTransfersSampling() const {
    std::set<unsigned> kernelsTo;
    for (Timeline::TimelineTransfer *t : D2HTransfers)
      kernelsTo.insert(t->kerTo);

    for (unsigned kernelTo : kernelsTo) {
      std::string plotFilename = std::string("plot-D2H-ker-") + to_string(kernelTo) +
	"-sampling.gnu";
      std::string dataFilename = std::string("data-D2H-ker-") + to_string(kernelTo) +
	"-sampling.dat";
      std::string pdfFilename = std::string("D2H-ker-") + to_string(kernelTo) +
	"-sampling.pdf";
      ofstream plotFile; plotFile.open(plotFilename);
      ofstream dataFile; dataFile.open(dataFilename);
      unsigned dataFileRowOffset = 1;

      // Plot file
      plotFile << "set terminal pdf\n"
	       << "set output \"" << pdfFilename << "\"\n"
	       << "set offset graph 0.1, 0.1, 0.1, 0.1\n"
	       << "unset key\n";

      // Data file
      std::set<unsigned> kernelsFrom;
      for (Timeline::TimelineTransfer *t : D2HTransfers) {
	if (!t->sampled || t->kerTo != kernelTo)
	  continue;
	kernelsFrom.insert(t->kerFrom);
      }

      for (unsigned kernelFrom : kernelsFrom) {
	unsigned startRow = dataFileRowOffset;

	for (Timeline::TimelineTransfer *t : D2HTransfers) {
	  if (!t->sampled || t->kerTo != kernelTo || t->kerFrom != kernelFrom)
	    continue;

	  dataFile << t->X << "\t" << t->Y << "\n";
	  dataFileRowOffset++;
	}

	unsigned endRow = dataFileRowOffset-1;

	// Plot file
	plotFile << "set title \"k" << kernelFrom << " to k" << kernelTo << "\"\n";
	plotFile << "plot \"<(sed -n '"<<startRow<<","
		 <<endRow<<"p' "
		 << dataFilename << ")\" using 1 : 2 with linespoints\n";
      }
    }
  }

  void
  Timeline::writeD2HPoints() const {
    std::string plotFilename = std::string("plot-D2H-points.gnu");
    std::string dataFilename = std::string("data-D2H-points.dat");
    std::string pdfFilename = std::string("D2H-points.pdf");
    ofstream plotFile; plotFile.open(plotFilename);
    ofstream dataFile; dataFile.open(dataFilename);
    unsigned dataFileRowOffset = 1;

    // Plot file
    plotFile << "set terminal pdf\n"
	     << "set output \"" << pdfFilename << "\"\n"
	     << "set offset graph 0.1, 0.1, 0.1, 0.1\n"
	     << "unset key\n";

    for (auto kertoIT : kerto2Kerform2Buffer2D2HPoint) {
      for (auto kerfromIT : kertoIT.second) {
	for (auto bufferIT : kerfromIT.second) {
	  unsigned from = kerfromIT.first;
	  unsigned to = kertoIT.first;
	  unsigned buffer = bufferIT.first;
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << "\"\n";

	  unsigned startRow = dataFileRowOffset;

	  double samplesX[bufferIT.second.size()];
	  double samplesY[bufferIT.second.size()];
	  double estY[bufferIT.second.size()];
	  unsigned sampleId=0;

	  for (auto pointIT : bufferIT.second) {
	    double x = pointIT.first;
	    double y = pointIT.second;
	    samplesX[sampleId] = x;
	    samplesY[sampleId] = y;
	    sampleId++;

	    dataFile << x << "\t" << y << "\n";
	    dataFileRowOffset++;

	  }

	  double c0, c1, cov00, cov01, cov11, sumsq, yerr, error, sstot, ssres;
	  if (sampleId > 0) {
	    gsl_fit_linear(samplesX, 1, samplesY, 1, sampleId, &c0, &c1, &cov00,
			   &cov01, &cov11, &sumsq);

	    // Compute Error
	    for (unsigned i=0; i<sampleId; i++)
	      gsl_fit_linear_est(samplesX[i], c0, c1, cov00, cov01, cov11, &estY[i], &yerr);
	    ssres = 0;
	    for (unsigned i=0; i<sampleId; i++){
	      double est = c1 * samplesX[i] + c0;
	      ssres += (samplesY[i] - est)*(samplesY[i] - est);
	    }

	    double mean = 0;
	    for (unsigned i=0; i<sampleId; i++)
	      mean += samplesY[i];
	    mean /= sampleId;
	    sstot = 0;
	    for (unsigned i=0; i<sampleId; i++)
	      sstot += (samplesY[i] - mean) * (samplesY[i] - mean);

	    error = 1.0 - ssres / sstot;
	  }

	  unsigned endRow = dataFileRowOffset-1;

	  if (endRow - startRow > 0) {
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << " R2 " << error << "\"\n";

	    plotFile << "f(x) = "<< c1 << " * x + " << c0 << "\n";
	    // plotFile << "fit f(x) \"<(sed -n '"<<startRow<<"," <<endRow<<"p' "
	    // 	     << dataFilename << ")\" using 1 : 2 via m,b\n";
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints, \\\n"
		     << "f(x)\n";
	  } else {
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints\n";
	  }
	}
      }
    }
  }

  void
  Timeline::writeH2DPoints() const {
    std::string plotFilename = std::string("plot-H2D-points.gnu");
    std::string dataFilename = std::string("data-H2D-points.dat");
    std::string pdfFilename = std::string("H2D-points.pdf");
    ofstream plotFile; plotFile.open(plotFilename);
    ofstream dataFile; dataFile.open(dataFilename);
    unsigned dataFileRowOffset = 1;

    // Plot file
    plotFile << "set terminal pdf\n"
	     << "set output \"" << pdfFilename << "\"\n"
	     << "set offset graph 0.1, 0.1, 0.1, 0.1\n"
	     << "unset key\n";

    for (auto kertoIT : kerto2Kerform2Buffer2H2DPoint) {
      for (auto kerfromIT : kertoIT.second) {
	for (auto bufferIT : kerfromIT.second) {
	  unsigned from = kerfromIT.first;
	  unsigned to = kertoIT.first;
	  unsigned buffer = bufferIT.first;
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << "\"\n";

	  unsigned startRow = dataFileRowOffset;

	  double samplesX[bufferIT.second.size()];
	  double samplesY[bufferIT.second.size()];
	  double estY[bufferIT.second.size()];
	  unsigned sampleId=0;

	  for (auto pointIT : bufferIT.second) {
	    double x = pointIT.first;
	    double y = pointIT.second;
	    samplesX[sampleId] = x;
	    samplesY[sampleId] = y;
	    sampleId++;

	    dataFile << x << "\t" << y << "\n";
	    dataFileRowOffset++;

	  }

	  double c0, c1, cov00, cov01, cov11, sumsq, yerr, error, sstot, ssres;
	  if (sampleId > 0) {
	    gsl_fit_linear(samplesX, 1, samplesY, 1, sampleId, &c0, &c1, &cov00,
			   &cov01, &cov11, &sumsq);

	    // Compute Error
	    for (unsigned i=0; i<sampleId; i++)
	      gsl_fit_linear_est(samplesX[i], c0, c1, cov00, cov01, cov11, &estY[i], &yerr);
	    ssres = 0;
	    for (unsigned i=0; i<sampleId; i++){
	      double est = c1 * samplesX[i] + c0;
	      ssres += (samplesY[i] - est)*(samplesY[i] - est);
	    }

	    double mean = 0;
	    for (unsigned i=0; i<sampleId; i++)
	      mean += samplesY[i];
	    mean /= sampleId;
	    sstot = 0;
	    for (unsigned i=0; i<sampleId; i++)
	      sstot += (samplesY[i] - mean) * (samplesY[i] - mean);

	    error = 1.0 - ssres / sstot;
	  }

	  unsigned endRow = dataFileRowOffset-1;

	  if (endRow - startRow > 0) {
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << " R2 " << error << "\"\n";

	    plotFile << "f(x) = "<< c1 << " * x + " << c0 << "\n";
	    // plotFile << "fit f(x) \"<(sed -n '"<<startRow<<"," <<endRow<<"p' "
	    // 	     << dataFilename << ")\" using 1 : 2 via m,b\n";
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints, \\\n"
		     << "f(x)\n";
	  } else {
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints\n";
	  }
	}
      }
    }
  }

  void
  Timeline::writeD2HPointsByDevice() const {
    std::string plotFilename = std::string("plot-D2H-points-by-dev.gnu");
    std::string dataFilename = std::string("data-D2H-points-by-dev.dat");
    std::string pdfFilename = std::string("D2H-points-by-dev.pdf");
    ofstream plotFile; plotFile.open(plotFilename);
    ofstream dataFile; dataFile.open(dataFilename);
    unsigned dataFileRowOffset = 1;

    // Plot file
    plotFile << "set terminal pdf\n"
	     << "set output \"" << pdfFilename << "\"\n"
	     << "unset key\n";

    for (auto kertoIT : kerto2Kerform2Buffer2Device2D2HPoint) {
      for (auto kerfromIT : kertoIT.second) {
	for (auto bufferIT : kerfromIT.second) {
	  for (auto devIT : bufferIT.second) {
	  unsigned from = kerfromIT.first;
	  unsigned to = kertoIT.first;
	  unsigned buffer = bufferIT.first;
	  unsigned device = devIT.first;
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << " dev " << device << "\"\n";

	  unsigned startRow = dataFileRowOffset;

	  double samplesX[devIT.second.size()];
	  double samplesY[devIT.second.size()];
	  double estY[devIT.second.size()];
	  unsigned sampleId=0;

	  for (auto pointIT : devIT.second) {
	    double x = pointIT.first;
	    double y = pointIT.second;
	    samplesX[sampleId] = x;
	    samplesY[sampleId] = y;
	    sampleId++;

	    dataFile << x << "\t" << y << "\n";
	    dataFileRowOffset++;

	  }

	  double c0, c1, cov00, cov01, cov11, sumsq, yerr, error, sstot, ssres;
	  int ret;
	  if (sampleId > 0) {
	    ret = gsl_fit_linear(samplesX, 1, samplesY, 1, sampleId, &c0, &c1, &cov00,
			   &cov01, &cov11, &sumsq);

	    // Compute Error
	    for (unsigned i=0; i<sampleId; i++)
	      gsl_fit_linear_est(samplesX[i], c0, c1, cov00, cov01, cov11, &estY[i], &yerr);
	    ssres = 0;
	    for (unsigned i=0; i<sampleId; i++){
	      double est = c1 * samplesX[i] + c0;
	      ssres += (samplesY[i] - est)*(samplesY[i] - est);
	    }

	    double mean = 0;
	    for (unsigned i=0; i<sampleId; i++)
	      mean += samplesY[i];
	    mean /= sampleId;
	    sstot = 0;
	    for (unsigned i=0; i<sampleId; i++)
	      sstot += (samplesY[i] - mean) * (samplesY[i] - mean);

	    error = 1.0 - ssres / sstot;
	  }

	  unsigned endRow = dataFileRowOffset-1;

	  if (endRow - startRow > 0 && ret == 0) {
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << " dev " << device << " R2 " << error << "\"\n";

	    plotFile << "f(x) = "<< c1 << " * x + " << c0 << "\n";
	    // plotFile << "fit f(x) \"<(sed -n '"<<startRow<<"," <<endRow<<"p' "
	    // 	     << dataFilename << ")\" using 1 : 2 via m,b\n";
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints, \\\n"
		     << "f(x)\n";
	  } else {
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints\n";
	  }
	  }
	}
      }
    }
  }

  void
  Timeline::writeH2DPointsByDevice() const {
    std::string plotFilename = std::string("plot-H2D-points-by-dev.gnu");
    std::string dataFilename = std::string("data-H2D-points-by-dev.dat");
    std::string pdfFilename = std::string("H2D-points-by-dev.pdf");
    ofstream plotFile; plotFile.open(plotFilename);
    ofstream dataFile; dataFile.open(dataFilename);
    unsigned dataFileRowOffset = 1;

    // Plot file
    plotFile << "set terminal pdf\n"
	     << "set output \"" << pdfFilename << "\"\n"
	     << "unset key\n";

    for (auto kertoIT : kerto2Kerform2Buffer2Device2H2DPoint) {
      for (auto kerfromIT : kertoIT.second) {
	for (auto bufferIT : kerfromIT.second) {
	  for (auto devIT : bufferIT.second) {
	  unsigned from = kerfromIT.first;
	  unsigned to = kertoIT.first;
	  unsigned buffer = bufferIT.first;
	  unsigned device = devIT.first;
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << " dev " << device << "\"\n";

	  unsigned startRow = dataFileRowOffset;

	  double samplesX[devIT.second.size()];
	  double samplesY[devIT.second.size()];
	  double estY[devIT.second.size()];
	  unsigned sampleId=0;

	  for (auto pointIT : devIT.second) {
	    double x = pointIT.first;
	    double y = pointIT.second;
	    samplesX[sampleId] = x;
	    samplesY[sampleId] = y;
	    sampleId++;

	    dataFile << x << "\t" << y << "\n";
	    dataFileRowOffset++;

	  }

	  double c0, c1, cov00, cov01, cov11, sumsq, yerr, error, sstot, ssres;
	  int ret;
	  if (sampleId > 0) {
	    ret = gsl_fit_linear(samplesX, 1, samplesY, 1, sampleId, &c0, &c1, &cov00,
			   &cov01, &cov11, &sumsq);

	    // Compute Error
	    for (unsigned i=0; i<sampleId; i++)
	      gsl_fit_linear_est(samplesX[i], c0, c1, cov00, cov01, cov11, &estY[i], &yerr);
	    ssres = 0;
	    for (unsigned i=0; i<sampleId; i++){
	      double est = c1 * samplesX[i] + c0;
	      ssres += (samplesY[i] - est)*(samplesY[i] - est);
	    }

	    double mean = 0;
	    for (unsigned i=0; i<sampleId; i++)
	      mean += samplesY[i];
	    mean /= sampleId;
	    sstot = 0;
	    for (unsigned i=0; i<sampleId; i++)
	      sstot += (samplesY[i] - mean) * (samplesY[i] - mean);

	    error = 1.0 - ssres / sstot;
	  }

	  unsigned endRow = dataFileRowOffset-1;

	  if (endRow - startRow > 0 && ret == 0) {
	  // Plot file
	  plotFile << "set title \"k" << from << " to k" << to
		   << " b " << buffer << " dev " << device << " R2 " << error << "\"\n";

	    plotFile << "f(x) = "<< c1 << " * x + " << c0 << "\n";
	    // plotFile << "fit f(x) \"<(sed -n '"<<startRow<<"," <<endRow<<"p' "
	    // 	     << dataFilename << ")\" using 1 : 2 via m,b\n";
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints, \\\n"
		     << "f(x)\n";
	  } else {
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints\n";
	  }
	  }
	}
      }
    }
  }

  void
  Timeline::writeH2DThroughput() const {
    std::string plotFilename = std::string("plot-H2D-throughput.gnu");
    std::string dataFilename = std::string("data-H2D-throughput.dat");
    std::string pdfFilename = std::string("H2D-throughput.pdf");
    ofstream plotFile; plotFile.open(plotFilename);
    ofstream dataFile; dataFile.open(dataFilename);
    unsigned dataFileRowOffset = 1;

    // Plot file
    plotFile << "set terminal pdf\n"
	     << "set output \"" << pdfFilename << "\"\n"
	     << "unset key\n";

    for (auto devIT : H2DThroughputPointPerDevice) {
      unsigned dev = devIT.first;
      // Plot file
      plotFile << "set title \"H2D throughput dev " << dev << "\"\n";

      unsigned startRow = dataFileRowOffset;

      double samplesX[devIT.second.size()];
      double samplesY[devIT.second.size()];
      double estY[devIT.second.size()];
      unsigned sampleId=0;

      for (auto pointIT : devIT.second) {
	double x = pointIT.first;
	double y = pointIT.second;
	samplesX[sampleId] = x;
	samplesY[sampleId] = y;
	sampleId++;

	dataFile << x << "\t" << y << "\n";
	dataFileRowOffset++;

      }

      double c0, c1, cov00, cov01, cov11, sumsq, yerr, error, sstot, ssres;
      int ret;
      if (sampleId > 0) {
	ret = gsl_fit_linear(samplesX, 1, samplesY, 1, sampleId, &c0, &c1, &cov00,
			     &cov01, &cov11, &sumsq);

	// Compute Error
	for (unsigned i=0; i<sampleId; i++)
	  gsl_fit_linear_est(samplesX[i], c0, c1, cov00, cov01, cov11, &estY[i], &yerr);
	ssres = 0;
	for (unsigned i=0; i<sampleId; i++){
	  double est = c1 * samplesX[i] + c0;
	  ssres += (samplesY[i] - est)*(samplesY[i] - est);
	}

	double mean = 0;
	for (unsigned i=0; i<sampleId; i++)
	  mean += samplesY[i];
	mean /= sampleId;
	sstot = 0;
	for (unsigned i=0; i<sampleId; i++)
	  sstot += (samplesY[i] - mean) * (samplesY[i] - mean);

	error = 1.0 - ssres / sstot;
      }

      unsigned endRow = dataFileRowOffset-1;

	  if (endRow - startRow > 0 && ret == 0) {
	  // Plot file
	  plotFile << "set title \"H2D troughput dev " << dev
		   << " R2 " << error << "\"\n";

	    plotFile << "f(x) = "<< c1 << " * x + " << c0 << "\n";
	    // plotFile << "fit f(x) \"<(sed -n '"<<startRow<<"," <<endRow<<"p' "
	    // 	     << dataFilename << ")\" using 1 : 2 via m,b\n";
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints, \\\n"
		     << "f(x)\n";
	  } else {
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints\n";
	  }
	  }
  }

  void
  Timeline::writeD2HThroughput() const {
    std::string plotFilename = std::string("plot-D2H-throughput.gnu");
    std::string dataFilename = std::string("data-D2H-throughput.dat");
    std::string pdfFilename = std::string("D2H-throughput.pdf");
    ofstream plotFile; plotFile.open(plotFilename);
    ofstream dataFile; dataFile.open(dataFilename);
    unsigned dataFileRowOffset = 1;

    // Plot file
    plotFile << "set terminal pdf\n"
	     << "set output \"" << pdfFilename << "\"\n"
	     << "unset key\n";

    for (auto devIT : D2HThroughputPointPerDevice) {
      unsigned dev = devIT.first;
      // Plot file
      plotFile << "set title \"D2H throughput dev " << dev << "\"\n";

      unsigned startRow = dataFileRowOffset;

      double samplesX[devIT.second.size()];
      double samplesY[devIT.second.size()];
      double estY[devIT.second.size()];
      unsigned sampleId=0;

      for (auto pointIT : devIT.second) {
	double x = pointIT.first;
	double y = pointIT.second;
	samplesX[sampleId] = x;
	samplesY[sampleId] = y;
	sampleId++;

	dataFile << x << "\t" << y << "\n";
	dataFileRowOffset++;

      }

      double c0, c1, cov00, cov01, cov11, sumsq, yerr, error, sstot, ssres;
      int ret;
      if (sampleId > 0) {
	ret = gsl_fit_linear(samplesX, 1, samplesY, 1, sampleId, &c0, &c1, &cov00,
			     &cov01, &cov11, &sumsq);

	// Compute Error
	for (unsigned i=0; i<sampleId; i++)
	  gsl_fit_linear_est(samplesX[i], c0, c1, cov00, cov01, cov11, &estY[i], &yerr);
	ssres = 0;
	for (unsigned i=0; i<sampleId; i++){
	  double est = c1 * samplesX[i] + c0;
	  ssres += (samplesY[i] - est)*(samplesY[i] - est);
	}

	double mean = 0;
	for (unsigned i=0; i<sampleId; i++)
	  mean += samplesY[i];
	mean /= sampleId;
	sstot = 0;
	for (unsigned i=0; i<sampleId; i++)
	  sstot += (samplesY[i] - mean) * (samplesY[i] - mean);

	error = 1.0 - ssres / sstot;
      }

      unsigned endRow = dataFileRowOffset-1;

	  if (endRow - startRow > 0 && ret == 0) {
	  // Plot file
	  plotFile << "set title \"D2H troughput dev " << dev
		   << " R2 " << error << "\"\n";

	    plotFile << "f(x) = "<< c1 << " * x + " << c0 << "\n";
	    // plotFile << "fit f(x) \"<(sed -n '"<<startRow<<"," <<endRow<<"p' "
	    // 	     << dataFilename << ")\" using 1 : 2 via m,b\n";
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints, \\\n"
		     << "f(x)\n";
	  } else {
	    plotFile << "plot \"<(sed -n '"<<startRow<<","
		     << endRow<<"p' "
		     << dataFilename << ")\" using 1 : 2 with linespoints\n";
	  }
	  }
  }

  void
  Timeline::pushD2HTroughputPoint(unsigned device, unsigned nbBytes,
				  double time) {
    D2HThroughputPointPerDevice[device].push_back(std::make_pair(nbBytes, time));
  }

  void
  Timeline::pushH2DTroughputPoint(unsigned device, unsigned nbBytes,
				  double time) {
    H2DThroughputPointPerDevice[device].push_back(std::make_pair(nbBytes, time));
  }


};
