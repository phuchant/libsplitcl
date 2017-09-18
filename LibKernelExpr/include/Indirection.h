#ifndef INDIRECTION_H
#define INDIRECTION_H

#include <cstdlib>

// TODO: replace num bytes par type ?

class WorkItemExpr;

struct ArgIndirectionRegionExpr {
  ArgIndirectionRegionExpr(unsigned id, unsigned pos, unsigned numBytes,
			   WorkItemExpr *expr);
  ~ArgIndirectionRegionExpr();

  unsigned id;
  unsigned pos;
  unsigned numBytes;
  WorkItemExpr *expr;
};


struct ArgIndirectionRegion {
  ArgIndirectionRegion(unsigned id, unsigned pos,
		       size_t cb, size_t lb, size_t hb);
  ~ArgIndirectionRegion();

  unsigned id;
  unsigned pos;
  size_t cb;
  size_t lb;
  size_t hb;
};

struct IndirectionValue {
  IndirectionValue(unsigned id, int lb, int hb);
  IndirectionValue(const IndirectionValue &indir);
  ~IndirectionValue();

  unsigned id;
  int lb;
  int hb;
};

#endif /* INDIRECTION_H */
