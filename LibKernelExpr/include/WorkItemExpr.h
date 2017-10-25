#ifndef WORKITEMEXPR_H
#define WORKITEMEXPR_H

#include "GuardExpr.h"
#include "IndexExpr/IndexExpr.h"

class IndexExprValue;
class IndirectionValue;

class WorkItemExpr {
public:
  enum TYPE {
    LOAD,
    STORE,
    OR,
    ATOMICSUM,
    ATOMICMIN,
    ATOMICMAX
  };

  WorkItemExpr(const IndexExpr &wiExpr, const std::vector<GuardExpr *> &guards);
  WorkItemExpr(IndexExpr *wiExpr, std::vector<GuardExpr *> *guards);
  WorkItemExpr(const WorkItemExpr &expr);
  ~WorkItemExpr();

  void injectArgsValues(const std::vector<IndexExprValue *> &values,
			const NDRange &kernelNDRange);
  IndexExpr *getKernelExpr(const NDRange &kernelNDRange,
			   const std::vector<IndirectionValue> &
			   indirValues) const;

  WorkItemExpr *clone() const;
  void dump() const;

  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;

  static WorkItemExpr *open(std::stringstream &s);
  static WorkItemExpr *openFromFile(const std::string &name);

private:
  bool isOutOfGuards(const NDRange &kernelNDRange) const;
  IndexExpr *mWiExpr;
  std::vector<GuardExpr *> *mGuards;
};

#endif /* WORKITEMEXPR_H */
