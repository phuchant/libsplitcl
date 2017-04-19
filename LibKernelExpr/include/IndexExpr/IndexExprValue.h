#ifndef INDEXEXPRVALUE_H
#define INDEXEXPRVALUE_H

#include "IndexExpr.h"

class IndexExprValue : public IndexExpr {
public:
  IndexExprValue(unsigned tag);
  virtual ~IndexExprValue();

  virtual void dump() const = 0;
  virtual IndexExpr *clone() const = 0;
  virtual bool getValue(long *value) const = 0;
};

#endif /* INDEXEXPRVALUE_H */
