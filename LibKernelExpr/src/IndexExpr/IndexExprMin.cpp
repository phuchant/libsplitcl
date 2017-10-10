#include "IndexExpr/IndexExprMin.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprMin::IndexExprMin(unsigned numOperands, IndexExpr **exprs)
  : IndexExpr(IndexExpr::MIN), mNumOperands(numOperands) {
  mExprs = new IndexExpr *[mNumOperands];

  for (unsigned i=0; i<mNumOperands; i++)
    mExprs[i] = exprs[i];
}

IndexExprMin::~IndexExprMin() {
  for (unsigned i=0; i<mNumOperands; i++)
      delete mExprs[i];

  delete[] mExprs;
}

void
IndexExprMin::dump() const {
  std::cerr << "MIN(";

  for (unsigned i=0; i<mNumOperands; i++) {
    if (mExprs[i]) {
      if (mExprs[i]->getTag() == IndexExpr::BINOP)
	std::cerr << "(";

      mExprs[i]->dump();

      if (mExprs[i]->getTag() == IndexExpr::BINOP)
	std::cerr << ")";
    } else {
      std::cerr << "NULL";
    }

    if (i < mNumOperands-1)
      std::cerr << ",";
  }

  std::cerr << ")";
}

IndexExpr *
IndexExprMin::clone() const {
  IndexExpr *cloneExprs[mNumOperands];

  for (unsigned i=0; i<mNumOperands; i++) {
    if (mExprs[i])
      cloneExprs[i] = mExprs[i]->clone();
    else
      mExprs[i] = NULL;
  }

  return new IndexExprMin(mNumOperands, cloneExprs);
}

IndexExpr *
IndexExprMin::getWorkgroupExpr(const NDRange &ndRange) const {
  IndexExpr *wg_exprs[mNumOperands];

  for (unsigned i=0; i<mNumOperands; i++) {
    if (mExprs[i])
      wg_exprs[i] = mExprs[i]->getWorkgroupExpr(ndRange);
    else
      wg_exprs[i] = new IndexExprUnknown("null");
  }

  return new IndexExprMin(mNumOperands, wg_exprs);
}

IndexExpr *
IndexExprMin::getKernelExpr(const NDRange &ndRange,
			    const std::vector<GuardExpr *> & guards,
			    const std::vector<IndirectionValue> &
			    indirValues) const {
  IndexExpr *kl_exprs[mNumOperands];

  for (unsigned i=0; i<mNumOperands; i++) {
    if (mExprs[i])
      kl_exprs[i] = mExprs[i]->getKernelExpr(ndRange, guards, indirValues);
    else
      kl_exprs[i] = new IndexExprUnknown("null");
  }

  return new IndexExprMin(mNumOperands, kl_exprs);
}

void
IndexExprMin::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  stream << " MIN ";

  stream << " \"];\n";

  for (unsigned i=0; i<mNumOperands; i++) {
    if (mExprs[i]) {
      mExprs[i]->toDot(stream);
      stream << id << " -> " << mExprs[i]->getID() << " [label=\"expr"
	     << i << "\"];\n";
    }
  }
}

const IndexExpr *
IndexExprMin::getExprN(unsigned n) const {
  return mExprs[n];
}

IndexExpr *
IndexExprMin::getExprN(unsigned n) {
  return mExprs[n];
}

void
IndexExprMin::setExprN(unsigned n, IndexExpr *expr) {
  mExprs[n] = expr;
}

unsigned
IndexExprMin::getNumOperands() const {
  return mNumOperands;
}

void
IndexExprMin::write(std::stringstream &s) const {
  IndexExpr::write(s);

  s.write(reinterpret_cast<const char *>(&mNumOperands), sizeof(mNumOperands));

  for (unsigned i=0; i<mNumOperands; i++) {
    if(mExprs[i])
      mExprs[i]->write(s);
    else
      writeNIL(s);
  }
}
