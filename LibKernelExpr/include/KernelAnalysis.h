#ifndef KERNELANALYSIS_H
#define KERNELANALYSIS_H

#include "ArgumentAnalysis.h"
#include "Indirection.h"

#include <map>
#include <vector>

class KernelAnalysis {
 public:
  KernelAnalysis(const char *name,
		 unsigned numArgs,
		 std::vector<size_t> argsSizes,
		 std::vector<ArgumentAnalysis *> argsAnalysis,
		 std::vector<ArgIndirectionRegionExpr *>
		 kernelIndirectionExprs);
  virtual ~KernelAnalysis();

  const char *getName() const;

  // Get arguments info.
  unsigned getNbArguments() const;
  unsigned getNbGlobalArguments() const;
  bool argIsGlobal(unsigned i);
  size_t getArgSize(unsigned i);
  ArgumentAnalysis::TYPE getArgType(unsigned i);
  unsigned getGlobalArgPos(unsigned i);
  unsigned getGlobalArgId(unsigned pos);

  // Set the requested partition.
  void setPartition(const NDRange &kernelNDRange,
		    const std::vector<NDRange> &subNDRanges,
		    const std::vector<int> &argValues);
  const NDRange &getKernelNDRange() const;
  const std::vector<NDRange> &getSubNDRanges() const;

  bool hasIndirection() const;

  // Get the indirection regions to read for the subkernel n.
  const std::vector<ArgIndirectionRegion *> &
  getSubkernelIndirectionsRegions(unsigned n);

  // Set the indirection values for the subkernel n.
  void
  setSubkernelIndirectionsValues(unsigned n,
				 const std::vector<IndirectionValue> &values);

  // Try to split the kernel for the current partition.
  // Return true if the kernel can be split, false otherwise.
  bool performAnalysis();

  // Regions info
  bool argReadBoundsComputed(unsigned argNo) const;

  bool argIsReadBySubkernel(unsigned argNo, unsigned i) const;
  bool argIsWrittenBySubkernel(unsigned argNo, unsigned i) const;
  bool argIsWrittenOrBySubkernel(unsigned argNo, unsigned i) const;
  bool argIsWrittenAtomicSumBySubkernel(unsigned argNo, unsigned i) const;
  bool argIsWrittenAtomicMaxBySubkernel(unsigned argNo, unsigned i) const;

  const ListInterval &
  getArgReadSubkernelRegion(unsigned argNo, unsigned i) const;
  const ListInterval &
  getArgWrittenSubkernelRegion(unsigned argNo, unsigned i) const;
  const ListInterval &
  getArgWrittenOrSubkernelRegion(unsigned argNo, unsigned i) const;
  const ListInterval &
  getArgWrittenAtomicSumSubkernelRegion(unsigned argNo, unsigned i) const;
  const ListInterval &
  getArgWrittenAtomicMaxSubkernelRegion(unsigned argNo, unsigned i) const;

  // I/O
  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;
  static KernelAnalysis *open(std::stringstream &s);
  static KernelAnalysis *openFromFile(const std::string &name);

  // Dump kernel analysis.
  void debug();

 private:
  char *mName;

  // Arguments info
  unsigned numArgs;
  unsigned numGlobalArgs;
  std::vector<size_t> argsSizes;
  std::vector<ArgumentAnalysis *> mArgsAnalysis;
  std::map<unsigned, unsigned> globalArg2PosMap;
  std::map<unsigned, unsigned> argPos2GlobalId;
  std::map<unsigned, bool> argIsGlobalMap;

  // Indirections regions to be read.
  std::vector<ArgIndirectionRegionExpr *> kernelIndirectionExprs;
  std::vector< std::vector<ArgIndirectionRegion *> > subKernelIndirectionRegions;
  std::vector< std::vector<IndirectionValue> > subKernelIndirectionValues;

  // Current partition
  NDRange *kernelNDRange;
  std::vector<NDRange> *subNDRanges;
};

#endif /* KERNELANALYSIS_H */
