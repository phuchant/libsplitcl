#include "KernelAnalysis.h"

#include <cassert>
#include <iostream>
#include <sstream>

#include <string.h>

KernelAnalysis::KernelAnalysis(const char *name,
			       unsigned numArgs,
			       std::vector<size_t> argsSizes,
			       std::vector<ArgumentAnalysis *> argsAnalysis,
			       bool hasAtomicOrBarrier)
  : numArgs(numArgs), numGlobalArgs(0), argsSizes(argsSizes),
    mHasAtomicOrBarrier(hasAtomicOrBarrier) {
  mName = strdup(name);

  for (unsigned i=0; i<numArgs; i++)
    argIsGlobalMap[i] = false;

  for (unsigned i=0; i<argsAnalysis.size(); i++) {
    numGlobalArgs++;
    globalArg2PosMap[i] = argsAnalysis[i]->getPos();
    argIsGlobalMap[argsAnalysis[i]->getPos()] = true;
    mArgsAnalysis.push_back(argsAnalysis[i]);
  }
}

KernelAnalysis::~KernelAnalysis() {
  free(mName);
  for (unsigned i=0; i<mArgsAnalysis.size(); i++) {
    delete mArgsAnalysis[i];
  }
}

const char *
KernelAnalysis::getName() const {
  return mName;
}

unsigned
KernelAnalysis::getNbArguments() const {
  return numArgs;
}

unsigned
KernelAnalysis::getNbGlobalArguments() const {
  return numGlobalArgs;
}

ArgumentAnalysis *
KernelAnalysis::getGlobalArgAnalysis(unsigned i) {
  return mArgsAnalysis[i];
}

bool
KernelAnalysis::argIsGlobal(unsigned i) {
  assert(i < numArgs);
  return argIsGlobalMap[i];
}

size_t
KernelAnalysis::getArgSize(unsigned i) {
  assert(i < numArgs);
  return argsSizes[i];
}

ArgumentAnalysis::TYPE
KernelAnalysis::getArgType(unsigned i) {
  assert(i < numArgs);
  return mArgsAnalysis[i]->getType();
}

unsigned
KernelAnalysis::getGlobalArgPos(unsigned i) {
  assert(i < numGlobalArgs);
  return globalArg2PosMap[i];
}

bool
KernelAnalysis::hasAtomicOrBarrier() const {
  return mHasAtomicOrBarrier;
}

void
KernelAnalysis::write(std::stringstream &s) const {
  // Write name
  unsigned len = strlen(mName);
  s.write((char *) &len, sizeof(len));
  s.write(mName, len);

  // Write Arguments sizes
  s.write(reinterpret_cast<const char *>(&numArgs), sizeof(numArgs));
  for (unsigned i=0; i<numArgs; i++)
    s.write(reinterpret_cast<const char *>(&argsSizes[i]),
	    sizeof(argsSizes[i]));

  // Write Global Arguments Analyses
  s.write(reinterpret_cast<const char *>(&numGlobalArgs),
	  sizeof(numGlobalArgs));

  for (unsigned i=0; i<numGlobalArgs; i++) {
    mArgsAnalysis[i]->write(s);
  }

  // Write hasAtomicOrBarrier
  s.write(reinterpret_cast<const char *>(&mHasAtomicOrBarrier),
  	  sizeof(mHasAtomicOrBarrier));
}

void
KernelAnalysis::writeToFile(const std::string &name) const {
  std::ofstream out(name.c_str(), std::ios::trunc | std::ios::binary |
		    std::ios::out);
  std::stringstream ss;
  write(ss);
  std::string final = ss.str();
  out.write(final.c_str(), final.size());
  out.close();
}

KernelAnalysis *
KernelAnalysis::open(std::stringstream &s) {
  unsigned len;
  char *name;
  unsigned numArgs;
  std::vector<size_t> argsSizes;
  unsigned numGlobalArgs;
  std::vector<ArgumentAnalysis *> argsAnalysis;
  bool hasAtomicOrBarrier;

  s.read((char *) &len, sizeof(len));
  name = (char *) malloc(len+1);
  s.read(name, len);
  name[len] = '\0';

  s.read(reinterpret_cast<char *>(&numArgs), sizeof(numArgs));
  for (unsigned i=0; i<numArgs; i++) {
    size_t size;
    s.read(reinterpret_cast<char *>(&size), sizeof(size));
    argsSizes.push_back(size);
  }

  s.read(reinterpret_cast<char *>(&numGlobalArgs), sizeof(numGlobalArgs));
  for (unsigned i=0; i<numGlobalArgs; i++) {
    argsAnalysis.push_back(ArgumentAnalysis::open(s));
  }

  s.read(reinterpret_cast<char *>(&hasAtomicOrBarrier),
  	 sizeof(hasAtomicOrBarrier));

  KernelAnalysis *ret =
    new KernelAnalysis(name, numArgs, argsSizes, argsAnalysis,
		       hasAtomicOrBarrier);

  free(name);

  return ret;
}

KernelAnalysis *
KernelAnalysis::openFromFile(const std::string &name) {
  std::ifstream in(name.c_str(), std::ios::in | std::ios::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  in.close();

  return open(ss);
}

void
KernelAnalysis::debug() {
  std::cerr << "KernelAnalysis " << mName << "\n";

  std::cerr << numArgs << " arguments, " << numGlobalArgs << " of which are "
	    << " global.\n";

  if (mHasAtomicOrBarrier)
    std::cerr << "kernel has atomic instruction or global barrier\n";
  else
    std::cerr << "kernel does not have atomic instruction or global barrier\n";

  for (unsigned i=0; i<mArgsAnalysis.size(); i++) {
    mArgsAnalysis[i]->dump();
  }
}
