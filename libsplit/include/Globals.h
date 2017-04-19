#ifndef GLOBALS_H
#define GLOBALS_H

#include "BufferManager.h"
#include "Logger.h"

#include <vector>
#include <string>

extern bool DebugFlag;
extern std::vector<std::string> CurrentDebugType;

extern BufferManager *bufferMgr;
extern Logger *logger;

void initGlobals();

#endif /* GLOBALS_H */
