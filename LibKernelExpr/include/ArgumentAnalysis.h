#ifndef ARGUMENTANALYSIS_H
#define ARGUMENTANALYSIS_H

#include "Indirection.h"
#include "ListInterval.h"
#include "NDRange.h"
#include "WorkItemExpr.h"

class IndexExprValue;

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

  enum status {
    SUCCESS,
    MERGE,
    FAIL,
  };

  ArgumentAnalysis(unsigned pos, TYPE type, unsigned sizeInBytes,
		   const std::vector<WorkItemExpr *> &loadExprs,
		   const std::vector<WorkItemExpr *> &storeExprs,
		   const std::vector<WorkItemExpr *> &orExprs,
		   const std::vector<WorkItemExpr *> &atomicSumExprs,
		   const std::vector<WorkItemExpr *> &atomicMinExprs,
		   const std::vector<WorkItemExpr *> &atomicMaxExprs);
  ArgumentAnalysis(unsigned pos, TYPE type, unsigned sizeInBytes,
		   std::vector<WorkItemExpr *> *loadExprs,
		   std::vector<WorkItemExpr *> *storeExprs,
		   std::vector<WorkItemExpr *> *orExprs,
		   std::vector<WorkItemExpr *> *atomicSumExprs,
		   std::vector<WorkItemExpr *> *atomicMinExprs,
		   std::vector<WorkItemExpr *> *atomicMaxExprs);
  ~ArgumentAnalysis();

  bool isWritten() const;
  bool isWrittenOr() const;
  bool isWrittenAtomicSum() const;
  bool isWrittenAtomicMin() const;
  bool isWrittenAtomicMax() const;
  bool isRead() const;
  unsigned getNumLoad() const;
  unsigned getNumStore() const;
  unsigned getNumOr() const;
  unsigned getNumAtomicSum() const;
  unsigned getNumAtomicMin() const;
  unsigned getNumAtomicMax() const;

  const WorkItemExpr *getLoadWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getStoreWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getOrWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getAtomicSumWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getAtomicMinWorkItemExpr(unsigned n) const;
  const WorkItemExpr *getAtomicMaxWorkItemExpr(unsigned n) const;
  const IndexExpr *getLoadSubkernelExpr(unsigned splitno, unsigned useno) const;
  const IndexExpr *getStoreSubkernelExpr(unsigned splitno,
					 unsigned useno) const;
  const IndexExpr *getOrSubkernelExpr(unsigned splitno,
				      unsigned useno) const;
  const IndexExpr *getAtomicSumSubkernelExpr(unsigned splitno,
					     unsigned useno) const;
  const IndexExpr *getAtomicMinSubkernelExpr(unsigned splitno,
					     unsigned useno) const;
  const IndexExpr *getAtomicMaxSubkernelExpr(unsigned splitno,
					     unsigned useno) const;

  bool isReadBySubkernel(unsigned i) const;
  bool isWrittenBySubkernel(unsigned i) const;
  bool isWrittenOrBySubkernel(unsigned i) const;
  bool isWrittenAtomicSumBySubkernel(unsigned i) const;
  bool isWrittenAtomicMinBySubkernel(unsigned i) const;
  bool isWrittenAtomicMaxBySubkernel(unsigned i) const;

  const ListInterval & getReadSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenOrSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenAtomicSumSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenAtomicMinSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenAtomicMaxSubkernelRegion(unsigned i) const;
  const ListInterval & getWrittenMergeRegion() const;

  void dump();

  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;

  void setPartition(const NDRange *kernelNDRange,
		    const std::vector<NDRange> *subNDRanges);
  void injectArgValues(const std::vector<IndexExprValue *> &argValues);

  enum status performAnalysis(const std::vector< std::vector<IndirectionValue> > &
			      subKernelIndirectionValues);

  unsigned getPos() const;
  TYPE getType() const;
  unsigned getSizeInBytes() const;

  static ArgumentAnalysis *open(std::stringstream &s);
  static ArgumentAnalysis *openFromFile(const std::string &name);

  bool readBoundsComputed() const;
  bool writeBoundsComputed() const;
  bool orBoundsComputed() const;
  bool atomicSumBoundsComputed() const;
  bool atomicMinBoundsComputed() const;
  bool atomicMaxBoundsComputed() const;

private:
  void computeRegions();
  void performDisjointTest();

  unsigned nbSplit;
  unsigned pos;
  TYPE type;
  unsigned sizeInBytes;

  /* Partition */
  const NDRange *kernelNDRange;
  const std::vector<NDRange> *subNDRanges;

  /* Expressions */

  // Vector of workitem expressions.
  std::vector<WorkItemExpr *> *loadWorkItemExprs;
  std::vector<WorkItemExpr *> *storeWorkItemExprs;
  std::vector<WorkItemExpr *> *orWorkItemExprs;
  std::vector<WorkItemExpr *> *atomicSumWorkItemExprs;
  std::vector<WorkItemExpr *> *atomicMinWorkItemExprs;
  std::vector<WorkItemExpr *> *atomicMaxWorkItemExprs;

  // Vector of size nbsplit, each  element containing a vector of subkernel
  // expressions.
  // m_subKernelsExprs[nbsplit][].
  std::vector<std::vector<IndexExpr *> > loadSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > storeSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > orSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > atomicSumSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > atomicMinSubKernelsExprs;
  std::vector<std::vector<IndexExpr *> > atomicMaxSubKernelsExprs;

  /* Regions */
  std::vector<ListInterval> readSubkernelsRegions;
  std::vector<ListInterval> writtenSubkernelsRegions;
  std::vector<ListInterval> writtenOrSubkernelsRegions;
  std::vector<ListInterval> writtenAtomicSumSubkernelsRegions;
  std::vector<ListInterval> writtenAtomicMinSubkernelsRegions;
  std::vector<ListInterval> writtenAtomicMaxSubkernelsRegions;
  ListInterval writtenMergeRegion;

  bool mReadBoundsComputed;
  bool mWriteBoundsComputed;
  bool mOrBoundsComputed;
  bool mAtomicSumBoundsComputed;
  bool mAtomicMinBoundsComputed;
  bool mAtomicMaxBoundsComputed;
  bool areDisjoint;
  bool analysisHasBeenRun;
};

#endif /* ARGUMENTANALYSIS_H */
