#ifndef INDEXEXPRBINOP_H
#define INDEXEXPRBINOP_H

#include "IndexExpr.h"

class IndexExprBinop : public IndexExpr {
public:
  IndexExprBinop(unsigned op, IndexExpr *expr1, IndexExpr *expr2);
  virtual ~IndexExprBinop();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;

  virtual void toDot(std::stringstream &stream) const;
  virtual void write(std::stringstream &s) const;

  int getOp() const;
  const IndexExpr *getExpr1() const;
  const IndexExpr *getExpr2() const;
  IndexExpr *getExpr1();
  IndexExpr *getExpr2();
  void setExpr1(IndexExpr *expr);
  void setExpr2(IndexExpr *expr);

  enum BinOp {
    Add = 1,
    Sub = 2,
    Mul = 3,
    Div = 4,
    Mod = 5,
    Or = 6,
    Xor = 7,
    And = 8,
    Shl = 9,
    Shr = 10,
    Unknown = 11
  };

private:
  unsigned op;
  IndexExpr *expr1;
  IndexExpr *expr2;
};

#endif /* INDEXEXPRBINOP_H */
