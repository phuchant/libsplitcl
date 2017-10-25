#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprBinop.h"
#include "IndexExpr/IndexExprInterval.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprBinop::IndexExprBinop(IndexExprBinop::BinOp op, IndexExpr *expr1, IndexExpr *expr2)
  : IndexExpr(IndexExpr::BINOP), op(op), expr1(expr1), expr2(expr2) {}

IndexExprBinop::~IndexExprBinop() {
  if (expr1)
    delete expr1;
  if (expr2)
    delete expr2;
}

void
IndexExprBinop::dump() const {
  if (expr1) {
    if (expr1->getTag() == IndexExpr::BINOP)
      std::cerr << "(";
    expr1->dump();
    if (expr1->getTag() == IndexExpr::BINOP)
      std::cerr << ")";
  }
  else {
    std::cerr << "NULL";
  }

  switch(op) {
  case IndexExprBinop::Add:
    std::cerr << " + ";
    break;
  case IndexExprBinop::Sub:
    std::cerr << " - ";
    break;
  case IndexExprBinop::Mul:
    std::cerr << " * ";
    break;
  case IndexExprBinop::Div:
    std::cerr << " / ";
    break;
  case IndexExprBinop::Mod:
    std::cerr << " % ";
    break;
  case IndexExprBinop::Or:
    std::cerr << " | ";
    break;
  case IndexExprBinop::Xor:
    std::cerr << " ^ ";
    break;
  case IndexExprBinop::And:
    std::cerr << " & ";
    break;
  case IndexExprBinop::Shl:
    std::cerr << " << ";
    break;
  case IndexExprBinop::Shr:
    std::cerr << " >> ";
    break;
  default:
    std::cerr << " ? ";
    break;
  };

  if (expr2) {
    if (expr2->getTag() == IndexExpr::BINOP)
      std::cerr << "(";
    expr2->dump();
    if (expr2->getTag() == IndexExpr::BINOP)
      std::cerr << ")";
  } else {
    std::cerr << "NULL";
  }
}

IndexExpr *
IndexExprBinop::clone() const {
  IndexExpr *expr1Clone = NULL;
  if (expr1)
    expr1Clone = expr1->clone();

  IndexExpr *expr2Clone = NULL;
  if (expr2)
    expr2Clone = expr2->clone();

  return new IndexExprBinop(op, expr1Clone, expr2Clone);
}

IndexExpr *
IndexExprBinop::getWorkgroupExpr(const NDRange &ndRange) const {
  IndexExpr *wg_expr1;
  IndexExpr *wg_expr2;

  if (expr1)
    wg_expr1 = expr1->getWorkgroupExpr(ndRange);
  else
    wg_expr1 = new IndexExprUnknown("null");

  if (expr2)
    wg_expr2 = expr2->getWorkgroupExpr(ndRange);
  else
    wg_expr2 = new IndexExprUnknown("null");

  return new IndexExprBinop(op, wg_expr1, wg_expr2);
}

IndexExpr *
IndexExprBinop::getKernelExpr(const NDRange &ndRange,
			      const std::vector<GuardExpr *> & guards,
			      const std::vector<IndirectionValue> &
			      indirValues) const {
  IndexExpr *kl_expr1;
  IndexExpr *kl_expr2;

  if (expr1)
    kl_expr1 = expr1->getKernelExpr(ndRange, guards, indirValues);
  else
    kl_expr1 = new IndexExprUnknown("null");

  if (expr2)
    kl_expr2 = expr2->getKernelExpr(ndRange, guards, indirValues);
  else
    kl_expr2 = new IndexExprUnknown("null");

  return new IndexExprBinop(op, kl_expr1, kl_expr2);
}

void
IndexExprBinop::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  switch (op) {
  case IndexExprBinop::Add:
    stream << " + ";
    break;
  case IndexExprBinop::Sub:
    stream << " - ";
    break;
  case IndexExprBinop::Mul:
    stream << " * ";
    break;
  case IndexExprBinop::Div:
    stream << " / ";
    break;
  case IndexExprBinop::Mod:
    stream << " % ";
    break;
  case IndexExprBinop::Or:
    stream << " or ";
    break;
  case IndexExprBinop::Xor:
    stream << " ^ ";
    break;
  case IndexExprBinop::And:
    stream << " & ";
    break;
  case IndexExprBinop::Shl:
    stream << " << ";
    break;
  case IndexExprBinop::Shr:
    stream << " >> ";
    break;
  }

  stream << " \"];\n";

  if (expr1) {
    expr1->toDot(stream);
    stream << id << " -> " << expr1->getID() << " [label=\"expr1\"];\n";
  }

  if (expr2) {
    expr2->toDot(stream);
    stream << id << " -> " << expr2->getID() << " [label=\"expr2\"];\n";
  }
}

IndexExprBinop::BinOp
IndexExprBinop::getOp() const {
  return op;
}

const IndexExpr *
IndexExprBinop::getExpr1() const {
  return expr1;
}

const IndexExpr *
IndexExprBinop::getExpr2() const {
  return expr2;
}

IndexExpr *
IndexExprBinop::getExpr1() {
  return expr1;
}

IndexExpr *
IndexExprBinop::getExpr2() {
  return expr2;
}

void
IndexExprBinop::setExpr1(IndexExpr *expr) {
  expr1 = expr;
}

void
IndexExprBinop::setExpr2(IndexExpr *expr) {
  expr2 = expr;
}

void
IndexExprBinop::write(std::stringstream &s) const {
  IndexExpr::write(s);
  s.write(reinterpret_cast<const char *>(&op), sizeof(op));
  if(expr1)
    expr1->write(s);
  else
    writeNIL(s);
  if (expr2)
    expr2->write(s);
  else
    writeNIL(s);
}
