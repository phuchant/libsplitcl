#include <Dispatch/OpenCLFunctions.h>
#include <BufferManager.h>
#include <Globals.h>
#include <Options.h>
#include <Init.h>

#include <iostream>

namespace libsplit {
  bool hasBeenInitialized = false;

  void init() {
    if (hasBeenInitialized)
      return;

    hasBeenInitialized = true;
    getOpenCLFunctions();
    parseEnvOptions();
    driver = new Driver();
    timeline = new Timeline(optDeviceSelection.size() / 2);
  }

};
