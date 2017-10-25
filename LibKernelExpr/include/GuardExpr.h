#ifndef GUARDEXPR_H
#define GUARDEXPR_H

#include "IndexExpr/IndexExprOCL.h"


#include <vector>
#include <sstream>

class IndexExpr;
class IndexExprValue;
class NDRange;

class GuardExpr {
public:
  enum predicate {
    LT  = 0,
    LE  = 1,
    GE  = 2,
    GT  = 3,
    EQ  = 4,
    NEQ = 5
  };

  GuardExpr(IndexExprOCL::OpenclFunction oclFunc,
	    unsigned dim, predicate pred, bool truth,
	    IndexExpr *expr);
  ~GuardExpr();

  GuardExpr *clone() const;

  long getIntExpr(long *intExpr) const;
  predicate getPredicate() const;
  unsigned getDim() const;
  unsigned getOclFunc() const;
  void injectArgsValues(const std::vector<IndexExprValue *> &values,
			const NDRange &kernelNDRange);

  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;

  void dump() const;

  static GuardExpr *open(std::stringstream &s);
  static GuardExpr *openFromFile(const std::string &name);


private:
  IndexExprOCL::OpenclFunction mOclFunc;
  unsigned mDim;
  predicate mPred;
  IndexExpr *mExpr;

  bool mValueComputed;
  long mIntExpr;
};

#endif /* GUARDEXPR_H */
