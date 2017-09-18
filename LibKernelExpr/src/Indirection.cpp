#include "Indirection.h"
#include "WorkItemExpr.h"

ArgIndirectionRegionExpr::ArgIndirectionRegionExpr(unsigned id,
						   unsigned pos,
						   unsigned numBytes,
						   WorkItemExpr *expr)
  : id(id), pos(pos), numBytes(numBytes), expr(expr) {}

ArgIndirectionRegionExpr::~ArgIndirectionRegionExpr() {
  delete expr;
}


ArgIndirectionRegion::ArgIndirectionRegion(unsigned id, unsigned pos,
					   size_t cb, size_t lb, size_t hb)
  : id(id), pos(pos), cb(cb), lb(lb), hb(hb) {}

ArgIndirectionRegion::~ArgIndirectionRegion() {}


IndirectionValue::IndirectionValue(unsigned id, int lb, int hb)
  : id(id), lb(lb), hb(hb) {}

IndirectionValue::IndirectionValue(const IndirectionValue &indir)
  : id(indir.id), lb(indir.lb), hb(indir.hb) {}

IndirectionValue::~IndirectionValue() {}
