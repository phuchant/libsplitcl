#include "IndexExpr/IndexExprCast.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprCast::IndexExprCast(IndexExpr *expr, castTy c)
  : IndexExpr(CAST), cast(c), expr(expr) {
}

IndexExprCast::~IndexExprCast() {
  delete expr;
}

const IndexExpr *
IndexExprCast::getExpr() const {
  return expr;
}

IndexExpr *
IndexExprCast::getExpr() {
  return expr;
}

void
IndexExprCast::dump() const {
  switch (cast) {
  case F2D:
    std::cerr << "F2D(";
    break;
  case D2F:
    std::cerr << "D2F(";
    break;
  case I2F:
    std::cerr << "I2F(";
    break;
  case F2I:
    std::cerr << "F2I(";
    break;
  case I2D:
    std::cerr << "I2D(";
    break;
  case D2I:
    std::cerr << "D2I(";
    break;
  case FLOOR:
    std::cerr << "FLOOR(";
    break;
  };

  expr->dump();

  std::cerr << ")";
}

IndexExpr *
IndexExprCast::clone() const {
  IndexExpr *cloneExpr = nullptr;
  if (expr)
    cloneExpr = expr->clone();
  return new IndexExprCast(cloneExpr, cast);
}

IndexExpr *
IndexExprCast::getWorkgroupExpr(const NDRange &ndRange) const {
  IndexExpr *wgExpr = expr->getWorkgroupExpr(ndRange);
  return new IndexExprCast(wgExpr, cast);
}

IndexExpr *
IndexExprCast::getKernelExpr(const NDRange & ndRange,
			     const std::vector<GuardExpr *> & guards,
			     const std::vector<IndirectionValue> &
			     indirValues) const {
  IndexExpr *kernelExpr = NULL;
  if (expr)
    kernelExpr = expr->getKernelExpr(ndRange, guards, indirValues);
  else
    kernelExpr = new IndexExprUnknown("null");

  return new IndexExprCast(kernelExpr, cast);
}

void
IndexExprCast::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  switch (cast) {
  case F2D:
    stream << "F2D";
    break;
  case D2F:
    stream << "D2F";
    break;
  case I2F:
    stream << "I2F";
    break;
  case F2I:
    stream << "F2I";
    break;
  case I2D:
    stream << "I2D";
    break;
  case D2I:
    stream << "D2I";
    break;
  case FLOOR:
    stream << "FLOOR";
    break;
  }

  stream << " \"];\n";

  expr->toDot(stream);
  stream << id << " -> " << expr->getID() << " [label=\"expr1\"];\n";
}

void
IndexExprCast::write(std::stringstream &s) const {
  IndexExpr::write(s);
  s.write(reinterpret_cast<const char *>(&cast), sizeof(cast));
  if(expr)
    expr->write(s);
  else
    writeNIL(s);
}
