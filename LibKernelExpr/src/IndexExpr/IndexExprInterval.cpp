#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprInterval.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprInterval::IndexExprInterval(IndexExpr *lb, IndexExpr *hb)
  : IndexExpr(IndexExpr::INTERVAL), lb(lb), hb(hb) {}

IndexExprInterval::~IndexExprInterval() {
  if (lb)
    delete lb;
  if (hb)
    delete hb;
}

void
IndexExprInterval::dump() const {
  std::cerr << "[";
  if (lb) {
    if (lb->getTag() == IndexExpr::BINOP)
      std::cerr << "(";

    lb->dump();

    if (lb->getTag() == IndexExpr::BINOP)
      std::cerr << ")";
  }
  std::cerr << ",";
  if (hb) {
    if (hb->getTag() == IndexExpr::BINOP)
      std::cerr << "(";

    hb->dump();

    if (hb->getTag() == IndexExpr::BINOP)
      std::cerr << ")";
  }
  std::cerr << "]";
}

IndexExpr *
IndexExprInterval::clone() const {
  IndexExpr *lbClone = NULL;
  if (lb)
    lbClone = lb->clone();
  IndexExpr *hbClone = NULL;
  if (hb)
    hbClone = hb->clone();

  return new IndexExprInterval(lbClone, hbClone);
}

IndexExpr *
IndexExprInterval::getWorkgroupExpr(const NDRange &ndRange) const {
  return new IndexExprInterval(lb->getWorkgroupExpr(ndRange),
			       hb->getWorkgroupExpr(ndRange));
}

IndexExpr *
IndexExprInterval::getKernelExpr(const NDRange &ndRange,
				 const std::vector<GuardExpr *> & guards,
				 const std::vector<IndirectionValue> &
				 indirValues) const {
  IndexExpr *kl_expr1, *kl_expr2;

  if (lb)
    kl_expr1 = lb->getKernelExpr(ndRange, guards, indirValues);
  else
    kl_expr1 = new IndexExprUnknown("null");
  if (hb)
    kl_expr2 = hb->getKernelExpr(ndRange, guards, indirValues);
  else
    kl_expr2 = new IndexExprUnknown("null");


  return new IndexExprInterval(kl_expr1, kl_expr2);
}

void
IndexExprInterval::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n" << "[]" << " \"];\n";

  if (lb) {
    lb->toDot(stream);
    stream << id << " -> " << lb->getID() << " [label=\"lb\"];\n";
  }

  if (hb) {
    hb->toDot(stream);
    stream << id << " -> " << hb->getID() << " [label=\"hb\"];\n";
  }
}

void
IndexExprInterval::write(std::stringstream &s) const {
  IndexExpr::write(s);
  if (lb)
    lb->write(s);
  else
    writeNIL(s);
  if(hb)
    hb->write(s);
  else
    writeNIL(s);
}

IndexExpr *
IndexExprInterval::getLb() {
  return lb;
}

IndexExpr *
IndexExprInterval::getHb() {
  return hb;
}

const IndexExpr *
IndexExprInterval::getLb() const {
  return lb;
}

const IndexExpr *
IndexExprInterval::getHb() const{
  return hb;
}

void
IndexExprInterval::setLb(IndexExpr *expr) {
  lb = expr;
}

void
IndexExprInterval::setHb(IndexExpr *expr) {
  hb = expr;
}

