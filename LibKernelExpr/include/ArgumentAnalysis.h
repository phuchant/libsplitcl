#ifndef ARGUMENTANALYSIS_H
#define ARGUMENTANALYSIS_H

#include "ListInterval.h"
#include "NDRange.h"
#include "WorkItemExpr.h"

class ArgumentAnalysis {
public:
  ArgumentAnalysis(unsigned pos, unsigned sizeInBytes,
		   const std::vector<WorkItemExpr *> &loadExprs,
		   const std::vector<WorkItemExpr *> &storeExprs);
  ArgumentAnalysis(unsigned pos, unsigned sizeInBytes,
		   std::vector<WorkItemExpr *> *loadExprs,
		   std::vector<WorkItemExpr *> *storeExprs);
  ~ArgumentAnalysis();

  bool canSplit() const;
  bool isWritten() const;
  bool isRead() const;
  unsigned getNumLoad() const;
  unsigned getNumStore() const;

  const WorkItemExpr *getLoadWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getStoreWorkItemExpr(unsigned n) const;
  const IndexExpr *getLoadKernelExpr(unsigned n) const;
  const IndexExpr *getStoreKernelExpr(unsigned n) const;
  const IndexExpr *getLoadSubkernelExpr(unsigned splitno, unsigned useno) const;
  const IndexExpr *getStoreSubkernelExpr(unsigned splitno,
					 unsigned useno) const;

  bool isReadBySplitNo(unsigned i) const;
  bool isWrittenBySplitNo(unsigned i) const;

  const ListInterval & getReadKernelRegion() const;
  const ListInterval & getWrittenKernelRegion() const;
  const ListInterval & getReadSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenSubkernelRegion(unsigned i) const;

  void dump();

  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;

  void performAnalysis(const std::vector<int> &args,
  		       const NDRange &ndRange,
  		       const std::vector<NDRange> &splitNDRanges);

  unsigned getPos() const;
  unsigned getSizeInBytes() const;

  static ArgumentAnalysis *open(std::stringstream &s);
  static ArgumentAnalysis *openFromFile(const std::string &name);

  bool readBoundsComputed() const;
  bool writeBoundsComputed() const;

private:
  void computeRegions();
  void performDisjointTest();

  unsigned nbSplit;
  unsigned pos;
  unsigned sizeInBytes;

  /* Expressions */

  // Vector of workitem expressions.
  std::vector<WorkItemExpr *> *loadWorkItemExprs;
  std::vector<WorkItemExpr *> *storeWorkItemExprs;

  // Vector of kernel expressions.
  std::vector<IndexExpr *> loadKernelExprs;
  std::vector<IndexExpr *> storeKernelExprs;

  // Vector of size nbsplit, each  element containinga vector of subkernel
  // expressions.
  // m_subKernelsExprs[nbsplit][].
  std::vector<std::vector<IndexExpr *> > loadSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > storeSubKernelsExprs;

  /* Regions */
  ListInterval readKernelRegions;
  ListInterval writtenKernelRegions;
  std::vector<ListInterval> readSubkernelsRegions;
  std::vector<ListInterval> writtenSubkernelsRegions;

  bool mReadBoundsComputed;
  bool mWriteBoundsComputed;
  bool areDisjoint;
};

#endif /* ARGUMENTANALYSIS_H */
