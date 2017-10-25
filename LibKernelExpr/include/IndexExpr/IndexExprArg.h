#ifndef INDEXEXPRARG_H
#define INDEXEXPRARG_H

#include "IndexExprValue.h"

class IndexExprArg : public IndexExpr {
public:
  IndexExprArg(const IndexExprArg& that) = delete;
  IndexExprArg(const std::string &name, unsigned pos);
  virtual ~IndexExprArg();

  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

  std::string getName() const;
  unsigned getPos() const;
  void setValue(const IndexExprValue *value);
  const IndexExprValue *getValue() const;

private:
  std::string name;
  unsigned pos;
  bool mIsValueSet;
  IndexExprValue *value;
};

#endif /* INDEXEXPRARG_H */

