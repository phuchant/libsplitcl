#ifndef INDEXEXPROCL_H
#define INDEXEXPROCL_H

#include "IndexExpr.h"

enum OpenclFunctions {
  UNKNOWN = 0,
  GET_GLOBAL_ID = 1,
  GET_LOCAL_ID = 2,
  GET_GLOBAL_SIZE = 3,
  GET_LOCAL_SIZE = 4,
  GET_GROUP_ID = 5,
  GET_NUM_GROUPS = 6
};

class IndexExprOCL : public IndexExpr {
public:
  IndexExprOCL(unsigned oclFunc, IndexExpr *arg);
  virtual ~IndexExprOCL();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExprWithGuards(const NDRange &ndRange,
					     const std::vector<GuardExpr *> &
					     guards) const;
  // virtual IndexExpr *getLowerBound() const;
  // virtual IndexExpr *getHigherBound() const;
  // virtual IndexExpr *computeExpr() const;
  // virtual IndexExpr *removeNeutralElem(bool *changed) const;
  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

  unsigned getOCLFunc() const;
  const IndexExpr *getArg() const;
  IndexExpr *getArg();
  void setArg(IndexExpr *expr);

private:
  unsigned oclFunc;
  IndexExpr *arg;
};

#endif /* INDEXEXPROCL_H */
