#include "KernelAnalysis.h"

#include <iostream>
#include <sstream>

#include <string.h>

KernelAnalysis::KernelAnalysis(const char *name,
			       std::vector<ArgumentAnalysis *> argsAnalysis,
			       bool hasAtomicOrBarrier)
  : mHasAtomicOrBarrier(hasAtomicOrBarrier) {
  mName = strdup(name);

  for (unsigned i=0; i<argsAnalysis.size(); i++) {
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
  return mArgsAnalysis.size();
}

ArgumentAnalysis *
KernelAnalysis::getArgAnalysis(unsigned i) {
  return mArgsAnalysis[i];
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

  // Write Argument Analysis
  unsigned nbArgs = mArgsAnalysis.size();
  s.write(reinterpret_cast<const char *>(&nbArgs), sizeof(nbArgs));

  for (unsigned i=0; i<nbArgs; i++) {
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
  unsigned nbArgs;
  std::vector<ArgumentAnalysis *> argsAnalysis;
  bool hasAtomicOrBarrier;

  s.read((char *) &len, sizeof(len));
  name = (char *) malloc(len+1);
  s.read(name, len);
  name[len] = '\0';

  s.read(reinterpret_cast<char *>(&nbArgs), sizeof(nbArgs));
  for (unsigned i=0; i<nbArgs; i++) {
    argsAnalysis.push_back(ArgumentAnalysis::open(s));
  }

  s.read(reinterpret_cast<char *>(&hasAtomicOrBarrier),
  	 sizeof(hasAtomicOrBarrier));

  KernelAnalysis *ret =
    new KernelAnalysis(name, argsAnalysis, hasAtomicOrBarrier);

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

  std::cerr << mArgsAnalysis.size() << " arguments\n";

  if (mHasAtomicOrBarrier)
    std::cerr << "kernel has atomic instruction or global barrier\n";
  else
    std::cerr << "kernel does not have atomic instruction or global barrier\n";

  for (unsigned i=0; i<mArgsAnalysis.size(); i++) {
    mArgsAnalysis[i]->dump();
  }
}
