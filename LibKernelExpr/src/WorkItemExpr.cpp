#include "IndexExpr/IndexExprOCL.h"
#include "IndexExpr/IndexExprValue.h"
#include "NDRange.h"
#include "WorkItemExpr.h"

#include <cassert>
#include <iostream>

WorkItemExpr::WorkItemExpr(const IndexExpr &wiExpr,
			   const std::vector<GuardExpr *> &guards) {
  mGuards = new std::vector<GuardExpr *>();
  mWiExpr = wiExpr.clone();

  for (unsigned i=0; i<guards.size(); ++i)
    mGuards->push_back(guards[i]->clone());
}

WorkItemExpr::WorkItemExpr(IndexExpr *wiExpr, std::vector<GuardExpr *> *guards)
  : mWiExpr(wiExpr), mGuards(guards) {
}

WorkItemExpr::~WorkItemExpr() {
  delete mWiExpr;
  for (unsigned i=0; i<mGuards->size(); ++i)
    delete (*mGuards)[i];
  delete mGuards;
}

WorkItemExpr::WorkItemExpr(const WorkItemExpr &expr) {
  WorkItemExpr(*expr.mWiExpr, *expr.mGuards);
}

void
WorkItemExpr::injectArgsValues(const std::vector<IndexExprValue *> &values,
			       const NDRange &kernelNDRange) {
  IndexExpr::injectArgsValues(mWiExpr, values);

  for (unsigned i=0; i<mGuards->size(); ++i)
    (*mGuards)[i]->injectArgsValues(values, kernelNDRange);
}

IndexExpr *
WorkItemExpr::getKernelExpr(const NDRange &kernelNDRange,
			    const std::vector<IndirectionValue> &indirValues)
  const {
  if (isOutOfGuards(kernelNDRange))
    return NULL;

  // TODO: Handle guards with indirections.

  return mWiExpr->getKernelExpr(kernelNDRange, *mGuards, indirValues);
}

WorkItemExpr *
WorkItemExpr::clone() const {
  return new WorkItemExpr(*mWiExpr, *mGuards);
}

void
WorkItemExpr::dump() const {
  std::cerr << "workitem expr :\n";
  if (mWiExpr)
    mWiExpr->dump();
  else
    std::cerr << "NULL\n";
  std::cerr << "\nguards :\n";
  for (unsigned i=0; i<mGuards->size(); ++i)
    (*mGuards)[i]->dump();
  std::cerr << "\n";
}

void
WorkItemExpr::write(std::stringstream &s) const {
  mWiExpr->write(s);
  unsigned size = mGuards->size();
  s.write(reinterpret_cast<const char *>(&size), sizeof(size));
  for (unsigned i=0; i<mGuards->size(); ++i)
    (*mGuards)[i]->write(s);
}

void
WorkItemExpr::writeToFile(const std::string &name) const {
  std::ofstream out(name.c_str(), std::ofstream::trunc | std::ofstream::binary);
  std::stringstream ss;
  write(ss);
  out << ss;
  out.close();
}

WorkItemExpr *
WorkItemExpr::open(std::stringstream &s) {
  IndexExpr *expr = IndexExpr::open(s);

  unsigned nbGuards;
  s.read(reinterpret_cast<char *>(&nbGuards), sizeof(nbGuards));

  std::vector<GuardExpr *> *guards = new std::vector<GuardExpr *>();
  for (unsigned i=0; i<nbGuards; ++i)
    guards->push_back(GuardExpr::open(s));

  return  new WorkItemExpr(expr, guards);
}

WorkItemExpr *
WorkItemExpr::openFromFile(const std::string &name) {
  std::ifstream in(name.c_str(), std::ifstream::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  in.close();
  return open(ss);
}

bool
WorkItemExpr::isOutOfGuards(const NDRange &kernelNDRange) const {
    // Check if KernelExpr if out of guards
  // ex: guard: global_id(0) = 0
  // global_size(0) = 1024
  // global_offset(0) = 1024
  // => global_id(0) = [1024, 2047] != 0

  for (unsigned i=0; i<mGuards->size(); ++i) {
    unsigned guardDim = (*mGuards)[i]->getDim();
    if (guardDim >= kernelNDRange.get_work_dim())
      continue;

    long guardValue;
    if (!(*mGuards)[i]->getIntExpr(&guardValue))
      continue;

    // global id bounds
    long globalLb = kernelNDRange.getOffset(guardDim);
    long globalHb = kernelNDRange.get_global_size(guardDim) - 1
      + kernelNDRange.getOffset(guardDim);

    // group id bounds
    long groupLb = globalLb / kernelNDRange.get_local_size(guardDim);
    long groupHb = globalHb / kernelNDRange.get_local_size(guardDim);

    // local id bounds
    long localLb = 0;
    long localHb = kernelNDRange.get_local_size(guardDim) - 1;

    switch((*mGuards)[i]->getOclFunc()) {
    case IndexExprOCL::GET_GLOBAL_ID:
      {
	switch ((*mGuards)[i]->getPredicate()) {
	case GuardExpr::LT:
	  {
	    if (globalLb >= guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::LE:
	  {
	    if (globalLb > guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::GE:
	  {
	    if (globalHb < guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::GT:
	  {
	    if (globalHb <= guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::EQ:
	  {
	    if (! ( guardValue >= globalLb &&
		    guardValue <= globalHb))
	      return true;
	    break;
	  }
	case GuardExpr::NEQ:
	  std::cerr << "warning: Guard predicate NEQ not handled !\n";
	  return false;
	};

	break;
      }

    case IndexExprOCL::GET_GROUP_ID:
      {
	switch((*mGuards)[i]->getPredicate()) {
	case GuardExpr::LT:
	  {
	    if (groupLb >= guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::LE:
	  {
	    if (groupLb > guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::GE:
	  {
	    if (groupHb < guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::GT:
	  {
	    if (groupHb <= guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::EQ:
	  {
	    if (! (guardValue >= groupLb &&
		   guardValue <= groupHb))
	      return true;
	    break;
	  }
	case GuardExpr::NEQ:
	  std::cerr << "warning: Guard predicate NEQ not handled !\n";
	  return false;
	};

	break;
      }

    case IndexExprOCL::GET_LOCAL_ID:
      {
	switch ((*mGuards)[i]->getPredicate()) {
	case GuardExpr::LT:
	  {
	    if (localLb >= guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::LE:
	  {
	    if (localLb > guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::GE:
	  {
	    if (localHb < guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::GT:
	  {
	    if (localHb <= guardValue)
	      return true;
	    break;
	  }

	case GuardExpr::EQ:
	  {
	    if (! (guardValue >= localLb &&
		   guardValue <= localHb))
	      return true;
	    break;
	  }
	case GuardExpr::NEQ:
	  std::cerr << "warning: Guard predicate NEQ not handled !\n";
	  return false;
	};

	break;
      }
    };
  }

  return false;
}
