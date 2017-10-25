#ifndef INDEXEXPRHB_H
#define INDEXEXPRHB_H

#include "IndexExpr.h"

class IndexExprHB : public IndexExpr {
public:
  explicit IndexExprHB(IndexExpr *expr);
  virtual ~IndexExprHB();

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

#endif /* INDEXEXPRHB_H */
