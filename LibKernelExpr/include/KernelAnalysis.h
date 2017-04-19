#ifndef KERNELANALYSIS_H
#define KERNELANALYSIS_H

#include "ArgumentAnalysis.h"

#include <vector>

class KernelAnalysis {
 public:
  KernelAnalysis(const char *name,
		 std::vector<ArgumentAnalysis *> argsAnalysis,
		 bool hasAtomicOrBarrier);
  virtual ~KernelAnalysis();

  const char *getName() const;
  unsigned getNbArguments() const;
  ArgumentAnalysis *getArgAnalysis(unsigned i);
  bool hasAtomicOrBarrier() const;

  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;
  static KernelAnalysis *open(std::stringstream &s);
  static KernelAnalysis *openFromFile(const std::string &name);

  void debug();

 private:
  char *mName;
  std::vector<ArgumentAnalysis *> mArgsAnalysis;
  bool mHasAtomicOrBarrier;
};

#endif /* KERNELANALYSIS_H */
