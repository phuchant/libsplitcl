#ifndef INDEXEXPRCONST_H
#define INDEXEXPRCONST_H

#include "IndexExprValue.h"

class IndexExprConst : public  IndexExprValue {
public:
  IndexExprConst(long i);
  virtual ~IndexExprConst();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual void toDot(std::stringstream &stringstream) const;
  virtual void write(std::stringstream &s) const;
  virtual bool getValue(long *value) const;

private:
  long  intValue;
};

#endif /* INDEXEXPRCONST_H */
