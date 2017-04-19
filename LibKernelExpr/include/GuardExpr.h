#ifndef GUARDEXPR_H
#define GUARDEXPR_H

#include "IndexExpr/IndexExpr.h"
#include "NDRange.h"

#include <vector>
#include <sstream>

class IndexExpr;

class GuardExpr {
public:
  GuardExpr(unsigned oclFunc, unsigned dim, unsigned pred, bool truth,
	    IndexExpr *expr);
  ~GuardExpr();

  GuardExpr *clone() const;

  long getIntExpr(long *intExpr) const;
  unsigned getPredicate() const;
  unsigned getDim() const;
  unsigned getOclFunc() const;
  void injectArgsValues(const std::vector<int> &values, const NDRange &ndRange);

  void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;

  void dump() const;

  static GuardExpr *open(std::stringstream &s);
  static GuardExpr *openFromFile(const std::string &name);

  enum predicate {
    LT  = 0,
    LE  = 1,
    GE  = 2,
    GT  = 3,
    EQ  = 4,
    NEQ = 5
  };

private:
  unsigned mOclFunc;
  unsigned mDim;
  unsigned mPred;
  IndexExpr *mExpr;

  bool mValueComputed;
  long mIntExpr;
};

#endif /* GUARDEXPR_H */
