#include "KernelAnalysis.h"

#include <cassert>
#include <iostream>
#include <sstream>

#include <string.h>

#include <IndexExpr/IndexExprValue.h>

KernelAnalysis::KernelAnalysis(const char *name,
			       unsigned numArgs,
			       bool *scalarArgs,
			       std::vector<size_t> &scalarArgsSizes,
			       std::vector<ArgumentAnalysis::TYPE> &scalarArgsTypes,
			       std::vector<ArgumentAnalysis *> argsAnalysis,
			       std::vector<ArgIndirectionRegionExpr *>
			       kernelIndirectionExprs)
  : numArgs(numArgs), numGlobalArgs(0), scalarArgsSizes(scalarArgsSizes),
    scalarArgsTypes(scalarArgsTypes), kernelNDRange(NULL), subNDRanges(NULL) {
  mName = strdup(name);

  for (unsigned i=0; i<numArgs; i++)
    argIsScalarMap[i] = scalarArgs[i];

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

  if (kernelIndirectionExprs.size() > 0) {
    indirectionsComputed = new bool[kernelIndirectionExprs.size()];
  } else {
    indirectionsComputed = nullptr;
  }
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
  delete[] indirectionsComputed;

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
KernelAnalysis::argIsGlobal(unsigned i) const {
  assert(i < numArgs);
  auto it = argIsGlobalMap.find(i);
  if (it == argIsGlobalMap.end())
    return false;

  return it->second;
}

bool
KernelAnalysis::argIsScalar(unsigned i) const {
  assert(i < numArgs);
  auto it = argIsScalarMap.find(i);
  if (it == argIsScalarMap.end())
    return false;

  return it->second;
}

size_t
KernelAnalysis::getScalarArgSize(unsigned i) {
  assert(i < numArgs);
  return scalarArgsSizes[i];
}

ArgumentAnalysis::TYPE
KernelAnalysis::getGlobalArgType(unsigned i) {
  assert(i < numGlobalArgs);
  return mArgsAnalysis[i]->getType();
}

ArgumentAnalysis::TYPE
KernelAnalysis::getScalarArgType(unsigned i) {
  assert(i < numArgs);
  return scalarArgsTypes[i];
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
			     const std::vector<IndexExprValue *> &argValues) {
  delete this->kernelNDRange;
  this->kernelNDRange = new NDRange(kernelNDRange);
  delete this->subNDRanges;
  this->subNDRanges = new std::vector<NDRange>(subNDRanges);
  unsigned nbSplit = this->subNDRanges->size();

  // Clear computed set
  for (unsigned i=0; i<kernelIndirectionExprs.size(); i++)
    indirectionsComputed[i] = false;

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

  // Clear indirection values.
  for (unsigned i=0; i<subKernelIndirectionValues.size(); i++) {
    for (unsigned j=0; j<subKernelIndirectionValues[i].size(); j++) {
      delete subKernelIndirectionValues[i][j].lb;
      delete subKernelIndirectionValues[i][j].hb;
    }
    subKernelIndirectionValues[i].clear();
  }
  subKernelIndirectionValues.clear();
  subKernelIndirectionValues.resize(nbSplit);
}

void
KernelAnalysis::computeIndirections() {
  std::vector<unsigned> indirComputed;
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

  // Compute indirection regions.
  for (unsigned i=0; i<nbSplit; i++) {
    for (unsigned j=0; j<kernelIndirectionExprs.size(); j++) {
      unsigned id = kernelIndirectionExprs[j]->id;

      if (indirectionsComputed[id])
	continue;

      unsigned pos = kernelIndirectionExprs[j]->pos;
      IndirectionType ty = kernelIndirectionExprs[j]->ty;
      unsigned cb = kernelIndirectionExprs[j]->numBytes;
      IndexExpr *expr =
	kernelIndirectionExprs[j]->expr
	->getKernelExpr((*this->subNDRanges)[i], subKernelIndirectionValues[i]);

      long lb, hb;
      if (!IndexExpr::computeBounds(expr, &lb, &hb)) {
	delete expr;
	continue;
      }


      indirComputed.push_back(id);

      hb = hb + 1 - cb;
      subKernelIndirectionRegions[i].push_back(new ArgIndirectionRegion(id,
									pos,
									ty,
									cb,
									lb,
									hb));
      delete expr;
    }
  }

  for (unsigned i=0; i<indirComputed.size(); i++)
    indirectionsComputed[indirComputed[i]] = true;
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

bool
KernelAnalysis::indirectionMissing() const {
  for (unsigned i=0; i<kernelIndirectionExprs.size(); i++) {
    if (!indirectionsComputed[i])
      return true;
  }

  return false;
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
  std::vector<int> toErase;

  for (unsigned i=0; i<subKernelIndirectionValues[n].size(); i++) {
    for (unsigned j=0; j<values.size(); j++) {
      if (subKernelIndirectionValues[n][i].id == values[j].id) {
	toErase.push_back(i);
	delete subKernelIndirectionValues[n][i].lb;
	delete subKernelIndirectionValues[n][i].hb;
      }
    }
  }

  for (unsigned i=0; i<toErase.size(); i++)
    subKernelIndirectionValues[n].erase(subKernelIndirectionValues[n].begin() + toErase[i]);

  subKernelIndirectionValues[n].insert(subKernelIndirectionValues[n].end(),
				       values.begin(), values.end());
}

ArgumentAnalysis::status
KernelAnalysis::performAnalysis() {
  enum ArgumentAnalysis::status ret = ArgumentAnalysis::SUCCESS;
  mergeArguments.clear();

  for (unsigned i = 0; i<mArgsAnalysis.size(); i++) {
    switch (mArgsAnalysis[i]->performAnalysis(subKernelIndirectionValues)) {
    case ArgumentAnalysis::SUCCESS:
      continue;

    case ArgumentAnalysis::MERGE:
      mergeArguments.push_back(i);
      ret = ret == ArgumentAnalysis::FAIL ?
	ArgumentAnalysis::FAIL : ArgumentAnalysis::MERGE;
      continue;

    case ArgumentAnalysis::FAIL:
      ret = ArgumentAnalysis::FAIL;
    };
  }

  return ret;
}

unsigned
KernelAnalysis::getNbMergeArguments() const {
  return mergeArguments.size();
}

unsigned
KernelAnalysis::getMergeArgGlobalPos(unsigned mergeNo) const {
  assert(mergeNo < mergeArguments.size());
  return mergeArguments[mergeNo];
}


ListInterval *
KernelAnalysis::getArgFullWrittenRegion(unsigned argNo) const {
  ListInterval *ret = new ListInterval();
  ret->myUnion(mArgsAnalysis[argNo]->getWrittenMergeRegion());
  assert(subNDRanges);
  for (unsigned i=0; i<subNDRanges->size(); i++)
    ret->myUnion(mArgsAnalysis[argNo]->getWrittenSubkernelRegion(i));
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
KernelAnalysis::argWrittenAtomicMinBoundsComputed(unsigned argNo) const {
  return mArgsAnalysis[argNo]->atomicMinBoundsComputed();
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
KernelAnalysis::argIsWrittenAtomicMinBySubkernel(unsigned argNo, unsigned i)
  const {
  return mArgsAnalysis[argNo]->isWrittenAtomicMinBySubkernel(i);
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

const ListInterval &
KernelAnalysis::getArgWrittenAtomicMinSubkernelRegion(unsigned argNo,
						      unsigned i) const {
  return mArgsAnalysis[argNo]->getWrittenAtomicMinSubkernelRegion(i);
}

const ListInterval &
KernelAnalysis::getArgWrittenMergeRegion(unsigned argNo) const {
  return mArgsAnalysis[argNo]->getWrittenMergeRegion();
}

void
KernelAnalysis::write(std::stringstream &s) const {
  // Write name
  unsigned len = strlen(mName);
  s.write((char *) &len, sizeof(len));
  s.write(mName, len);

  // Write num args
  s.write(reinterpret_cast<const char *>(&numArgs), sizeof(numArgs));

  // Write scalar arg map
  for (unsigned i=0; i<numArgs; i++) {
    bool isScalar = argIsScalar(i);
    s.write(reinterpret_cast<const char *>(&isScalar), sizeof(isScalar));
  }

  // Write scalar arg sizes
  for (unsigned i=0; i<numArgs; i++)
    s.write(reinterpret_cast<const char *>(&scalarArgsSizes[i]),
	    sizeof(scalarArgsSizes[i]));

  // Write scalar arg types.
  for (unsigned i=0; i<numArgs; i++)
    s.write(reinterpret_cast<const char *>(&scalarArgsTypes[i]),
	    sizeof(scalarArgsTypes[i]));

  // Write num global args
  s.write(reinterpret_cast<const char *>(&numGlobalArgs),
	  sizeof(numGlobalArgs));

  // Write Global Arguments Analyses
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
    IndirectionType ty = kernelIndirectionExprs[i]->ty;
    s.write(reinterpret_cast<const char *>(&ty), sizeof(ty));
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
  std::vector<ArgumentAnalysis::TYPE> argsTypes;
  unsigned numGlobalArgs;
  std::vector<ArgumentAnalysis *> argsAnalysis;
  unsigned nbIndirections;
  std::vector<ArgIndirectionRegionExpr *> kernelIndirectionExprs;

  // Read name
  s.read((char *) &len, sizeof(len));
  name = (char *) malloc(len+1);
  s.read(name, len);
  name[len] = '\0';

  // Read num args
  s.read(reinterpret_cast<char *>(&numArgs), sizeof(numArgs));

  // Read scalar arg map
  bool isScalarArray[numArgs];
  for (unsigned i=0; i<numArgs; i++) {
    s.read(reinterpret_cast<char *>(&isScalarArray[i]),
	   sizeof(isScalarArray[i]));
  }

  // Read scalar arg sizes
  for (unsigned i=0; i<numArgs; i++) {
    size_t size;
    s.read(reinterpret_cast<char *>(&size), sizeof(size));
    argsSizes.push_back(size);
  }

  // Read scalar arg types.
  for (unsigned i=0; i<numArgs; i++) {
    ArgumentAnalysis::TYPE ty;
    s.read(reinterpret_cast<char *>(&ty), sizeof(ty));
    argsTypes.push_back(ty);
  }

  // Read num global args
  s.read(reinterpret_cast<char *>(&numGlobalArgs), sizeof(numGlobalArgs));

  // Read global arg analyses
  for (unsigned i=0; i<numGlobalArgs; i++) {
    argsAnalysis.push_back(ArgumentAnalysis::open(s));
  }

  // Read indirection expressions.
  s.read(reinterpret_cast<char *>(&nbIndirections), sizeof(nbIndirections));
  for (unsigned i=0; i<nbIndirections; i++) {
    unsigned id;
    s.read(reinterpret_cast<char *>(&id), sizeof(id));
    unsigned pos;
    s.read(reinterpret_cast<char *>(&pos), sizeof(pos));
    unsigned numBytes;
    s.read(reinterpret_cast<char *>(&numBytes), sizeof(numBytes));
    IndirectionType ty;
    s.read(reinterpret_cast<char *>(&ty), sizeof(ty));
    WorkItemExpr *expr = WorkItemExpr::open(s);
    kernelIndirectionExprs.push_back(new ArgIndirectionRegionExpr(id,
								  pos,
								  numBytes,
								  ty,
								  expr));
  }

  KernelAnalysis *ret =
    new KernelAnalysis(name, numArgs, isScalarArray, argsSizes, argsTypes,
		       argsAnalysis,
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

  for (unsigned i=0; i<numArgs; i++) {
    if (argIsGlobal(i)) {
      mArgsAnalysis[getGlobalArgId(i)]->dump();
    }

    else if (argIsScalar(i)) {
      std::cerr << "\n\033[1;31m*** Arg no \"" << i << "\""
		<< " is scalar, type=";
      switch(getScalarArgType(i)) {
      case ArgumentAnalysis::BOOL:
	std::cerr << "bool";
	break;
      case ArgumentAnalysis::CHAR:
	std::cerr << "char";
	break;
      case ArgumentAnalysis::UCHAR:
	std::cerr << "uchar";
	break;
      case ArgumentAnalysis::SHORT:
	std::cerr << "short";
	break;
      case ArgumentAnalysis::USHORT:
	std::cerr << "ushort";
	break;
      case ArgumentAnalysis::INT:
	std::cerr << "int";
	break;
      case ArgumentAnalysis::UINT:
	std::cerr << "uint";
	break;
      case ArgumentAnalysis::LONG:
	std::cerr << "long";
	break;
      case ArgumentAnalysis::ULONG:
	std::cerr << "ulong";
	break;
      case ArgumentAnalysis::FLOAT:
	std::cerr << "float";
	break;
      case ArgumentAnalysis::DOUBLE:
	std::cerr << "double";
	break;
      case ArgumentAnalysis::UNKNOWN:
	std::cerr << "unknown";
	break;
      };

      std::cerr << " size=" << getScalarArgSize(i) << "\033[0m\n";
    }

    else {
      std::cerr << "\n\033[1;31m*** Arg no \"" << i
		<< "\033[0m is local\n";
    }
  }

  for (unsigned i=0; i<kernelIndirectionExprs.size(); i++) {
    std::cerr << "Indirection" << kernelIndirectionExprs[i]->id << ":\n"
	      << "arg pos " << kernelIndirectionExprs[i]->pos
	      << " cb " << kernelIndirectionExprs[i]->numBytes << "\n";
    kernelIndirectionExprs[i]->expr->dump();
  }
}
