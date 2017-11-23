#ifndef INDIRECTION_H
#define INDIRECTION_H

#include <cstdlib>

class IndexExprValue;
class WorkItemExpr;

enum IndirectionType {
  INT,
  FLOAT,
  DOUBLE,
  UNDEF
};

// Expression for the indirection, created by analysis pass.
struct ArgIndirectionRegionExpr {

  ArgIndirectionRegionExpr(unsigned id, unsigned pos, unsigned numBytes,
			   IndirectionType ty, WorkItemExpr *expr);
  ~ArgIndirectionRegionExpr();


  unsigned id;
  unsigned pos;
  unsigned numBytes;
  IndirectionType ty;
  WorkItemExpr *expr;
};

// Returned by KernelAnalysis to libsplit.
struct ArgIndirectionRegion {
  ArgIndirectionRegion(unsigned id, unsigned pos, IndirectionType ty,
		       size_t cb, long lb, long hb);
  ~ArgIndirectionRegion();

  unsigned id;
  unsigned pos;
  IndirectionType ty;
  size_t cb;
  long lb;
  long hb;
};

// Indirection value to inject.
struct IndirectionValue {
  IndirectionValue(unsigned id, IndexExprValue *lb, IndexExprValue *hb);
  IndirectionValue(const IndirectionValue &indir);
  ~IndirectionValue();

  unsigned id;
  IndexExprValue *lb;
  IndexExprValue *hb;
};

#endif /* INDIRECTION_H */
