#ifndef INDEXEXPRCAST_H
#define INDEXEXPRCAST_H

#include "IndexExpr.h"

class IndexExprCast : public IndexExpr {
public:
  enum castTy {
    F2D, D2F,
    I2F, F2I,
    I2D, D2I,
    FLOOR
  };

  castTy cast;

  IndexExprCast(IndexExpr *expr, castTy c);
  virtual ~IndexExprCast();

  const IndexExpr *getExpr() const;
  IndexExpr *getExpr();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;

  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

private:
  IndexExpr *expr;
};

#endif /* INDEXEXPRCAST_H */
