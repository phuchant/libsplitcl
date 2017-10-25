#ifndef INDEXEXPROCL_H
#define INDEXEXPROCL_H

#include "IndexExpr.h"

class IndexExprOCL : public IndexExpr {
public:
  enum OpenclFunction {
    GET_GLOBAL_ID = 0,
    GET_LOCAL_ID = 1,
    GET_GLOBAL_SIZE = 2,
    GET_LOCAL_SIZE = 3,
    GET_GROUP_ID = 4,
    GET_NUM_GROUPS = 5
  };

  IndexExprOCL(OpenclFunction oclFunc, IndexExpr *arg);
  virtual ~IndexExprOCL();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;

  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

  OpenclFunction getOCLFunc() const;
  const IndexExpr *getArg() const;
  IndexExpr *getArg();
  void setArg(IndexExpr *expr);

private:
  OpenclFunction oclFunc;
  IndexExpr *arg;
};

#endif /* INDEXEXPROCL_H */
