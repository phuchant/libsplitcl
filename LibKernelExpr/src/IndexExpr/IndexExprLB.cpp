#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprLB.h"
#include "IndexExpr/IndexExprInterval.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprLB::IndexExprLB(IndexExpr *expr)
  : IndexExpr(IndexExpr::LB), expr(expr) {}

IndexExprLB::~IndexExprLB() {
  if (expr)
    delete expr;
}

void
IndexExprLB::dump() const {
  std::cerr << "LB(";
  if (expr)
    expr->dump();
  else
    std::cerr << "NULL";
  std::cerr << ")";
}

IndexExpr *
IndexExprLB::clone() const {
  IndexExpr *exprClone = NULL;
  if (expr)
    exprClone = expr->clone();

  return new IndexExprLB(exprClone);
}

IndexExpr *
IndexExprLB::getWorkgroupExpr(const NDRange &ndRange) const {
  IndexExpr *wg_expr;

  if (expr)
    wg_expr = expr->getWorkgroupExpr(ndRange);
  else
    wg_expr = new IndexExprUnknown("null");

  return new IndexExprLB(wg_expr);
}

IndexExpr *
IndexExprLB::getKernelExpr(const NDRange &ndRange,
			      const std::vector<GuardExpr *> & guards,
			      const std::vector<IndirectionValue> &
			      indirValues) const {
  IndexExpr *kl_expr;

  if (expr)
    kl_expr = expr->getKernelExpr(ndRange, guards, indirValues);
  else
    kl_expr = new IndexExprUnknown("null");

  return new IndexExprLB(kl_expr);
}

void
IndexExprLB::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  stream << " LB ";

  stream << " \"];\n";

  if (expr) {
    expr->toDot(stream);
    stream << id << " -> " << expr->getID() << " [label=\"expr\"];\n";
  }
}

const IndexExpr *
IndexExprLB::getExpr() const {
  return expr;
}

IndexExpr *
IndexExprLB::getExpr() {
  return expr;
}

void
IndexExprLB::write(std::stringstream &s) const {
  IndexExpr::write(s);
  if(expr)
    expr->write(s);
  else
    writeNIL(s);
}
