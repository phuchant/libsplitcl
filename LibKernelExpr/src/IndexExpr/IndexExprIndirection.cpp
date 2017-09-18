#include "Indirection.h"
#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprConst.h"
#include "IndexExpr/IndexExprIndirection.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprIndirection::IndexExprIndirection(unsigned no)
  : IndexExprInterval(new IndexExprUnknown("indir"),
		      new IndexExprUnknown("indir")),
    no(no) {
  tag = INDIR;
}

IndexExprIndirection::IndexExprIndirection(unsigned no,
					   IndexExpr *lb, IndexExpr *hb)
  : IndexExprInterval(lb, hb),
    no(no) {
  tag = INDIR;
}

IndexExprIndirection::~IndexExprIndirection() {}

void
IndexExprIndirection::dump() const {
  std::cerr << "indir" << no << "[";
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
IndexExprIndirection::clone() const {
  IndexExpr *lbClone = NULL;
  if (lb)
    lbClone = lb->clone();
  IndexExpr *hbClone = NULL;
  if (hb)
    hbClone = hb->clone();

  return new IndexExprIndirection(no, lbClone, hbClone);
}

IndexExpr *
IndexExprIndirection::getWorkgroupExpr(const NDRange &ndRange) const {
  return new IndexExprIndirection(no,
				  lb->getWorkgroupExpr(ndRange),
				  hb->getWorkgroupExpr(ndRange));
}

IndexExpr *
IndexExprIndirection::getKernelExpr(const NDRange &ndRange,
			      const std::vector<GuardExpr *> & guards,
			      const std::vector<IndirectionValue> &
			      indirValues) const {
  (void) ndRange;
  (void) guards;

  IndexExpr *lb, *hb;
  bool found = false;

  for (unsigned i=0; i<indirValues.size(); i++) {
    if (indirValues[i].id == no) {
      found = true;
      lb = new IndexExprConst(indirValues[i].lb);
      hb = new IndexExprConst(indirValues[i].hb);
    }
  }

  if (!found) {
    lb = new IndexExprUnknown("indir");
    hb = new IndexExprUnknown("indir");
  }

  return new IndexExprIndirection(no, lb, hb);
}

void
IndexExprIndirection::write(std::stringstream &s) const {
  IndexExpr::write(s);
  s.write(reinterpret_cast<const char *>(&no), sizeof(no));

  if (lb)
    lb->write(s);
  else
    writeNIL(s);
  if(hb)
    hb->write(s);
  else
    writeNIL(s);
}

unsigned
IndexExprIndirection:: getNo() const {
  return no;
}

