#include "IndexExpr/IndexExprMax.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprMax::IndexExprMax(unsigned numOperands, IndexExpr **exprs)
  : IndexExpr(IndexExpr::MAX), mNumOperands(numOperands) {
  mExprs = new IndexExpr *[mNumOperands];

  for (unsigned i=0; i<mNumOperands; i++)
    mExprs[i] = exprs[i];
}

IndexExprMax::~IndexExprMax() {
  for (unsigned i=0; i<mNumOperands; i++)
      delete mExprs[i];

  delete[] mExprs;
}

void
IndexExprMax::dump() const {
  std::cerr << "MAX(";

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
IndexExprMax::clone() const {
  IndexExpr *cloneExprs[mNumOperands];

  for (unsigned i=0; i<mNumOperands; i++) {
    if (mExprs[i])
      cloneExprs[i] = mExprs[i]->clone();
    else
      mExprs[i] = NULL;
  }

  return new IndexExprMax(mNumOperands, cloneExprs);
}

IndexExpr *
IndexExprMax::getWorkgroupExpr(const NDRange &ndRange) const {
  IndexExpr *wg_exprs[mNumOperands];

  for (unsigned i=0; i<mNumOperands; i++) {
    if (mExprs[i])
      wg_exprs[i] = mExprs[i]->getWorkgroupExpr(ndRange);
    else
      wg_exprs[i] = new IndexExprUnknown("null");
  }

  return new IndexExprMax(mNumOperands, wg_exprs);
}

IndexExpr *
IndexExprMax::getKernelExpr(const NDRange &ndRange,
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

  return new IndexExprMax(mNumOperands, kl_exprs);
}

void
IndexExprMax::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  stream << " MAX ";

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
IndexExprMax::getExprN(unsigned n) const {
  return mExprs[n];
}

IndexExpr *
IndexExprMax::getExprN(unsigned n) {
  return mExprs[n];
}

void
IndexExprMax::setExprN(unsigned n, IndexExpr *expr) {
  mExprs[n] = expr;
}

unsigned
IndexExprMax::getNumOperands() const {
  return mNumOperands;
}

void
IndexExprMax::write(std::stringstream &s) const {
  IndexExpr::write(s);

  s.write(reinterpret_cast<const char *>(&mNumOperands), sizeof(mNumOperands));

  for (unsigned i=0; i<mNumOperands; i++) {
    if(mExprs[i])
      mExprs[i]->write(s);
    else
      writeNIL(s);
  }
}
