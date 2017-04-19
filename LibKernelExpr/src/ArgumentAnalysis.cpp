#include "ArgumentAnalysis.h"

#include "IndexExpr/IndexExprs.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>

ArgumentAnalysis::ArgumentAnalysis(unsigned pos, unsigned sizeInBytes,
				   const std::vector<WorkItemExpr *> &loadExprs,
				   const std::vector<WorkItemExpr *>
				   &storeExprs)
  : nbSplit(0), pos(pos), sizeInBytes(sizeInBytes),
    mReadBoundsComputed(false), mWriteBoundsComputed(false), areDisjoint(false)
{
  loadWorkItemExprs = new std::vector<WorkItemExpr *>();
  storeWorkItemExprs = new std::vector<WorkItemExpr *>();

  for (unsigned i=0; i<loadExprs.size(); i++)
    loadWorkItemExprs->push_back(loadExprs[i]->clone());
  for (unsigned i=0; i<storeExprs.size(); i++)
    storeWorkItemExprs->push_back(storeExprs[i]->clone());
}

ArgumentAnalysis::ArgumentAnalysis(unsigned pos, unsigned sizeInBytes,
				   std::vector<WorkItemExpr *> *loadExprs,
				   std::vector<WorkItemExpr *> *storeExprs)
  : nbSplit(0), pos(pos), sizeInBytes(sizeInBytes),
    loadWorkItemExprs(loadExprs), storeWorkItemExprs(storeExprs),
    mReadBoundsComputed(false), mWriteBoundsComputed(false), areDisjoint(false)
{
}

ArgumentAnalysis::~ArgumentAnalysis() {
  for (unsigned i=0; i<loadWorkItemExprs->size(); ++i)
    delete (*loadWorkItemExprs)[i];
  delete loadWorkItemExprs;
  for (unsigned i=0; i<storeWorkItemExprs->size(); ++i)
    delete (*storeWorkItemExprs)[i];
  delete storeWorkItemExprs;

  for (unsigned i=0; i<loadKernelExprs.size(); ++i)
    delete loadKernelExprs[i];
  for (unsigned i=0; i<storeKernelExprs.size(); ++i)
    delete storeKernelExprs[i];

  for (unsigned i=0; i<loadSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<loadSubKernelsExprs[i].size(); ++j) {
      delete loadSubKernelsExprs[i][j];
    }
  }
  for (unsigned i=0; i<storeSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<storeSubKernelsExprs[i].size(); ++j) {
      delete storeSubKernelsExprs[i][j];
    }
  }
}

void
ArgumentAnalysis::dump() {
  std::cerr << "\n\033[1;31m*** Arg no \"" << pos
	    << "\" nb load: " << loadWorkItemExprs->size()
	    << "nb store " << storeWorkItemExprs->size() << "\033[0m\n";

  std::cerr << "size in bytes : " << sizeInBytes << "\n";

  for (unsigned i=0; i<loadWorkItemExprs->size(); ++i) {
      std::cerr << "\033[1mLoad work item expression : \033[0m\n";
      (*loadWorkItemExprs)[i]->dump();
      std::cerr << "\n";
  }

  for (unsigned i=0; i<storeWorkItemExprs->size(); ++i) {
      std::cerr << "\033[1mStore work item expression : \033[0m\n";
      (*storeWorkItemExprs)[i]->dump();
      std::cerr << "\n";
  }

  if (mReadBoundsComputed) {
    std::cerr << "\033[1mRead kernel region :\033[0m\n";
    readKernelRegions.debug();
    std::cerr << "\n";

    for (unsigned i=0; i<nbSplit; i++) {
      std::cerr << "read subkernel " << i << " region : ";
      readSubkernelsRegions[i].debug();
      std::cerr << "\n";
    }
  } else {
    std::cerr << "\033[1mRead region not computed\033[0m\n";
  }

  if (mWriteBoundsComputed) {
    std::cerr << "\033[1mWritten kernel region :\033[0m\n";
    writtenKernelRegions.debug();
    std::cerr << "\n";

    for (unsigned i=0; i<nbSplit; i++) {
      std::cerr << "written subkernel " << i << " region : ";
      writtenSubkernelsRegions[i].debug();
      std::cerr << "\n";
    }
  } else {
    std::cerr << "\033[1mWritten region not computed\033[0m\n";
  }

  if (areDisjoint) {
    std::cerr << "\033[1mSubkernels kernels are disjoint !"
  	      << "\033[0m\n";
  } else {
    std::cerr << "\033[1mSubkernels are not disjoint !"
  	      << "\033[0m\n";
  }
}

void
ArgumentAnalysis::performAnalysis(const std::vector<int> &args,
				  const NDRange &ndRange,
				  const std::vector<NDRange> &splitNDRanges) {
#ifdef DEBUG
  std::cerr << "\n\033[1;31m*** Arg no \"" << m_pos << "\" nb load: "
  	    << loadWorkItemExprs->size()
	    << " nb store: " << storeWorkItemExprs->size() << "\033[0m\n";

  for (unsigned i=0; i<splitNDRanges.size(); ++i) {
    std::cerr << "splitrange no " << i << "\n";
    splitNDRanges[i].dump();
    std::cerr << "\n";
  }
#endif

  // Clear m_kernelExprs and m_subKernelsExprs.
  for (unsigned i=0; i<loadKernelExprs.size(); ++i)
    delete loadKernelExprs[i];
  loadKernelExprs.resize(0);
  for (unsigned i=0; i<storeKernelExprs.size(); ++i)
    delete storeKernelExprs[i];
  storeKernelExprs.resize(0);

  for (unsigned i=0; i<loadSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<loadSubKernelsExprs[i].size(); ++j) {
      delete loadSubKernelsExprs[i][j];
    }

    loadSubKernelsExprs[i].resize(0);
  }
  for (unsigned i=0; i<storeSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<storeSubKernelsExprs[i].size(); ++j) {
      delete storeSubKernelsExprs[i][j];
    }

    storeSubKernelsExprs[i].resize(0);
  }

  nbSplit = splitNDRanges.size();

  // Vector of size nbsplit, each element containing of vector of
  // IndexExpr (one indexExpr per use)
  //  std::vector<std::vector<IndexExpr *> > kernelSplitExprs(m_nbSplit);
  loadSubKernelsExprs.resize(nbSplit);
  storeSubKernelsExprs.resize(nbSplit);

  // Clear regions
  readKernelRegions.clear();
  writtenKernelRegions.clear();
  readSubkernelsRegions.clear();
  readSubkernelsRegions.resize(nbSplit);
  writtenSubkernelsRegions.clear();
  writtenSubkernelsRegions.resize(nbSplit);

  // Build load kernel and subkernel expressions
  for (unsigned idx=0; idx<loadWorkItemExprs->size(); idx++) {

    // Inject arguments values into expression
    (*loadWorkItemExprs)[idx]->injectArgsValues(args, ndRange);

    // Build kernel expression (ndRange)
    IndexExpr *kernelExpr = (*loadWorkItemExprs)[idx]->getKernelExpr(ndRange);

    // NULL if kernelexpr is out of guards
    if (kernelExpr)
      loadKernelExprs.push_back(kernelExpr);

    // Build subkernel expressions
    for (unsigned i=0; i<nbSplit; ++i) {
      IndexExpr *splitExpr =
	(*loadWorkItemExprs)[idx]->getKernelExpr(splitNDRanges[i]);

      // NULL if kernelexpr is out of guards
      if (splitExpr)
	loadSubKernelsExprs[i].push_back(splitExpr);
    }
  }

  // Build store kernel and subkernel expressions
  for (unsigned idx=0; idx<storeWorkItemExprs->size(); idx++) {

    // Inject arguments values into expression
    (*storeWorkItemExprs)[idx]->injectArgsValues(args, ndRange);

    // Build kernel expression (ndRange)
    IndexExpr *kernelExpr = (*storeWorkItemExprs)[idx]->getKernelExpr(ndRange);

    // NULL if kernelexpr is out of guards
    if (kernelExpr)
      storeKernelExprs.push_back(kernelExpr);

    // Build subkernel expressions
    for (unsigned i=0; i<nbSplit; ++i) {
      IndexExpr *splitExpr =
	(*storeWorkItemExprs)[idx]->getKernelExpr(splitNDRanges[i]);

      // NULL if kernelexpr is out of guards
      if (splitExpr)
	storeSubKernelsExprs[i].push_back(splitExpr);
    }
  }

  // Compute subkernels bounds
  computeRegions();

  if (!mWriteBoundsComputed)
    return;

  // Find out if subkernels region are disjoint
  performDisjointTest();
}

bool
ArgumentAnalysis::canSplit() const {
  // We can split if the argument is read-only or not used or
  // if each split kernel access disjoint regions of the argument
  return !isWritten() || (mWriteBoundsComputed && areDisjoint);
}

bool
ArgumentAnalysis::isRead() const {
  return loadWorkItemExprs->size() > 0;
}

bool
ArgumentAnalysis::isWritten() const {
  return storeWorkItemExprs->size() > 0;
}

bool
ArgumentAnalysis::isReadBySplitNo(unsigned i) const {
  return loadSubKernelsExprs[i].size() > 0;
}

bool
ArgumentAnalysis::isWrittenBySplitNo(unsigned i) const {
  return storeSubKernelsExprs[i].size() > 0;
}


unsigned
ArgumentAnalysis::getNumLoad() const {
  return loadWorkItemExprs->size();
}

unsigned
ArgumentAnalysis::getNumStore() const {
  return storeWorkItemExprs->size();
}

void
ArgumentAnalysis::write(std::stringstream &s) const {
  s.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
  s.write(reinterpret_cast<const char *>(&sizeInBytes), sizeof(sizeInBytes));
  unsigned nbLoad = loadWorkItemExprs->size();
  s.write(reinterpret_cast<const char*>(&nbLoad), sizeof(nbLoad));
  for (unsigned i=0; i<nbLoad; ++i)
    (*loadWorkItemExprs)[i]->write(s);
  unsigned nbStore = storeWorkItemExprs->size();
  s.write(reinterpret_cast<const char*>(&nbStore), sizeof(nbStore));
  for (unsigned i=0; i<nbStore; ++i)
    (*storeWorkItemExprs)[i]->write(s);
}

void
ArgumentAnalysis::writeToFile(const std::string &name) const {
  std::ofstream out(name.c_str(), std::ofstream::trunc | std::ofstream::binary);
  std::stringstream ss;
  write(ss);
  out << ss;
  out.close();
}

ArgumentAnalysis *
ArgumentAnalysis::open(std::stringstream &s) {
  unsigned pos;
  s.read(reinterpret_cast<char *>(&pos), sizeof(pos));
  unsigned sizeInBytes;
  s.read(reinterpret_cast<char *>(&sizeInBytes), sizeof(sizeInBytes));

  unsigned nbLoad;
  s.read(reinterpret_cast<char *>(&nbLoad), sizeof(nbLoad));
  std::vector<WorkItemExpr *> *loadExprs =
    new std::vector<WorkItemExpr *>();
  for (unsigned i=0; i<nbLoad; ++i)
    loadExprs->push_back(WorkItemExpr::open(s));

  unsigned nbStore;
  s.read(reinterpret_cast<char *>(&nbStore), sizeof(nbStore));
  std::vector<WorkItemExpr *> *storeExprs =
    new std::vector<WorkItemExpr *>();
  for (unsigned i=0; i<nbStore; ++i)
    storeExprs->push_back(WorkItemExpr::open(s));

  return new ArgumentAnalysis(pos, sizeInBytes, loadExprs, storeExprs);
}

ArgumentAnalysis *
ArgumentAnalysis::openFromFile(const std::string &name) {
  std::ifstream in(name.c_str(), std::ifstream::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  in.close();
  return open(ss);
}

unsigned
ArgumentAnalysis::getPos() const {
  return pos;
}

const ListInterval &
ArgumentAnalysis::getReadKernelRegion() const {
  return readKernelRegions;
}

const ListInterval &
ArgumentAnalysis::getWrittenKernelRegion() const {
  return writtenKernelRegions;
}

const ListInterval &
ArgumentAnalysis::getReadSubkernelRegion(unsigned i) const {
  return readSubkernelsRegions[i];
}

const ListInterval &
ArgumentAnalysis::getWrittenSubkernelRegion(unsigned i) const {
  return writtenSubkernelsRegions[i];
}

unsigned
ArgumentAnalysis::getSizeInBytes() const {
  return sizeInBytes;
}

const WorkItemExpr *
ArgumentAnalysis::getLoadWorkItemExpr(unsigned n) const {
  return (*loadWorkItemExprs)[n];
}

const WorkItemExpr *
ArgumentAnalysis::getStoreWorkItemExpr(unsigned n) const {
  return (*storeWorkItemExprs)[n];
}

const IndexExpr *
ArgumentAnalysis::getLoadKernelExpr(unsigned n) const {
  return loadKernelExprs[n];
}

const IndexExpr *
ArgumentAnalysis::getStoreKernelExpr(unsigned n) const {
  return storeKernelExprs[n];
}

const IndexExpr *
ArgumentAnalysis::getLoadSubkernelExpr(unsigned splitno, unsigned useno) const {
  return loadSubKernelsExprs[splitno][useno];
}

const IndexExpr *
ArgumentAnalysis::getStoreSubkernelExpr(unsigned splitno, unsigned useno) const {
  return storeSubKernelsExprs[splitno][useno];
}

void
ArgumentAnalysis::computeRegions() {
  mReadBoundsComputed = true;
  mWriteBoundsComputed = true;


  // Compute read kernel region
  readKernelRegions.clear();
  for (unsigned i=0; i<loadKernelExprs.size(); ++i) {
    long lb, hb;
    if (!IndexExpr::computeBounds(loadKernelExprs[i], &lb, &hb)) {
      mReadBoundsComputed = false;
      break;
    }

    lb = lb < 0 ? 0 : lb;
    assert(lb <= hb);

    readKernelRegions.add(Interval(lb, hb));
  }

  // Compute written kernel region
  writtenKernelRegions.clear();
  for (unsigned i=0; i<storeKernelExprs.size(); ++i) {
    long lb, hb;
    if (!IndexExpr::computeBounds(storeKernelExprs[i], &lb, &hb)) {
      mWriteBoundsComputed = false;
      break;
    }

    lb = lb < 0 ? 0 : lb;
    assert(lb <= hb);

    writtenKernelRegions.add(Interval(lb, hb));
  }

  // Compute read subkernels regions
  for (unsigned i=0; i<loadSubKernelsExprs.size(); ++i) {
    readSubkernelsRegions[i].clear();

    for (unsigned j=0; j<loadSubKernelsExprs[i].size(); ++j) {
      long lb, hb;
      if (!IndexExpr::computeBounds(loadSubKernelsExprs[i][j], &lb, &hb)) {
	mReadBoundsComputed = false;
	break;
      }

      lb = lb < 0 ? 0 : lb;
      assert(lb <= hb);

      readSubkernelsRegions[i].add(Interval(lb, hb));
    }
  }

  // Compute written subkernels regions
  for (unsigned i=0; i<storeSubKernelsExprs.size(); ++i) {
    writtenSubkernelsRegions[i].clear();

    for (unsigned j=0; j<storeSubKernelsExprs[i].size(); ++j) {
      long lb, hb;
      if (!IndexExpr::computeBounds(storeSubKernelsExprs[i][j], &lb, &hb)) {
	mWriteBoundsComputed = false;
	break;
      }

      lb = lb < 0 ? 0 : lb;
      assert(lb <= hb);

      writtenSubkernelsRegions[i].add(Interval(lb, hb));
    }
  }
}

void ArgumentAnalysis::performDisjointTest() {
  areDisjoint = true;

  // If the argument is n ot used, set areDisjoint to true.
  if (!isWritten())
    return;

  for (unsigned i=0; i<nbSplit -1; ++i) {
    if (!isWrittenBySplitNo(i))
      continue;

    for (unsigned j=i+1; j<nbSplit ; ++j) {
      if (!isWrittenBySplitNo(j))
	continue;

      ListInterval *inter =
	ListInterval::intersection(writtenSubkernelsRegions[i],
				   writtenSubkernelsRegions[j]);
      if (!inter->mList.empty()) {
	areDisjoint = false;
	delete inter;
	return;
      }

      delete inter;
    }
  }
}

bool
ArgumentAnalysis::readBoundsComputed() const {
  return mReadBoundsComputed;
}

bool
ArgumentAnalysis::writeBoundsComputed() const {
  return mWriteBoundsComputed;
}
