#ifndef KERNELANALYSIS_H
#define KERNELANALYSIS_H

#include "ArgumentAnalysis.h"

#include <map>
#include <vector>

class KernelAnalysis {
 public:
  KernelAnalysis(const char *name,
		 unsigned numArgs,
		 std::vector<size_t> argsSizes,
		 std::vector<ArgumentAnalysis *> argsAnalysis,
		 bool hasAtomicOrBarrier);
  virtual ~KernelAnalysis();

  const char *getName() const;

  unsigned getNbArguments() const;
  unsigned getNbGlobalArguments() const;

  ArgumentAnalysis *getGlobalArgAnalysis(unsigned i);

  bool argIsGlobal(unsigned i);
  size_t getArgSize(unsigned i);
  unsigned getGlobalArgPos(unsigned i);

  bool hasAtomicOrBarrier() const;

  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;
  static KernelAnalysis *open(std::stringstream &s);
  static KernelAnalysis *openFromFile(const std::string &name);

  void debug();

 private:
  char *mName;
  unsigned numArgs;
  unsigned numGlobalArgs;
  std::vector<size_t> argsSizes;
  std::vector<ArgumentAnalysis *> mArgsAnalysis;
  std::map<unsigned, unsigned> globalArg2PosMap;
  std::map<unsigned, bool> argIsGlobalMap;
  bool mHasAtomicOrBarrier;
};

#endif /* KERNELANALYSIS_H */
