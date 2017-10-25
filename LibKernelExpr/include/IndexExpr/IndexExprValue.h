#ifndef INDEXEXPRVALUE_H
#define INDEXEXPRVALUE_H

#include "IndexExpr.h"

class IndexExprValue : public IndexExpr {
public:

  static IndexExprValue *createLong(long v);
  static IndexExprValue *createFloat(float v);
  static IndexExprValue *createDouble(double b);


  virtual ~IndexExprValue();

  virtual void write(std::stringstream &s) const;
  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;

  virtual void toDot(std::stringstream &stream) const;

  long getLongValue() const;
  float getFloatValue() const;
  double getDoubleValue() const;

  value_type type;
private:
  explicit IndexExprValue(value_type t);

  value value;
};

#endif /* INDEXEXPRVALUE_H */
