#include <Utils/Debug.h>

#include <iostream>
#include <string>
#include <vector>

namespace libsplit {

  bool DebugFlag = false;
  std::vector<std::string> CurrentDebugType;

  bool isCurrentDebugType(const char *DebugType) {
    if (CurrentDebugType.empty())
      return true;

    // See if DebugType is in list. Note: do not use find() as that forces us to
    // unnecessarily create an std::string instance.
    for (auto &d : CurrentDebugType) {
      if (d == DebugType)
	return true;
    }
    return false;
  }

  void setCurrentDebugType(const char *Type) {
    CurrentDebugType.clear();
    CurrentDebugType.push_back(Type);
  }

};
