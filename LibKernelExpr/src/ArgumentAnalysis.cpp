
#include "ArgumentAnalysis.h"

#include "IndexExpr/IndexExprs.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>

ArgumentAnalysis::ArgumentAnalysis(unsigned pos, TYPE type,
				   unsigned sizeInBytes,
				   const std::vector<WorkItemExpr *> &loadExprs,
				   const std::vector<WorkItemExpr *>
				   &storeExprs,
				   const std::vector<WorkItemExpr *> &orExprs,
				   const std::vector<WorkItemExpr *>
				   &atomicSumExprs,
				   const std::vector<WorkItemExpr *>
				   &atomicMinExprs,
				   const std::vector<WorkItemExpr *>
				   &atomicMaxExprs)
  : nbSplit(0), pos(pos), type(type), sizeInBytes(sizeInBytes),
    kernelNDRange(NULL), subNDRanges(NULL),
    mReadBoundsComputed(false), mWriteBoundsComputed(false),
    mOrBoundsComputed(false), mAtomicSumBoundsComputed(false),
    mAtomicMinBoundsComputed(false), mAtomicMaxBoundsComputed(false),
    areDisjoint(false), analysisHasBeenRun(false)
{
  loadWorkItemExprs = new std::vector<WorkItemExpr *>();
  storeWorkItemExprs = new std::vector<WorkItemExpr *>();
  orWorkItemExprs = new std::vector<WorkItemExpr *>();
  atomicSumWorkItemExprs = new std::vector<WorkItemExpr *>();
  atomicMinWorkItemExprs = new std::vector<WorkItemExpr *>();
  atomicMaxWorkItemExprs = new std::vector<WorkItemExpr *>();

  for (unsigned i=0; i<loadExprs.size(); i++)
    loadWorkItemExprs->push_back(loadExprs[i]->clone());
  for (unsigned i=0; i<storeExprs.size(); i++)
    storeWorkItemExprs->push_back(storeExprs[i]->clone());
  for (unsigned i=0; i<orExprs.size(); i++)
    orWorkItemExprs->push_back(orExprs[i]->clone());
  for (unsigned i=0; i<atomicSumExprs.size(); i++)
    atomicSumWorkItemExprs->push_back(atomicSumExprs[i]->clone());
  for (unsigned i=0; i<atomicMinExprs.size(); i++)
    atomicMinWorkItemExprs->push_back(atomicMinExprs[i]->clone());
  for (unsigned i=0; i<atomicMaxExprs.size(); i++)
    atomicMaxWorkItemExprs->push_back(atomicMaxExprs[i]->clone());
}

ArgumentAnalysis::ArgumentAnalysis(unsigned pos, TYPE type,
				   unsigned sizeInBytes,
				   std::vector<WorkItemExpr *> *loadExprs,
				   std::vector<WorkItemExpr *> *storeExprs,
				   std::vector<WorkItemExpr *> *orExprs,
				   std::vector<WorkItemExpr *> *atomicSumExprs,
				   std::vector<WorkItemExpr *> *atomicMinExprs,
				   std::vector<WorkItemExpr *> *atomicMaxExprs)
  : nbSplit(0), pos(pos), type(type), sizeInBytes(sizeInBytes),
    kernelNDRange(NULL), subNDRanges(NULL),
    loadWorkItemExprs(loadExprs), storeWorkItemExprs(storeExprs),
    orWorkItemExprs(orExprs), atomicSumWorkItemExprs(atomicSumExprs),
    atomicMinWorkItemExprs(atomicMinExprs),
    atomicMaxWorkItemExprs(atomicMaxExprs),
    mReadBoundsComputed(false), mWriteBoundsComputed(false),
    mOrBoundsComputed(false), mAtomicSumBoundsComputed(false),
    mAtomicMinBoundsComputed(false), mAtomicMaxBoundsComputed(false),
    areDisjoint(false), analysisHasBeenRun(false)
{
}

ArgumentAnalysis::~ArgumentAnalysis() {
  for (unsigned i=0; i<loadWorkItemExprs->size(); ++i)
    delete (*loadWorkItemExprs)[i];
  delete loadWorkItemExprs;
  for (unsigned i=0; i<storeWorkItemExprs->size(); ++i)
    delete (*storeWorkItemExprs)[i];
  delete storeWorkItemExprs;
  for (unsigned i=0; i<orWorkItemExprs->size(); ++i)
    delete (*orWorkItemExprs)[i];
  delete orWorkItemExprs;
  for (unsigned i=0; i<atomicSumWorkItemExprs->size(); ++i)
    delete (*atomicSumWorkItemExprs)[i];
  delete atomicSumWorkItemExprs;
  for (unsigned i=0; i<atomicMinWorkItemExprs->size(); ++i)
    delete (*atomicMinWorkItemExprs)[i];
  delete atomicMinWorkItemExprs;
  for (unsigned i=0; i<atomicMaxWorkItemExprs->size(); ++i)
    delete (*atomicMaxWorkItemExprs)[i];
  delete atomicMaxWorkItemExprs;

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
  for (unsigned i=0; i<orSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<orSubKernelsExprs[i].size(); ++j) {
      delete orSubKernelsExprs[i][j];
    }
  }
  for (unsigned i=0; i<atomicSumSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicSumSubKernelsExprs[i].size(); ++j) {
      delete atomicSumSubKernelsExprs[i][j];
    }
  }
  for (unsigned i=0; i<atomicMinSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicMinSubKernelsExprs[i].size(); ++j) {
      delete atomicMinSubKernelsExprs[i][j];
    }
  }
  for (unsigned i=0; i<atomicMaxSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicMaxSubKernelsExprs[i].size(); ++j) {
      delete atomicMaxSubKernelsExprs[i][j];
    }
  }
}

void
ArgumentAnalysis::dump() {
  std::cerr << "\n\033[1;31m*** Arg no \"" << pos
	    << "\" nb load: " << loadWorkItemExprs->size()
	    << " nb store: " << storeWorkItemExprs->size()
	    << " nb or: " << orWorkItemExprs->size()
	    << " nb atomic sum: " << atomicSumWorkItemExprs->size()
	    << " nb atomic min: " << atomicMinWorkItemExprs->size()
	    << " nb atomic max: " << atomicMaxWorkItemExprs->size()
	    << "\033[0m\n";

  std::cerr << "arg type : ";
  switch(type) {
  case BOOL:
    std::cerr << "bool\n";
    break;
  case CHAR:
    std::cerr << "char\n";
    break;
  case UCHAR:
    std::cerr << "uchar\n";
    break;
  case SHORT:
    std::cerr << "short\n";
    break;
  case USHORT:
    std::cerr << "ushort\n";
    break;
  case INT:
    std::cerr << "int\n";
    break;
  case UINT:
    std::cerr << "uint\n";
    break;
  case LONG:
    std::cerr << "long\n";
    break;
  case ULONG:
    std::cerr << "ulong\n";
    break;
  case FLOAT:
    std::cerr << "float\n";
    break;
  case DOUBLE:
    std::cerr << "double\n";
    break;
  case UNKNOWN:
    std::cerr << "unknown\n";
    break;
  };

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
  for (unsigned i=0; i<orWorkItemExprs->size(); ++i) {
      std::cerr << "\033[1mOr work item expression : \033[0m\n";
      (*orWorkItemExprs)[i]->dump();
      std::cerr << "\n";
  }
  for (unsigned i=0; i<atomicSumWorkItemExprs->size(); ++i) {
      std::cerr << "\033[1mAtomicSum work item expression : \033[0m\n";
      (*atomicSumWorkItemExprs)[i]->dump();
      std::cerr << "\n";
  }
  for (unsigned i=0; i<atomicMinWorkItemExprs->size(); ++i) {
      std::cerr << "\033[1mAtomicMin work item expression : \033[0m\n";
      (*atomicMinWorkItemExprs)[i]->dump();
      std::cerr << "\n";
  }
  for (unsigned i=0; i<atomicMaxWorkItemExprs->size(); ++i) {
      std::cerr << "\033[1mAtomicMax work item expression : \033[0m\n";
      (*atomicMaxWorkItemExprs)[i]->dump();
      std::cerr << "\n";
  }

  if (!analysisHasBeenRun)
    return;

  if (mReadBoundsComputed) {
    for (unsigned i=0; i<nbSplit; i++) {
      std::cerr << "\033[1mread subkernel " << i << " region : \033[0m";
      readSubkernelsRegions[i].debug();
      std::cerr << "\n";
    }
  } else {
    std::cerr << "\033[1mRead region not computed\033[0m\n";
  }

  if (mWriteBoundsComputed) {
    for (unsigned i=0; i<nbSplit; i++) {
      std::cerr << "\033[1mWritten subkernel " << i << " region : \033[0m";
      writtenSubkernelsRegions[i].debug();
      std::cerr << "\n";
    }

    std::cerr << "\033[1mWritten merge region : \033[0m";
    writtenMergeRegion.debug();
    std::cerr << "\n";

  } else {
    std::cerr << "\033[1mWritten region not computed\033[0m\n";
  }

  if (mOrBoundsComputed) {
    for (unsigned i=0; i<nbSplit; i++) {
      std::cerr << "\033[1mOr subkernel " << i << " region : \033[0m";
      writtenOrSubkernelsRegions[i].debug();
      std::cerr << "\n";
    }
  } else {
    std::cerr << "\033[1mWritten OR region not computed\033[0m\n";
  }

  if (mAtomicSumBoundsComputed) {
    for (unsigned i=0; i<nbSplit; i++) {
      std::cerr << "\033[1mAtomicSum subkernel " << i << " region : \033[0m";
      writtenAtomicSumSubkernelsRegions[i].debug();
      std::cerr << "\n";
    }
  } else {
    std::cerr << "\033[1mAtomicSum region not computed\033[0m\n";
  }

  if (mAtomicMinBoundsComputed) {
    for (unsigned i=0; i<nbSplit; i++) {
      std::cerr << "\033[1mAtomicMin subkernel " << i << " region : \033[0m";
      writtenAtomicMinSubkernelsRegions[i].debug();
      std::cerr << "\n";
    }
  } else {
    std::cerr << "\033[1mAtomicMin region not computed\033[0m\n";
  }

  if (mAtomicMaxBoundsComputed) {
    for (unsigned i=0; i<nbSplit; i++) {
      std::cerr << "\033[1mAtomicMax subkernel " << i << " region : \033[0m";
      writtenAtomicMaxSubkernelsRegions[i].debug();
      std::cerr << "\n";
    }
  } else {
    std::cerr << "\033[1mAtomicMax region not computed\033[0m\n";
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
ArgumentAnalysis:: setPartition(const NDRange *kernelNDRange,
				const std::vector<NDRange> *subNDRanges) {
  // Set partition
  this->kernelNDRange = kernelNDRange;
  this->subNDRanges = subNDRanges;

  // Clear subKernelsExprs.
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
  for (unsigned i=0; i<orSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<orSubKernelsExprs[i].size(); ++j) {
      delete orSubKernelsExprs[i][j];
    }

    orSubKernelsExprs[i].resize(0);
  }
  for (unsigned i=0; i<atomicSumSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicSumSubKernelsExprs[i].size(); ++j) {
      delete atomicSumSubKernelsExprs[i][j];
    }

    atomicSumSubKernelsExprs[i].resize(0);
  }
  for (unsigned i=0; i<atomicMinSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicMinSubKernelsExprs[i].size(); ++j) {
      delete atomicMinSubKernelsExprs[i][j];
    }

    atomicMinSubKernelsExprs[i].resize(0);
  }
  for (unsigned i=0; i<atomicMaxSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicMaxSubKernelsExprs[i].size(); ++j) {
      delete atomicMaxSubKernelsExprs[i][j];
    }

    atomicMaxSubKernelsExprs[i].resize(0);
  }

  nbSplit = subNDRanges->size();

  loadSubKernelsExprs.resize(nbSplit);
  storeSubKernelsExprs.resize(nbSplit);
  orSubKernelsExprs.resize(nbSplit);
  atomicSumSubKernelsExprs.resize(nbSplit);
  atomicMinSubKernelsExprs.resize(nbSplit);
  atomicMaxSubKernelsExprs.resize(nbSplit);

  // Clear regions
  readSubkernelsRegions.clear();
  readSubkernelsRegions.resize(nbSplit);
  writtenSubkernelsRegions.clear();
  writtenSubkernelsRegions.resize(nbSplit);
  writtenOrSubkernelsRegions.clear();
  writtenOrSubkernelsRegions.resize(nbSplit);
  writtenAtomicSumSubkernelsRegions.clear();
  writtenAtomicSumSubkernelsRegions.resize(nbSplit);
  writtenAtomicMinSubkernelsRegions.clear();
  writtenAtomicMinSubkernelsRegions.resize(nbSplit);
  writtenAtomicMaxSubkernelsRegions.clear();
  writtenAtomicMaxSubkernelsRegions.resize(nbSplit);
  writtenMergeRegion.clear();

  analysisHasBeenRun = false;
}

void
ArgumentAnalysis::injectArgValues(const std::vector<IndexExprValue *> &argValues) {
  assert(kernelNDRange);

  for (unsigned idx=0; idx<loadWorkItemExprs->size(); idx++)
    (*loadWorkItemExprs)[idx]->injectArgsValues(argValues, *kernelNDRange);
  for (unsigned idx=0; idx<storeWorkItemExprs->size(); idx++)
    (*storeWorkItemExprs)[idx]->injectArgsValues(argValues, *kernelNDRange);
  for (unsigned idx=0; idx<orWorkItemExprs->size(); idx++)
    (*orWorkItemExprs)[idx]->injectArgsValues(argValues, *kernelNDRange);
  for (unsigned idx=0; idx<atomicSumWorkItemExprs->size(); idx++)
    (*atomicSumWorkItemExprs)[idx]->injectArgsValues(argValues, *kernelNDRange);
  for (unsigned idx=0; idx<atomicMinWorkItemExprs->size(); idx++)
    (*atomicMinWorkItemExprs)[idx]->injectArgsValues(argValues, *kernelNDRange);
  for (unsigned idx=0; idx<atomicMaxWorkItemExprs->size(); idx++)
    (*atomicMaxWorkItemExprs)[idx]->injectArgsValues(argValues, *kernelNDRange);
}

enum ArgumentAnalysis::status
ArgumentAnalysis::performAnalysis(const
				  std::vector<std::vector<IndirectionValue> > &
				  subKernelIndirectionValues) {
 // Clear subKernelsExprs.
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
  for (unsigned i=0; i<orSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<orSubKernelsExprs[i].size(); ++j) {
      delete orSubKernelsExprs[i][j];
    }

    orSubKernelsExprs[i].resize(0);
  }
  for (unsigned i=0; i<atomicSumSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicSumSubKernelsExprs[i].size(); ++j) {
      delete atomicSumSubKernelsExprs[i][j];
    }

    atomicSumSubKernelsExprs[i].resize(0);
  }
  for (unsigned i=0; i<atomicMinSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicMinSubKernelsExprs[i].size(); ++j) {
      delete atomicMinSubKernelsExprs[i][j];
    }

    atomicMinSubKernelsExprs[i].resize(0);
  }
  for (unsigned i=0; i<atomicMaxSubKernelsExprs.size(); ++i) {
    for (unsigned j=0; j<atomicMaxSubKernelsExprs[i].size(); ++j) {
      delete atomicMaxSubKernelsExprs[i][j];
    }

    atomicMaxSubKernelsExprs[i].resize(0);
  }

  nbSplit = subNDRanges->size();

  // Vector of size nbsplit, each element containing of vector of
  // IndexExpr (one indexExpr per use)
  //  std::vector<std::vector<IndexExpr *> > kernelSplitExprs(m_nbSplit);
  loadSubKernelsExprs.resize(nbSplit);
  storeSubKernelsExprs.resize(nbSplit);
  orSubKernelsExprs.resize(nbSplit);
  atomicSumSubKernelsExprs.resize(nbSplit);
  atomicMinSubKernelsExprs.resize(nbSplit);
  atomicMaxSubKernelsExprs.resize(nbSplit);

  // Clear regions
  readSubkernelsRegions.clear();
  readSubkernelsRegions.resize(nbSplit);
  writtenSubkernelsRegions.clear();
  writtenSubkernelsRegions.resize(nbSplit);
  writtenOrSubkernelsRegions.clear();
  writtenOrSubkernelsRegions.resize(nbSplit);
  writtenAtomicSumSubkernelsRegions.clear();
  writtenAtomicSumSubkernelsRegions.resize(nbSplit);
  writtenAtomicMinSubkernelsRegions.clear();
  writtenAtomicMinSubkernelsRegions.resize(nbSplit);
  writtenAtomicMaxSubkernelsRegions.clear();
  writtenAtomicMaxSubkernelsRegions.resize(nbSplit);
  writtenMergeRegion.clear();

  analysisHasBeenRun = true;
#ifdef DEBUG
  std::cerr << "\n\033[1;31m*** Arg no \"" << m_pos << "\" nb load: "
  	    << loadWorkItemExprs->size()
	    << " nb store: " << storeWorkItemExprs->size()
	    << " nb or: " << orWorkItemExprs->size()
	    << " nb atomic sum: " << atomicSumWorkItemExprs->size()
	    << " nb atomic min: " << atomicMinWorkItemExprs->size()
	    << " nb atomic max: " << atomicMaxWorkItemExprs->size()
	    << "\033[0m\n";

  for (unsigned i=0; i<subNDRanges->size(); ++i) {
    std::cerr << "subrange no " << i << "\n";
    (*subNDRanges)[i].dump();
    std::cerr << "\n";
  }
#endif

  // Build load subkernel expressions
  for (unsigned idx=0; idx<loadWorkItemExprs->size(); idx++) {
    for (unsigned i=0; i<nbSplit; ++i) {
      IndexExpr *subExpr =
	(*loadWorkItemExprs)[idx]->getKernelExpr((*subNDRanges)[i],
						 subKernelIndirectionValues[i]);

      // NULL if kernelexpr is out of guards
      if (subExpr)
	loadSubKernelsExprs[i].push_back(subExpr);
    }
  }

  // Build store subkernel expressions
  for (unsigned idx=0; idx<storeWorkItemExprs->size(); idx++) {
    for (unsigned i=0; i<nbSplit; ++i) {
      IndexExpr *subExpr =
	(*storeWorkItemExprs)[idx]
	->getKernelExpr((*subNDRanges)[i], subKernelIndirectionValues[i]);

      // NULL if kernelexpr is out of guards
      if (subExpr)
	storeSubKernelsExprs[i].push_back(subExpr);
    }
  }

  // Build or subkernel expressions
  for (unsigned idx=0; idx<orWorkItemExprs->size(); idx++) {
    for (unsigned i=0; i<nbSplit; ++i) {
      IndexExpr *subExpr =
	(*orWorkItemExprs)[idx]->getKernelExpr((*subNDRanges)[i],
					       subKernelIndirectionValues[i]);

      // NULL if kernelexpr is out of guards
      if (subExpr)
	orSubKernelsExprs[i].push_back(subExpr);
    }
  }

  // Build atomicSum subkernel expressions
  for (unsigned idx=0; idx<atomicSumWorkItemExprs->size(); idx++) {
    for (unsigned i=0; i<nbSplit; ++i) {
      IndexExpr *subExpr =
	(*atomicSumWorkItemExprs)[idx]
	->getKernelExpr((*subNDRanges)[i], subKernelIndirectionValues[i]);

      // NULL if kernelexpr is out of guards
      if (subExpr)
	atomicSumSubKernelsExprs[i].push_back(subExpr);
    }
  }

  // Build atomicMin subkernel expressions
  for (unsigned idx=0; idx<atomicMinWorkItemExprs->size(); idx++) {
    for (unsigned i=0; i<nbSplit; ++i) {
      IndexExpr *subExpr =
	(*atomicMinWorkItemExprs)[idx]
	->getKernelExpr((*subNDRanges)[i],subKernelIndirectionValues[i]);

      // NULL if kernelexpr is out of guards
      if (subExpr)
	atomicMinSubKernelsExprs[i].push_back(subExpr);
    }
  }

  // Build atomicMax subkernel expressions
  for (unsigned idx=0; idx<atomicMaxWorkItemExprs->size(); idx++) {
    for (unsigned i=0; i<nbSplit; ++i) {
      IndexExpr *subExpr =
	(*atomicMaxWorkItemExprs)[idx]
	->getKernelExpr((*subNDRanges)[i],subKernelIndirectionValues[i]);

      // NULL if kernelexpr is out of guards
      if (subExpr)
	atomicMaxSubKernelsExprs[i].push_back(subExpr);
    }
  }

  // Compute subkernels bounds
  computeRegions();

  // AtomicSum bounds can be undefined.

  // If written bounds are not computed return FAIL
  if ((isWrittenOr() && !mWriteBoundsComputed) ||
      (isWrittenAtomicMin() && !mAtomicMinBoundsComputed) ||
      (isWrittenAtomicMax() && !mAtomicMaxBoundsComputed) ||
      (isWritten() && !mWriteBoundsComputed)) {
    return ArgumentAnalysis::FAIL;
  }

  // Find out if subkernels region are disjoint
  performDisjointTest();

  if (!areDisjoint) {
    return ArgumentAnalysis::MERGE;
  }

  return ArgumentAnalysis::SUCCESS;
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
ArgumentAnalysis::isWrittenOr() const {
  return orWorkItemExprs->size() > 0;
}

bool
ArgumentAnalysis::isWrittenAtomicSum() const {
  return atomicSumWorkItemExprs->size() > 0;
}

bool
ArgumentAnalysis::isWrittenAtomicMin() const {
  return atomicMinWorkItemExprs->size() > 0;
}

bool
ArgumentAnalysis::isWrittenAtomicMax() const {
  return atomicMaxWorkItemExprs->size() > 0;
}

bool
ArgumentAnalysis::isReadBySubkernel(unsigned i) const {
  return loadSubKernelsExprs[i].size() > 0;
}

bool
ArgumentAnalysis::isWrittenBySubkernel(unsigned i) const {
  return storeSubKernelsExprs[i].size() > 0;
}

bool
ArgumentAnalysis::isWrittenOrBySubkernel(unsigned i) const {
  return orSubKernelsExprs[i].size() > 0;
}

bool
ArgumentAnalysis::isWrittenAtomicSumBySubkernel(unsigned i) const {
  return atomicSumSubKernelsExprs[i].size() > 0;
}

bool
ArgumentAnalysis::isWrittenAtomicMinBySubkernel(unsigned i) const {
  return atomicMinSubKernelsExprs[i].size() > 0;
}

bool
ArgumentAnalysis::isWrittenAtomicMaxBySubkernel(unsigned i) const {
  return atomicMaxSubKernelsExprs[i].size() > 0;
}

unsigned
ArgumentAnalysis::getNumLoad() const {
  return loadWorkItemExprs->size();
}

unsigned
ArgumentAnalysis::getNumStore() const {
  return storeWorkItemExprs->size();
}

unsigned
ArgumentAnalysis::getNumOr() const {
  return orWorkItemExprs->size();
}

unsigned
ArgumentAnalysis::getNumAtomicSum() const {
  return atomicSumWorkItemExprs->size();
}

unsigned
ArgumentAnalysis::getNumAtomicMin() const {
  return atomicMinWorkItemExprs->size();
}

unsigned
ArgumentAnalysis::getNumAtomicMax() const {
  return atomicMaxWorkItemExprs->size();
}

void
ArgumentAnalysis::write(std::stringstream &s) const {
  s.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
  s.write(reinterpret_cast<const char*>(&type), sizeof(type));
  s.write(reinterpret_cast<const char *>(&sizeInBytes), sizeof(sizeInBytes));
  unsigned nbLoad = loadWorkItemExprs->size();
  s.write(reinterpret_cast<const char*>(&nbLoad), sizeof(nbLoad));
  for (unsigned i=0; i<nbLoad; ++i)
    (*loadWorkItemExprs)[i]->write(s);
  unsigned nbStore = storeWorkItemExprs->size();
  s.write(reinterpret_cast<const char*>(&nbStore), sizeof(nbStore));
  for (unsigned i=0; i<nbStore; ++i)
    (*storeWorkItemExprs)[i]->write(s);
  unsigned nbOr = orWorkItemExprs->size();
  s.write(reinterpret_cast<const char*>(&nbOr), sizeof(nbOr));
  for (unsigned i=0; i<nbOr; ++i)
    (*orWorkItemExprs)[i]->write(s);
  unsigned nbAtomicSum = atomicSumWorkItemExprs->size();
  s.write(reinterpret_cast<const char*>(&nbAtomicSum), sizeof(nbAtomicSum));
  for (unsigned i=0; i<nbAtomicSum; ++i)
    (*atomicSumWorkItemExprs)[i]->write(s);
  unsigned nbAtomicMin = atomicMinWorkItemExprs->size();
  s.write(reinterpret_cast<const char*>(&nbAtomicMin), sizeof(nbAtomicMin));
  for (unsigned i=0; i<nbAtomicMin; ++i)
    (*atomicMinWorkItemExprs)[i]->write(s);
  unsigned nbAtomicMax = atomicMaxWorkItemExprs->size();
  s.write(reinterpret_cast<const char*>(&nbAtomicMax), sizeof(nbAtomicMax));
  for (unsigned i=0; i<nbAtomicMax; ++i)
    (*atomicMaxWorkItemExprs)[i]->write(s);
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
  TYPE type;
  s.read(reinterpret_cast<char *>(&type), sizeof(type));
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

  unsigned nbOr;
  s.read(reinterpret_cast<char *>(&nbOr), sizeof(nbOr));
  std::vector<WorkItemExpr *> *orExprs =
    new std::vector<WorkItemExpr *>();
  for (unsigned i=0; i<nbOr; ++i)
    orExprs->push_back(WorkItemExpr::open(s));

  unsigned nbAtomicSum;
  s.read(reinterpret_cast<char *>(&nbAtomicSum), sizeof(nbAtomicSum));
  std::vector<WorkItemExpr *> *atomicSumExprs =
    new std::vector<WorkItemExpr *>();
  for (unsigned i=0; i<nbAtomicSum; ++i)
    atomicSumExprs->push_back(WorkItemExpr::open(s));

  unsigned nbAtomicMin;
  s.read(reinterpret_cast<char *>(&nbAtomicMin), sizeof(nbAtomicMin));
  std::vector<WorkItemExpr *> *atomicMinExprs =
    new std::vector<WorkItemExpr *>();
  for (unsigned i=0; i<nbAtomicMin; ++i)
    atomicMinExprs->push_back(WorkItemExpr::open(s));

  unsigned nbAtomicMax;
  s.read(reinterpret_cast<char *>(&nbAtomicMax), sizeof(nbAtomicMax));
  std::vector<WorkItemExpr *> *atomicMaxExprs =
    new std::vector<WorkItemExpr *>();
  for (unsigned i=0; i<nbAtomicMax; ++i)
    atomicMaxExprs->push_back(WorkItemExpr::open(s));

  return new ArgumentAnalysis(pos, type, sizeInBytes,
			      loadExprs, storeExprs, orExprs,
			      atomicSumExprs, atomicMinExprs, atomicMaxExprs);
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
ArgumentAnalysis::getReadSubkernelRegion(unsigned i) const {
  return readSubkernelsRegions[i];
}

const ListInterval &
ArgumentAnalysis::getWrittenSubkernelRegion(unsigned i) const {
  return writtenSubkernelsRegions[i];
}

const ListInterval &
ArgumentAnalysis::getWrittenOrSubkernelRegion(unsigned i) const {
  return writtenOrSubkernelsRegions[i];
}

const ListInterval &
ArgumentAnalysis::getWrittenAtomicSumSubkernelRegion(unsigned i) const {
  return writtenAtomicSumSubkernelsRegions[i];
}

const ListInterval &
ArgumentAnalysis::getWrittenAtomicMinSubkernelRegion(unsigned i) const {
  return writtenAtomicMinSubkernelsRegions[i];
}

const ListInterval &
ArgumentAnalysis::getWrittenAtomicMaxSubkernelRegion(unsigned i) const {
  return writtenAtomicMaxSubkernelsRegions[i];
}

const ListInterval &
ArgumentAnalysis::getWrittenMergeRegion() const {
  return writtenMergeRegion;
}

ArgumentAnalysis::TYPE
ArgumentAnalysis::getType() const {
  return type;
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

const WorkItemExpr *
ArgumentAnalysis::getOrWorkItemExpr(unsigned n) const {
  return (*orWorkItemExprs)[n];
}

const WorkItemExpr *
ArgumentAnalysis::getAtomicSumWorkItemExpr(unsigned n) const {
  return (*atomicSumWorkItemExprs)[n];
}

const WorkItemExpr *
ArgumentAnalysis::getAtomicMinWorkItemExpr(unsigned n) const {
  return (*atomicMinWorkItemExprs)[n];
}

const WorkItemExpr *
ArgumentAnalysis::getAtomicMaxWorkItemExpr(unsigned n) const {
  return (*atomicMaxWorkItemExprs)[n];
}

const IndexExpr *
ArgumentAnalysis::getLoadSubkernelExpr(unsigned splitno, unsigned useno) const {
  return loadSubKernelsExprs[splitno][useno];
}

const IndexExpr *
ArgumentAnalysis::getStoreSubkernelExpr(unsigned splitno, unsigned useno) const {
  return storeSubKernelsExprs[splitno][useno];
}

const IndexExpr *
ArgumentAnalysis::getOrSubkernelExpr(unsigned splitno, unsigned useno) const {
  return orSubKernelsExprs[splitno][useno];
}

const IndexExpr *
ArgumentAnalysis::getAtomicSumSubkernelExpr(unsigned splitno, unsigned useno) const {
  return atomicSumSubKernelsExprs[splitno][useno];
}

const IndexExpr *
ArgumentAnalysis::getAtomicMinSubkernelExpr(unsigned splitno, unsigned useno) const {
  return atomicMinSubKernelsExprs[splitno][useno];
}

const IndexExpr *
ArgumentAnalysis::getAtomicMaxSubkernelExpr(unsigned splitno, unsigned useno) const {
  return atomicMaxSubKernelsExprs[splitno][useno];
}

void
ArgumentAnalysis::computeRegions() {
  mReadBoundsComputed = true;
  mWriteBoundsComputed = true;
  mOrBoundsComputed = true;
  mAtomicSumBoundsComputed = true;
  mAtomicMinBoundsComputed = true;
  mAtomicMaxBoundsComputed = true;

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

  // Compute written or subkernels regions
  for (unsigned i=0; i<orSubKernelsExprs.size(); ++i) {
    writtenOrSubkernelsRegions[i].clear();

    for (unsigned j=0; j<orSubKernelsExprs[i].size(); ++j) {
      long lb, hb;
      if (!IndexExpr::computeBounds(orSubKernelsExprs[i][j], &lb, &hb)) {
	mOrBoundsComputed = false;
	break;
      }

      lb = lb < 0 ? 0 : lb;
      assert(lb <= hb);

      writtenOrSubkernelsRegions[i].add(Interval(lb, hb));
    }
  }

  // Compute written atomic sum subkernels regions
  for (unsigned i=0; i<atomicSumSubKernelsExprs.size(); ++i) {
    writtenAtomicSumSubkernelsRegions[i].clear();

    for (unsigned j=0; j<atomicSumSubKernelsExprs[i].size(); ++j) {
      long lb, hb;
      if (!IndexExpr::computeBounds(atomicSumSubKernelsExprs[i][j], &lb, &hb)) {
	mAtomicSumBoundsComputed = false;
	break;
      }

      lb = lb < 0 ? 0 : lb;
      assert(lb <= hb);

      writtenAtomicSumSubkernelsRegions[i].add(Interval(lb, hb));
    }
  }

  // Compute written atomic min subkernels regions
  for (unsigned i=0; i<atomicMinSubKernelsExprs.size(); ++i) {
    writtenAtomicMinSubkernelsRegions[i].clear();

    for (unsigned j=0; j<atomicMinSubKernelsExprs[i].size(); ++j) {
      long lb, hb;
      if (!IndexExpr::computeBounds(atomicMinSubKernelsExprs[i][j], &lb, &hb)) {
	mAtomicMinBoundsComputed = false;
	break;
      }

      lb = lb < 0 ? 0 : lb;
      assert(lb <= hb);

      writtenAtomicMinSubkernelsRegions[i].add(Interval(lb, hb));
    }
  }

  // Compute written atomic max subkernels regions
  for (unsigned i=0; i<atomicMaxSubKernelsExprs.size(); ++i) {
    writtenAtomicMaxSubkernelsRegions[i].clear();

    for (unsigned j=0; j<atomicMaxSubKernelsExprs[i].size(); ++j) {
      long lb, hb;
      if (!IndexExpr::computeBounds(atomicMaxSubKernelsExprs[i][j], &lb, &hb)) {
	mAtomicMaxBoundsComputed = false;
	break;
      }

      lb = lb < 0 ? 0 : lb;
      assert(lb <= hb);

      writtenAtomicMaxSubkernelsRegions[i].add(Interval(lb, hb));
    }
  }
}

void ArgumentAnalysis::performDisjointTest() {
  areDisjoint = true;
  writtenMergeRegion.clear();

  // If the argument is n ot used, set areDisjoint to true.
  if (!isWritten())
    return;

  for (unsigned i=0; i<nbSplit -1; ++i) {
    if (!isWrittenBySubkernel(i))
      continue;

    for (unsigned j=i+1; j<nbSplit ; ++j) {
      if (!isWrittenBySubkernel(j))
	continue;

      ListInterval *inter =
	ListInterval::intersection(writtenSubkernelsRegions[i],
				   writtenSubkernelsRegions[j]);
      if (!inter->mList.empty()) {
	areDisjoint = false;
	writtenMergeRegion.myUnion(*inter);
      }

      delete inter;
    }
  }

  if (!areDisjoint) {
    for (unsigned i=0; i<nbSplit; i++) {
      writtenSubkernelsRegions[i].difference(writtenMergeRegion);
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

bool
ArgumentAnalysis::orBoundsComputed() const {
  return mOrBoundsComputed;
}

bool
ArgumentAnalysis::atomicSumBoundsComputed() const {
  return mAtomicSumBoundsComputed;
}

bool
ArgumentAnalysis::atomicMinBoundsComputed() const {
  return mAtomicMinBoundsComputed;
}

bool
ArgumentAnalysis::atomicMaxBoundsComputed() const {
  return mAtomicMaxBoundsComputed;
}
