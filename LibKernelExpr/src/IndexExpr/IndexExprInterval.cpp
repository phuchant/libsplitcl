#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprConst.h"
#include "IndexExpr/IndexExprInterval.h"

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
  if (lb)
    lb->dump();
  std::cerr << ",";
  if (hb)
    hb->dump();
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
IndexExprInterval::getKernelExpr(const NDRange &ndRange) const {
  return new IndexExprInterval(lb->getKernelExpr(ndRange),
			       hb->getKernelExpr(ndRange));
}

IndexExpr *
IndexExprInterval::getKernelExprWithGuards(const NDRange &ndRange,
					   const std::vector<GuardExpr *> &
					   guards) const {
  return new IndexExprInterval(lb->getKernelExprWithGuards(ndRange, guards),
			       hb->getKernelExprWithGuards(ndRange, guards));
}

// IndexExpr *
// IndexExprInterval::computeExpr() const {
//   IndexExpr *computedLb = NULL;
//   if (lb)
//     computedLb = lb->computeExpr();

//   IndexExpr *computedHb = NULL;
//   if (hb)
//     computedHb = hb->computeExpr();

//   return new IndexExprInterval(computedLb, computedHb);
// }

// IndexExpr *
// IndexExprInterval::removeNeutralElem(bool *changed) const {
//   bool lbChanged = false;
//   IndexExpr *newLb = NULL;
//   if (lb)
//     newLb = lb->removeNeutralElem(&lbChanged);

//   bool hbChanged = false;
//   IndexExpr *newHb = NULL;
//   if (hb)
//     newHb = hb->removeNeutralElem(&hbChanged);

//   *changed = lbChanged || hbChanged;
//   return new IndexExprInterval(newLb, newHb);
// }

// IndexExpr *
// IndexExprInterval::getLowerBound() const {
//   IndexExpr *ret = NULL;
//   if (lb)
//     ret = lb->getLowerBound();
//   return ret;
// }

// IndexExpr *
// IndexExprInterval::getHigherBound() const{
//   IndexExpr *ret = NULL;
//   if (hb)
//     ret = hb->getHigherBound();
//   return ret;
// }

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

