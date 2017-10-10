#ifndef INDEXEXPRMIN_H
#define INDEXEXPRMIN_H

#include "IndexExpr.h"

class IndexExprMin : public IndexExpr {
public:
  IndexExprMin(unsigned numOperands, IndexExpr **exprs);
  virtual ~IndexExprMin();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;

  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

  const IndexExpr *getExprN(unsigned n) const;
  IndexExpr *getExprN(unsigned n);
  void setExprN(unsigned n, IndexExpr *expr);
  unsigned getNumOperands() const;

private:
  unsigned mNumOperands;
  IndexExpr **mExprs;
};

#endif /* INDEXEXPRMIN_H */
