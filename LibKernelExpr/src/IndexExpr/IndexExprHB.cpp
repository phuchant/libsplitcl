#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprHB.h"
#include "IndexExpr/IndexExprConst.h"
#include "IndexExpr/IndexExprInterval.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprHB::IndexExprHB(IndexExpr *expr)
  : IndexExpr(IndexExpr::HB), expr(expr) {}

IndexExprHB::~IndexExprHB() {
  if (expr)
    delete expr;
}

void
IndexExprHB::dump() const {
  std::cerr << "HB(";
  if (expr)
    expr->dump();
  else
    std::cerr << "NULL";
  std::cerr << ")";
}

IndexExpr *
IndexExprHB::clone() const {
  IndexExpr *exprClone = NULL;
  if (expr)
    exprClone = expr->clone();

  return new IndexExprHB(exprClone);
}

IndexExpr *
IndexExprHB::getWorkgroupExpr(const NDRange &ndRange) const {
  IndexExpr *wg_expr;

  if (expr)
    wg_expr = expr->getWorkgroupExpr(ndRange);
  else
    wg_expr = new IndexExprUnknown("null");

  return new IndexExprHB(wg_expr);
}

IndexExpr *
IndexExprHB::getKernelExpr(const NDRange &ndRange,
			   const std::vector<GuardExpr *> & guards,
			   const std::vector<IndirectionValue> &
			   indirValues) const {
  IndexExpr *kl_expr;

  if (expr)
    kl_expr = expr->getKernelExpr(ndRange, guards, indirValues);
  else
    kl_expr = new IndexExprUnknown("null");

  return new IndexExprHB(kl_expr);
}

void
IndexExprHB::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  stream << " HB ";

  stream << " \"];\n";

  if (expr) {
    expr->toDot(stream);
    stream << id << " -> " << expr->getID() << " [label=\"expr\"];\n";
  }
}

const IndexExpr *
IndexExprHB::getExpr() const {
  return expr;
}

IndexExpr *
IndexExprHB::getExpr() {
  return expr;
}

void
IndexExprHB::write(std::stringstream &s) const {
  IndexExpr::write(s);
  if(expr)
    expr->write(s);
  else
    writeNIL(s);
}
