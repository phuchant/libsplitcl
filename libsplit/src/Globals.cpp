#include <BufferManagerOptim.h>
#include <BufferManagerSimple.h>
#include "Globals.h"
#include <Options.h>
#include "Logger.h"

BufferManager *bufferMgr = NULL;
Logger *logger = NULL;

void initGlobals()
{
  if (optBuffMgr == BufferManager::SIMPLE)
    bufferMgr = new BufferManagerSimple();
  else
    bufferMgr = new BufferManagerOptim();
}
