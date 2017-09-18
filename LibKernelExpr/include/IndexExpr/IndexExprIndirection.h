#ifndef INDEXEXPRINDIRECTION_H
#define INDEXEXPRINDIRECTION_H

#include "IndexExprInterval.h"

class IndexExprIndirection : public IndexExprInterval {
public:
  IndexExprIndirection(unsigned no);
  IndexExprIndirection(unsigned no, IndexExpr *lb, IndexExpr *hb);
  virtual ~IndexExprIndirection();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;
  virtual void write(std::stringstream &s) const;

  unsigned getNo() const;

private:
  unsigned no;
};

#endif /* INDEXEXPRINDIRECTION_H */

