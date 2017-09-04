#ifndef ARGUMENTANALYSIS_H
#define ARGUMENTANALYSIS_H

#include "ListInterval.h"
#include "NDRange.h"
#include "WorkItemExpr.h"

class ArgumentAnalysis {
public:

  enum TYPE {
    BOOL,
    CHAR, UCHAR,
    SHORT, USHORT,
    INT, UINT,
    LONG, ULONG,
    FLOAT, DOUBLE,
    UNKNOWN
  };

  ArgumentAnalysis(unsigned pos, TYPE type, unsigned sizeInBytes,
		   const std::vector<WorkItemExpr *> &loadExprs,
		   const std::vector<WorkItemExpr *> &storeExprs,
		   const std::vector<WorkItemExpr *> &orExprs,
		   const std::vector<WorkItemExpr *> &atomicSumExprs,
		   const std::vector<WorkItemExpr *> &atomicMaxExprs);
  ArgumentAnalysis(unsigned pos, TYPE type, unsigned sizeInBytes,
		   std::vector<WorkItemExpr *> *loadExprs,
		   std::vector<WorkItemExpr *> *storeExprs,
		   std::vector<WorkItemExpr *> *orExprs,
		   std::vector<WorkItemExpr *> *atomicSumExprs,
		   std::vector<WorkItemExpr *> *atomicMaxExprs);
  ~ArgumentAnalysis();

  bool canSplit() const;
  bool isWritten() const;
  bool isWrittenOr() const;
  bool isWrittenAtomicSum() const;
  bool isWrittenAtomicMax() const;
  bool isRead() const;
  unsigned getNumLoad() const;
  unsigned getNumStore() const;
  unsigned getNumOr() const;
  unsigned getNumAtomicSum() const;
  unsigned getNumAtomicMax() const;

  const WorkItemExpr *getLoadWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getStoreWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getOrWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getAtomicSumWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getAtomicMaxWorkItemExpr(unsigned n) const;
  const IndexExpr *getLoadKernelExpr(unsigned n) const;
  const IndexExpr *getStoreKernelExpr(unsigned n) const;
  const IndexExpr *getOrKernelExpr(unsigned n) const;
  const IndexExpr *getAtomicSumKernelExpr(unsigned n) const;
  const IndexExpr *getAtomicMaxKernelExpr(unsigned n) const;
  const IndexExpr *getLoadSubkernelExpr(unsigned splitno, unsigned useno) const;
  const IndexExpr *getStoreSubkernelExpr(unsigned splitno,
					 unsigned useno) const;
  const IndexExpr *getOrSubkernelExpr(unsigned splitno,
				      unsigned useno) const;
  const IndexExpr *getAtomicSumSubkernelExpr(unsigned splitno,
					     unsigned useno) const;
  const IndexExpr *getAtomicMaxSubkernelExpr(unsigned splitno,
					     unsigned useno) const;

  bool isReadBySplitNo(unsigned i) const;
  bool isWrittenBySplitNo(unsigned i) const;
  bool isWrittenOrBySplitNo(unsigned i) const;
  bool isWrittenAtomicSumBySplitNo(unsigned i) const;
  bool isWrittenAtomicMaxBySplitNo(unsigned i) const;

  const ListInterval & getReadKernelRegion() const;
  const ListInterval & getWrittenKernelRegion() const;
  const ListInterval & getWrittenOrKernelRegion() const;
  const ListInterval & getWrittenAtomicSumKernelRegion() const;
  const ListInterval & getWrittenAtomicMaxKernelRegion() const;
  const ListInterval & getReadSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenOrSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenAtomicSumSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenAtomicMaxSubkernelRegion(unsigned i) const;

  void dump();

  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;

  void performAnalysis(const std::vector<int> &args,
  		       const NDRange &ndRange,
  		       const std::vector<NDRange> &splitNDRanges);

  unsigned getPos() const;
  TYPE getType() const;
  unsigned getSizeInBytes() const;

  static ArgumentAnalysis *open(std::stringstream &s);
  static ArgumentAnalysis *openFromFile(const std::string &name);

  bool readBoundsComputed() const;
  bool writeBoundsComputed() const;
  bool orBoundsComputed() const;
  bool atomicSumBoundsComputed() const;
  bool atomicMaxBoundsComputed() const;

private:
  void computeRegions();
  void performDisjointTest();

  unsigned nbSplit;
  unsigned pos;
  TYPE type;
  unsigned sizeInBytes;

  /* Expressions */

  // Vector of workitem expressions.
  std::vector<WorkItemExpr *> *loadWorkItemExprs;
  std::vector<WorkItemExpr *> *storeWorkItemExprs;
  std::vector<WorkItemExpr *> *orWorkItemExprs;
  std::vector<WorkItemExpr *> *atomicSumWorkItemExprs;
  std::vector<WorkItemExpr *> *atomicMaxWorkItemExprs;

  // Vector of kernel expressions.
  std::vector<IndexExpr *> loadKernelExprs;
  std::vector<IndexExpr *> storeKernelExprs;
  std::vector<IndexExpr *> orKernelExprs;
  std::vector<IndexExpr *> atomicSumKernelExprs;
  std::vector<IndexExpr *> atomicMaxKernelExprs;

  // Vector of size nbsplit, each  element containinga vector of subkernel
  // expressions.
  // m_subKernelsExprs[nbsplit][].
  std::vector<std::vector<IndexExpr *> > loadSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > storeSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > orSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > atomicSumSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > atomicMaxSubKernelsExprs;

  /* Regions */
  ListInterval readKernelRegions;
  ListInterval writtenKernelRegions;
  ListInterval writtenOrKernelRegions;
  ListInterval writtenAtomicSumKernelRegions;
  ListInterval writtenAtomicMaxKernelRegions;
  std::vector<ListInterval> readSubkernelsRegions;
  std::vector<ListInterval> writtenSubkernelsRegions;
  std::vector<ListInterval> writtenOrSubkernelsRegions;
  std::vector<ListInterval> writtenAtomicSumSubkernelsRegions;
  std::vector<ListInterval> writtenAtomicMaxSubkernelsRegions;

  bool mReadBoundsComputed;
  bool mWriteBoundsComputed;
  bool mOrBoundsComputed;
  bool mAtomicSumBoundsComputed;
  bool mAtomicMaxBoundsComputed;
  bool areDisjoint;
  bool analysisHasBeenRun;
};

#endif /* ARGUMENTANALYSIS_H */
