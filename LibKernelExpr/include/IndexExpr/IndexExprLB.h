#ifndef INDEXEXPRLB_H
#define INDEXEXPRLB_H

#include "IndexExpr.h"

class IndexExprLB : public IndexExpr {
public:
  IndexExprLB(IndexExpr *expr);
  virtual ~IndexExprLB();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;
  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

  const IndexExpr *getExpr() const;
  IndexExpr *getExpr();

private:
  IndexExpr *expr;
};

#endif /* INDEXEXPRLB_H */
