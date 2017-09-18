#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprOCL.h"
#include "IndexExpr/IndexExprConst.h"
#include "IndexExpr/IndexExprInterval.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprOCL::IndexExprOCL(unsigned oclFunc, IndexExpr *arg)
  : IndexExpr(IndexExpr::OCL), oclFunc(oclFunc), arg(arg) {}

IndexExprOCL::~IndexExprOCL() {
  if (arg)
    delete arg;
}

void
IndexExprOCL::dump() const {
  switch(oclFunc) {
  case GET_GLOBAL_ID:
    std::cerr << "get_global_id(";
    break;
  case GET_LOCAL_ID:
    std::cerr << "get_local_id(";
    break;
  case GET_GLOBAL_SIZE:
    std::cerr << "get_global_size(";
    break;
  case GET_LOCAL_SIZE:
    std::cerr << "get_local_size(";
    break;
  case GET_GROUP_ID:
    std::cerr << "get_group_id(";
    break;
  case GET_NUM_GROUPS:
    std::cerr << "get_num_groups(";
    break;
  default:
    std::cerr << "unknown(";
    break;
  };

  if (arg)
    arg->dump();

  std::cerr << ")";
}

IndexExpr *
IndexExprOCL::clone() const {
  IndexExpr *cloneArg = NULL;
  if (arg)
    cloneArg = arg->clone();
  return new IndexExprOCL(oclFunc, cloneArg);
}

IndexExpr *
IndexExprOCL::getWorkgroupExpr(const NDRange &ndRange) const {
  unsigned dimindx;

  if (arg && arg->getTag() == IndexExpr::CONST) {
    IndexExprConst *constExpr = static_cast<IndexExprConst *>(arg);
    long value;
    constExpr->getValue(&value);
    dimindx = (unsigned) value;
  } else {
    return new IndexExprUnknown("ocl func");
  }

  switch (oclFunc) {
  case GET_GLOBAL_ID:
    {
      if (dimindx >= ndRange.get_work_dim())
	return new IndexExprConst(0);

      long lb = 0;
      long hb = ndRange.get_local_size(dimindx) - 1;
      return new IndexExprInterval(new IndexExprConst(lb),
				   new IndexExprConst(hb));
    }

  case GET_GLOBAL_SIZE:
    if (dimindx >= ndRange.get_work_dim())
      return new IndexExprConst(1);

    return new IndexExprConst(ndRange.get_global_size(dimindx));

  case GET_LOCAL_SIZE:
    if (dimindx >= ndRange.get_work_dim())
      return new IndexExprConst(1);

    return new IndexExprConst(ndRange.get_local_size(dimindx));

  default:
    return new IndexExprUnknown("ocl func");
  }
}

static void applyGuard(GuardExpr *guard, long *lb, long *hb) {
  long guardValue;
  if (!guard->getIntExpr(&guardValue))
    return;

  switch(guard->getPredicate()) {
  case GuardExpr::LT:
    if ((int) *lb >= guardValue)
      *lb = guardValue-1;
    if ((int) *hb >= guardValue)
      *hb = guardValue-1;
    break;
  case GuardExpr::LE:
    if ((int) *lb > guardValue)
      *lb = guardValue;
    if ((int) *hb > guardValue)
      *hb = guardValue;
    break;
  case GuardExpr::GE:
    if ((int) *lb < guardValue)
      *lb = guardValue;
    if ((int) *hb < guardValue)
      *hb = guardValue;
    break;
  case GuardExpr::GT:
    if ((int) *lb <= guardValue)
      *lb = guardValue+1;
    if ((int) *hb <= guardValue)
      *hb = guardValue+1;
    break;
  case GuardExpr::EQ:
    *lb = guardValue;
    *hb = guardValue;
    break;
  case GuardExpr::NEQ:
    if (guardValue == 0) {
      if ((int) *lb <= 0)
	*lb = 1;
      if ((int) *hb <= 0)
	*hb = 1;
    }
    break;
  };
}

IndexExpr *
IndexExprOCL::getKernelExpr(const NDRange &ndRange,
			    const std::vector<GuardExpr *> & guards,
			    const std::vector<IndirectionValue> &
			    indirValues) const {
  (void) indirValues;

  long dimindx;

  if (IndexExprValue *valueExprArg = dynamic_cast<IndexExprValue *>(arg)) {
    long value;
    if (valueExprArg->getValue(&value))
      dimindx = value;
    else
      return new IndexExprUnknown("ocl func");
  } else {
    return new IndexExprUnknown("ocl func");
  }

  switch (oclFunc) {
  case GET_GLOBAL_ID:
    {
      if (dimindx >= ndRange.get_work_dim())
	return new IndexExprConst(0);

      // global id bounds
      long globalLb = ndRange.getOffset(dimindx);
      long globalHb = ndRange.get_global_size(dimindx) - 1
	+ ndRange.getOffset(dimindx);

      // group id bounds
      long groupLb = globalLb / ndRange.get_local_size(dimindx);
      long groupHb = globalHb / ndRange.get_local_size(dimindx);

      // local id bounds
      long localLb = 0;
      long localHb = ndRange.get_local_size(dimindx) - 1;

      // Apply guards
      for (unsigned i=0; i<guards.size(); ++i) {
	if (guards[i]->getDim() != dimindx)
	  continue;

	switch(guards[i]->getOclFunc()) {
	case GET_GLOBAL_ID:
	  applyGuard(guards[i], &globalLb, &globalHb);
	  break;

	case GET_GROUP_ID:
	  applyGuard(guards[i], &groupLb, &groupHb);
	    break;

	case GET_LOCAL_ID:
	  applyGuard(guards[i], &localLb, &localHb);
	    break;
	};
      }

      long newLb, newHb;

      // Apply group id and local id bounds
      newLb = groupLb * ndRange.get_local_size(dimindx) + localLb;
      newHb = groupHb * ndRange.get_local_size(dimindx) + localHb;
      globalLb = newLb > globalLb ? newLb : globalLb;
      globalHb = newHb < globalHb ? newHb : globalHb;

      return new IndexExprInterval(new IndexExprConst(globalLb),
				   new IndexExprConst(globalHb));
    }

  case GET_GROUP_ID:
    {
      if (dimindx >= ndRange.get_work_dim())
	return new IndexExprConst(0);

      // global id bounds
      long globalLb = ndRange.getOffset(dimindx);
      long globalHb = ndRange.get_global_size(dimindx) - 1
	+ ndRange.getOffset(dimindx);

      // group id bounds
      long groupLb = globalLb / ndRange.get_local_size(dimindx);
      long groupHb = globalHb / ndRange.get_local_size(dimindx);

      // Apply guards
      for (unsigned i=0; i<guards.size(); ++i) {
	if (guards[i]->getDim() != dimindx)
	  continue;

	switch (guards[i]->getOclFunc()) {
	case GET_GLOBAL_ID:
	  applyGuard(guards[i], &globalLb, &globalHb);
	  break;

	case GET_GROUP_ID:
	  applyGuard(guards[i], &groupLb, &groupHb);
	  break;
	};
      }

      long newLb = globalLb / ndRange.get_local_size(dimindx);
      long newHb = globalHb / ndRange.get_local_size(dimindx);

      groupLb = newLb > groupLb ? newLb : groupLb;
      groupHb = newHb < groupHb ? newHb : groupHb;

      return new IndexExprInterval(new IndexExprConst(groupLb),
				   new IndexExprConst(groupHb));
    }

  case GET_LOCAL_ID:
    {
      if (dimindx >= ndRange.get_work_dim())
	return new IndexExprConst(0);

      long globalLb = ndRange.getOffset(dimindx);
      long globalHb= ndRange.get_global_size(dimindx) - 1
	+ ndRange.getOffset(dimindx);

      long localLb = 0;
      long localHb = ndRange.get_local_size(dimindx) - 1;

      // Apply guards
      for (unsigned i=0; i<guards.size(); ++i) {
	if (guards[i]->getDim() != dimindx)
	  continue;

	switch (guards[i]->getOclFunc()) {
	case GET_GLOBAL_ID:
	  applyGuard(guards[i], &globalLb, &globalHb);
	  break;

	case GET_LOCAL_ID:
	  applyGuard(guards[i], &localLb, &localHb);
	  break;
	};
      }

      long newLb = globalLb % ndRange.get_local_size(dimindx);
      long newHb = globalHb % ndRange.get_local_size(dimindx);
      localLb = newLb > localLb ? newLb : localLb;
      localHb = newHb < localHb ? newHb : localHb;

      return new IndexExprInterval(new IndexExprConst(localLb),
				   new IndexExprConst(localHb));
    }

  case GET_GLOBAL_SIZE:
    if (dimindx >= ndRange.get_work_dim())
      return new IndexExprConst(1);

    return new IndexExprConst(ndRange.get_orig_global_size(dimindx));

  case GET_LOCAL_SIZE:
    if (dimindx >= ndRange.get_work_dim())
      return new IndexExprConst(1);

    return new IndexExprConst(ndRange.get_local_size(dimindx));

  case GET_NUM_GROUPS:
    {
      if (dimindx >= ndRange.get_work_dim())
	return new IndexExprConst(1);

      long nbGroups = ndRange.get_orig_global_size(dimindx) /
	ndRange.get_local_size(dimindx);
      return new IndexExprConst(nbGroups);
    }

  default:
    return new IndexExprUnknown("ocl func");
  }
}

void
IndexExprOCL::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  switch(oclFunc) {
  case GET_GLOBAL_ID:
    stream << "get_global_id";
    break;
  case GET_LOCAL_ID:
    stream << "get_local_id";
    break;
  case GET_GLOBAL_SIZE:
    stream << "get_global_size";
    break;
  case GET_LOCAL_SIZE:
    stream << "get_local_size";
    break;
  case GET_GROUP_ID:
    stream << "get_group_id";
    break;
  default:
    stream << "unknown";
    break;
  };

  stream << " \"];\n";

  if (arg) {
    arg->toDot(stream);

    stream << id << " -> " << arg->getID() << " [label=\"\"];\n";
  }
}

unsigned
IndexExprOCL::getOCLFunc() const {
  return oclFunc;
}

const IndexExpr *
IndexExprOCL::getArg() const {
  return arg;
}

IndexExpr *
IndexExprOCL::getArg() {
  return arg;
}

void
IndexExprOCL::setArg(IndexExpr *expr) {
  arg = expr;
}

void
IndexExprOCL::write(std::stringstream &s) const {
  IndexExpr::write(s);
  s.write(reinterpret_cast<const char*>(&oclFunc), sizeof(oclFunc));
  if (arg)
    arg->write(s);
  else
    writeNIL(s);
}
