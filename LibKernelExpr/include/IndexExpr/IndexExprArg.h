#ifndef INDEXEXPRARG_H
#define INDEXEXPRARG_H

#include "IndexExprValue.h"

class IndexExprArg : public IndexExprValue {
public:
  IndexExprArg(std::string name, unsigned pos);
  virtual ~IndexExprArg();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

  std::string getName() const;
  unsigned getPos() const;
  void setValue(long value);
  bool getValue(long *value) const;

private:
  std::string name;
  unsigned pos;
  bool mIsValueSet;
  long mValue;
};

#endif /* INDEXEXPRARG_H */

