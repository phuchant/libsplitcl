#include "KernelAnalysis.h"

#include <cassert>
#include <iostream>
#include <sstream>

#include <string.h>

KernelAnalysis::KernelAnalysis(const char *name,
			       unsigned numArgs,
			       std::vector<size_t> &argsSizes,
			       std::vector<ArgumentAnalysis *> argsAnalysis,
			       std::vector<ArgIndirectionRegionExpr *>
			       kernelIndirectionExprs)
  : numArgs(numArgs), numGlobalArgs(0), argsSizes(argsSizes),
    kernelNDRange(NULL), subNDRanges(NULL) {
  mName = strdup(name);

  for (unsigned i=0; i<numArgs; i++)
    argIsGlobalMap[i] = false;

  for (unsigned i=0; i<argsAnalysis.size(); i++) {
    numGlobalArgs++;
    unsigned pos = argsAnalysis[i]->getPos();
    globalArg2PosMap[i] = pos;
    argPos2GlobalId[pos] = i;
    argIsGlobalMap[argsAnalysis[i]->getPos()] = true;
    mArgsAnalysis.push_back(argsAnalysis[i]);
  }

  for (unsigned i=0; i<kernelIndirectionExprs.size(); i++)
    this->kernelIndirectionExprs.push_back(kernelIndirectionExprs[i]);
}

KernelAnalysis::~KernelAnalysis() {
  free(mName);
  for (unsigned i=0; i<mArgsAnalysis.size(); i++)
    delete mArgsAnalysis[i];

  for (unsigned i=0; i<kernelIndirectionExprs.size(); i++)
    delete kernelIndirectionExprs[i];

  for (unsigned i=0; i<subKernelIndirectionRegions.size(); i++) {
    for (unsigned j=0; j<subKernelIndirectionRegions[i].size(); j++) {
      delete subKernelIndirectionRegions[i][j];
    }
  }

  delete kernelNDRange;
  delete subNDRanges;
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

unsigned
KernelAnalysis::getGlobalArgId(unsigned pos) {
  assert(argIsGlobal(pos));
  return argPos2GlobalId[pos];
}

void
KernelAnalysis::setPartition(const NDRange &kernelNDRange,
			     const std::vector<NDRange> &subNDRanges,
			     const std::vector<int> &argValues) {
  delete this->kernelNDRange;
  this->kernelNDRange = new NDRange(kernelNDRange);
  delete this->subNDRanges;
  this->subNDRanges = new std::vector<NDRange>(subNDRanges);

  // For each global argument: set partition and inject argValues
  for (unsigned i=0; i<numGlobalArgs; i++) {
    mArgsAnalysis[i]->setPartition(this->kernelNDRange, this->subNDRanges);
    mArgsAnalysis[i]->injectArgValues(argValues);
  }

  // For each indirection: inject argValues.
  for (unsigned i=0; i<kernelIndirectionExprs.size(); i++) {
    kernelIndirectionExprs[i]->expr->injectArgsValues(argValues,
						      *(this->kernelNDRange));
  }

  // Clear indirection regions.
  unsigned nbSplit = this->subNDRanges->size();
  for (unsigned i=0; i<subKernelIndirectionRegions.size(); i++) {
    for (unsigned j=0; j<subKernelIndirectionRegions[i].size(); j++) {
      delete subKernelIndirectionRegions[i][j];
    }

    subKernelIndirectionRegions[i].clear();
  }
  subKernelIndirectionRegions.clear();
  subKernelIndirectionRegions.resize(nbSplit);

  // Clear indirection values.
  for (unsigned i=0; i<subKernelIndirectionValues.size(); i++)
    subKernelIndirectionValues[i].clear();
  subKernelIndirectionValues.clear();
  subKernelIndirectionValues.resize(nbSplit);

  // Compute indirection regions.
  for (unsigned i=0; i<nbSplit; i++) {
    for (unsigned j=0; j<kernelIndirectionExprs.size(); j++) {
      unsigned id = kernelIndirectionExprs[j]->id;
      unsigned pos = kernelIndirectionExprs[j]->pos;
      unsigned cb = kernelIndirectionExprs[j]->numBytes;
      IndexExpr *expr =
	kernelIndirectionExprs[j]->expr
	->getKernelExpr((*this->subNDRanges)[i], subKernelIndirectionValues[i]);

      long lb, hb;
      if (!IndexExpr::computeBounds(expr, &lb, &hb)) {
	delete expr;
	continue;
      }

      hb = hb + 1 - cb;
      subKernelIndirectionRegions[i].push_back(new ArgIndirectionRegion(id,
									pos,
									cb,
									lb,
									hb));
    }
  }
}

const NDRange &
KernelAnalysis::getKernelNDRange() const {
  assert(kernelNDRange);
  return *kernelNDRange;
}

const std::vector<NDRange> &
KernelAnalysis::getSubNDRanges() const {
  assert(subNDRanges);
  return *subNDRanges;
}

bool
KernelAnalysis::hasIndirection() const {
  return kernelIndirectionExprs.size() > 0;
}

const std::vector<ArgIndirectionRegion *> &
KernelAnalysis::getSubkernelIndirectionsRegions(unsigned n) {
  return subKernelIndirectionRegions[n];
}

void
KernelAnalysis::setSubkernelIndirectionsValues(unsigned n,
					       const
					       std::vector<IndirectionValue> &
					       values) {
  subKernelIndirectionValues[n] = values;
}

bool
KernelAnalysis::performAnalysis() {
  bool ret = true;
  for (unsigned i = 0; i<mArgsAnalysis.size(); i++) {
    mArgsAnalysis[i]->performAnalysis(subKernelIndirectionValues);
    if (!mArgsAnalysis[i]->canSplit()) {
      std::cerr << "cannot split arg pos " << mArgsAnalysis[i]->getPos() << "\n";
      ret = false;
    }
  }

  return ret;
}

bool
KernelAnalysis::argReadBoundsComputed(unsigned argNo) const {
  return mArgsAnalysis[argNo]->readBoundsComputed();
}

bool
KernelAnalysis::argWrittenBoundsComputed(unsigned argNo) const {
  return mArgsAnalysis[argNo]->writeBoundsComputed();
}

bool
KernelAnalysis::argWrittenOrBoundsComputed(unsigned argNo) const {
  return mArgsAnalysis[argNo]->orBoundsComputed();
}

bool
KernelAnalysis::argWrittenAtomicSumBoundsComputed(unsigned argNo) const {
  return mArgsAnalysis[argNo]->atomicSumBoundsComputed();
}

bool
KernelAnalysis::argWrittenAtomicMaxBoundsComputed(unsigned argNo) const {
  return mArgsAnalysis[argNo]->atomicMaxBoundsComputed();
}

bool
KernelAnalysis::argIsReadBySubkernel(unsigned argNo, unsigned i) const {
  return mArgsAnalysis[argNo]->isReadBySubkernel(i);
}

bool
KernelAnalysis::argIsWrittenBySubkernel(unsigned argNo, unsigned i) const {
  return mArgsAnalysis[argNo]->isWrittenBySubkernel(i);
}

bool
KernelAnalysis::argIsWrittenOrBySubkernel(unsigned argNo, unsigned i) const {
  return mArgsAnalysis[argNo]->isWrittenOrBySubkernel(i);
}

bool
KernelAnalysis::argIsWrittenAtomicSumBySubkernel(unsigned argNo, unsigned i)
  const {
  return mArgsAnalysis[argNo]->isWrittenAtomicSumBySubkernel(i);
}

bool
KernelAnalysis::argIsWrittenAtomicMaxBySubkernel(unsigned argNo, unsigned i)
  const {
  return mArgsAnalysis[argNo]->isWrittenAtomicMaxBySubkernel(i);
}

const ListInterval &
KernelAnalysis::getArgReadSubkernelRegion(unsigned argNo, unsigned i) const {
  return mArgsAnalysis[argNo]->getReadSubkernelRegion(i);
}

const ListInterval &
KernelAnalysis::getArgWrittenSubkernelRegion(unsigned argNo, unsigned i) const {
  return mArgsAnalysis[argNo]->getWrittenSubkernelRegion(i);
}

const ListInterval &
KernelAnalysis::getArgWrittenOrSubkernelRegion(unsigned argNo, unsigned i) const
{
  return mArgsAnalysis[argNo]->getWrittenOrSubkernelRegion(i);
}

const ListInterval &
KernelAnalysis::getArgWrittenAtomicSumSubkernelRegion(unsigned argNo,
						      unsigned i) const {
  return mArgsAnalysis[argNo]->getWrittenAtomicSumSubkernelRegion(i);
}

const ListInterval &
KernelAnalysis::getArgWrittenAtomicMaxSubkernelRegion(unsigned argNo,
						      unsigned i) const {
  return mArgsAnalysis[argNo]->getWrittenAtomicMaxSubkernelRegion(i);
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

  // Write Indirection Expressions
  unsigned nbIndirection = kernelIndirectionExprs.size();
  s.write(reinterpret_cast<const char *>(&nbIndirection),
	  sizeof(nbIndirection));
  for (unsigned i=0; i<nbIndirection; i++) {
    unsigned id = kernelIndirectionExprs[i]->id;
    s.write(reinterpret_cast<const char *>(&id), sizeof(id));
    unsigned pos = kernelIndirectionExprs[i]->pos;
    s.write(reinterpret_cast<const char *>(&pos), sizeof(pos));
    unsigned numBytes = kernelIndirectionExprs[i]->numBytes;
    s.write(reinterpret_cast<const char *>(&numBytes), sizeof(numBytes));
    kernelIndirectionExprs[i]->expr->write(s);
  }
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
  unsigned nbIndirections;
  std::vector<ArgIndirectionRegionExpr *> kernelIndirectionExprs;

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

  s.read(reinterpret_cast<char *>(&nbIndirections), sizeof(nbIndirections));
  for (unsigned i=0; i<nbIndirections; i++) {
    unsigned id;
    s.read(reinterpret_cast<char *>(&id), sizeof(id));
    unsigned pos;
    s.read(reinterpret_cast<char *>(&pos), sizeof(pos));
    unsigned numBytes;
    s.read(reinterpret_cast<char *>(&numBytes), sizeof(numBytes));
    WorkItemExpr *expr = WorkItemExpr::open(s);
    kernelIndirectionExprs.push_back(new ArgIndirectionRegionExpr(id,
								  pos,
								  numBytes,
								  expr));
  }

  KernelAnalysis *ret =
    new KernelAnalysis(name, numArgs, argsSizes, argsAnalysis,
		       kernelIndirectionExprs);

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

  for (unsigned i=0; i<mArgsAnalysis.size(); i++) {
    mArgsAnalysis[i]->dump();
  }

  for (unsigned i=0; i<kernelIndirectionExprs.size(); i++) {
    std::cerr << "Indirection" << kernelIndirectionExprs[i]->id << ":\n"
	      << "arg pos " << kernelIndirectionExprs[i]->pos
	      << " cb " << kernelIndirectionExprs[i]->numBytes << "\n";
    kernelIndirectionExprs[i]->expr->dump();
  }
}
