#include "Indirection.h"
#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprIndirection.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <cassert>
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

  IndexExpr *kl_lb, *kl_hb;
  bool found = false;

  for (unsigned i=0; i<indirValues.size(); i++) {
    if (indirValues[i].id == no) {
      found = true;
      assert(indirValues[i].lb);
      assert(indirValues[i].hb);
      kl_lb = indirValues[i].lb->clone();
      kl_hb = indirValues[i].hb->clone();
      break;
    }
  }

  if (!found) {
    kl_lb = new IndexExprUnknown("indir");
    kl_hb = new IndexExprUnknown("indir");
  }

  return new IndexExprIndirection(no, kl_lb, kl_hb);
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

void
IndexExprIndirection::toDot(std::stringstream &stream) const {
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


unsigned
IndexExprIndirection:: getNo() const {
  return no;
}

