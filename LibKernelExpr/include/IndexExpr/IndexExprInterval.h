#ifndef INDEXEXPRINTERVAL_H
#define INDEXEXPRINTERVAL_H

#include "IndexExpr.h"

class IndexExprInterval : public IndexExpr {
public:
  IndexExprInterval(IndexExpr *lb, IndexExpr *hb);
  virtual ~IndexExprInterval();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExprWithGuards(const NDRange &ndRange,
					     const std::vector<GuardExpr *> &
					     guards) const;
  // virtual IndexExpr *getLowerBound() const;
  // virtual IndexExpr *getHigherBound() const;
  IndexExpr *getLb();
  IndexExpr *getHb();
  const IndexExpr *getLb() const;
  const IndexExpr *getHb() const;
  void setLb(IndexExpr *expr);
  void setHb(IndexExpr *expr);

  // virtual IndexExpr *computeExpr() const;
  // virtual IndexExpr *removeNeutralElem(bool *changed) const;
  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

private:
  IndexExpr *lb;
  IndexExpr *hb;
};

#endif /* INDEXEXPRINTERVAL_H */
