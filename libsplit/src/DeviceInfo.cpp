#include "DeviceInfo.h"

DeviceInfo::DeviceInfo()
  : deviceIndex(0), queue(NULL), dataRequired(NULL), dataWritten(NULL),
    needToRead(NULL), needToWrite(NULL),
    nbKernels(0), kernelsIndex(NULL), kernelsEvent(NULL), mGlobalWorkSize(NULL),
    mGlobalWorkOffset(NULL) {}

DeviceInfo::~DeviceInfo() {
  delete[] dataRequired;
  delete[] dataWritten;
  delete[] needToRead;
  delete[] needToWrite;
  delete[] kernelsIndex;
  delete[] kernelsEvent;

  for (unsigned i=0; i<nbKernels; i++) {
    delete[] mGlobalWorkSize[i];
    delete[] mGlobalWorkOffset[i];
  }

  delete[] mGlobalWorkSize;
  delete[] mGlobalWorkOffset;
}
