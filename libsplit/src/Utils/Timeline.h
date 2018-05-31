#ifndef TIMELINE_H
#define TIMELINE_H

#include <Queue/Event.h>

#include <CL/cl.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include <cstring>

namespace libsplit {

  class Timeline {
  private:
    struct TimelineEvent {
      TimelineEvent(Event *event, std::string method);
      ~TimelineEvent();

      Event *event;
      std::string method;
    };

    struct TimelineTransfer {
      TimelineTransfer(unsigned iter, unsigned kerFrom, unsigned kerTo,
		       double *granuFrom, double *granuTo,
		       double *granuFromPrefix, double *granuToPrefix,
		       unsigned dev, unsigned size, unsigned nbDevices)
	: iter(iter), kerFrom(kerFrom), kerTo(kerTo), dev(dev), size(size),
	  sampled(false) {
	memcpy(this->granuFrom, granuFrom, nbDevices * sizeof(double));
	memcpy(this->granuTo, granuTo, nbDevices * sizeof(double));
	memcpy(this->granuFromPrefix, granuFromPrefix, nbDevices * sizeof(double));
	memcpy(this->granuToPrefix, granuToPrefix, nbDevices * sizeof(double));
      }

      TimelineTransfer(unsigned iter, unsigned kerFrom, unsigned kerTo,
		       double *granuFrom, double *granuTo,
		       double *granuFromPrefix, double *granuToPrefix,
		       unsigned dev, unsigned size, unsigned nbDevices,
		       int *constraint, double X, double Y)
	: iter(iter), kerFrom(kerFrom), kerTo(kerTo), dev(dev), size(size),
	  X(X), Y(Y), sampled(true) {
	memcpy(this->constraint, constraint, 2*nbDevices*sizeof(int));
	memcpy(this->granuFrom, granuFrom, nbDevices * sizeof(double));
	memcpy(this->granuTo, granuTo, nbDevices * sizeof(double));
	memcpy(this->granuFromPrefix, granuFromPrefix, nbDevices * sizeof(double));
	memcpy(this->granuToPrefix, granuToPrefix, nbDevices * sizeof(double));
      }

      ~TimelineTransfer() {}

      unsigned iter;
      unsigned kerFrom;
      unsigned kerTo;
      unsigned dev;
      unsigned size;
      int constraint[10];
      double granuFrom[5];
      double granuTo[5];
      double granuFromPrefix[5];
      double granuToPrefix[5];
      double X;
      double Y;
      bool sampled;
    };

  public:
    Timeline(unsigned nbDevices) : nbDevices(nbDevices) {}
    ~Timeline() {}

    void pushEvent(Event *event, std::string &method, int queueId);
    void pushH2DEvent(Event *event, int queueId);
    void pushD2HEvent(Event *event, int queueId);

    void writeTrace(std::string &filename) const;
    void writePartitions(std::string &filename) const;
    void writeReqPartitions(std::string &filename) const;
    void writeD2HTransfers() const;
    void writeH2DTransfers() const;

    void writeD2HTransfersSampling() const;
    void writeH2DTransfersSampling() const;

    void writeD2HPoints() const;
    void writeH2DPoints() const;

    void pushPartition(double *partition);
    void pushReqPartition(double *partition);

    void pushD2HTransfersWithoutSampling(unsigned iter, unsigned kerFrom, unsigned kerTo,
					 double *granuFrom, double *granuTo,
					 double *granuFromPrefix, double *granuToPrefix,
					 unsigned dev, unsigned size);
    void pushD2HTransfersWithSampling(unsigned iter, unsigned kerFrom, unsigned kerTo,
				      double *granuFrom, double *granuTo,
				      double *granuFromPrefix, double *granuToPrefix,
				      unsigned dev, unsigned size,
				      int *constraint, double X, double Y);
    void pushH2DTransfersWithoutSampling(unsigned iter, unsigned kerFrom, unsigned kerTo,
					 double *granuFrom, double *granuTo,
					 double *granuFromPrefix, double *granuToPrefix,
					 unsigned dev, unsigned size);
    void pushH2DTransfersWithSampling(unsigned iter, unsigned kerFrom, unsigned kerTo,
				      double *granuFrom, double *granuTo,
				      double *granuFromPrefix, double *granuToPrefix,
				      unsigned dev, unsigned size,
				      int *constraint, double X, double Y);
    void pushD2HPoint(unsigned kerFrom, unsigned kerTo, unsigned buffer, double X, double Y);
    void pushH2DPoint(unsigned kerFrom, unsigned kerTo, unsigned buffer, double X, double Y);
    void pushD2HPointForDevice(unsigned kerFrom, unsigned kerTo, unsigned d,
			       unsigned buffer, double X, double Y);
    void pushH2DPointForDevice(unsigned kerFrom, unsigned kerTo, unsigned d,
		      unsigned buffer, double X, double Y);
    void writeD2HPointsByDevice() const;
    void writeH2DPointsByDevice() const;

    void pushH2DTroughputPoint(unsigned device, unsigned nbBytes, double time);
    void pushD2HTroughputPoint(unsigned device, unsigned nbBytes, double time);
    void writeH2DThroughput() const;
    void writeD2HThroughput() const;

  private:
    unsigned nbDevices;
    std::set<int> devices;

    std::map<int, std::vector<Timeline::TimelineEvent> > timelineEvents;

    std::vector<double> partitions;
    std::vector<double> reqPartitions;

    std::vector<Timeline::TimelineTransfer *> D2HTransfers;
    std::vector<Timeline::TimelineTransfer *> H2DTransfers;

    const float plotKerFromY = 2;
    const float plotKerToY = 4;
    const int plotKerColor = 1;
    const float plotKerFromConstraintPosY = 1.5;
    const float plotKerFromConstraintNegY = 1;
    const float plotKerToConstraintPosY = 3.5;
    const float plotKerToConstraintNegY = 3;
    const int plotConstraintPosColor = 3;
    const int plotConstraintNegColor = 2;
    std::map<unsigned, std::map<unsigned, std::map<unsigned, std::vector<std::pair<double, double> > > > > kerto2Kerform2Buffer2D2HPoint;
    std::map<unsigned, std::map<unsigned, std::map<unsigned, std::vector<std::pair<double, double> > > > > kerto2Kerform2Buffer2H2DPoint;

    std::map<unsigned, std::map<unsigned, std::map<unsigned, std::map<unsigned, std::vector<std::pair<double, double> > > > > > kerto2Kerform2Buffer2Device2D2HPoint;
    std::map<unsigned, std::map<unsigned, std::map<unsigned, std::map<unsigned, std::vector<std::pair<double, double> > > > > > kerto2Kerform2Buffer2Device2H2DPoint;

    std::map<unsigned, std::vector<std::pair<double, double> > > H2DThroughputPointPerDevice;
    std::map<unsigned, std::vector<std::pair<double, double> > > D2HThroughputPointPerDevice;
  };

};


#endif /* TIMELINE_H */
