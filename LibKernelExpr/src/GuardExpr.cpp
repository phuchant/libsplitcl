#include "GuardExpr.h"

#include  "Indirection.h"

#include "IndexExpr/IndexExprConst.h"
#include "IndexExpr/IndexExprOCL.h"

#include <iostream>

GuardExpr::GuardExpr(unsigned oclFunc, unsigned dim, unsigned pred, bool truth,
		     IndexExpr *expr)
  : mOclFunc(oclFunc), mDim(dim), mExpr(expr)
{
  if (truth)
    mPred = pred;
  else {
    switch (pred) {
    case LT:
      mPred = GE;
      break;
    case LE:
      mPred = GT;
      break;
    case GE:
      mPred = LT;
      break;
    case GT:
      mPred = LE;
      break;
    case EQ:
      mPred = NEQ;
      break;
    case NEQ:
      mPred = EQ;
      break;
    default:
      exit(EXIT_FAILURE);
    };
  }
  mValueComputed = false;
  mIntExpr = 0;
}

GuardExpr::~GuardExpr() {
  delete mExpr;
}

long
GuardExpr::getIntExpr(long *intExpr) const {
  if (!mValueComputed)
    return 0;

  *intExpr = mIntExpr;
  return 1;
}

unsigned
GuardExpr::getPredicate() const {
  return mPred;
}

unsigned
GuardExpr::getDim() const {
  return mDim;
}

unsigned
GuardExpr::getOclFunc() const {
  return mOclFunc;
}

void
GuardExpr::dump() const {
  switch(mOclFunc) {
  case GET_GLOBAL_ID:
    std::cerr << "get_global_id(" << mDim << ") ";
    break;
  case GET_LOCAL_ID:
    std::cerr << "get_local_id(" << mDim << ") ";
    break;
  case GET_GLOBAL_SIZE:
    std::cerr << "get_global_size(" << mDim << ") ";
    break;
  case GET_LOCAL_SIZE:
    std::cerr << "get_local_size(" << mDim << ") ";
    break;
  case GET_GROUP_ID:
    std::cerr << "get_group_id(" << mDim << ") ";
    break;
  case GET_NUM_GROUPS:
    std::cerr << "get_num_groups(" << mDim << ") ";
    break;
  default:
    std::cerr << "oclfunc(" << mDim << ") ";
  };

  switch(mPred) {
  case LT:
    std::cerr << "<";
    break;
  case LE:
    std::cerr << "<=";
    break;
  case GE:
    std::cerr << ">=";
    break;
  case GT:
    std::cerr << ">";
    break;
  case EQ:
    std::cerr << "==";
    break;
  case NEQ:
    std::cerr << "!=";
    break;
  default:
    std::cerr << "?";
  };

  mExpr->dump();
  std::cerr << "\n";
}

GuardExpr *
GuardExpr::clone() const {
  return new GuardExpr(mOclFunc, mDim, mPred, true, mExpr->clone());
}

// Inject argument integer values in the Index Expression and try to compute the
// integer value of the expression.
void
GuardExpr::injectArgsValues(const std::vector<int> &values,
			    const NDRange &kernelNDRange) {
  IndexExpr::injectArgsValues(mExpr, values);
  std::vector<IndirectionValue> indirValues;
  std::vector<GuardExpr *> guards;

  IndexExpr *kernelExpr = mExpr->getKernelExpr(kernelNDRange, guards,
					       indirValues);
  long lb, hb;
  if (!IndexExpr::computeBounds(kernelExpr, &lb, &hb) || lb != hb) {
    mValueComputed = false;
  } else {
    mValueComputed = true;
    mIntExpr = lb;
  }

  delete kernelExpr;
}

void
GuardExpr::write(std::stringstream &s) const {
  s.write(reinterpret_cast<const char *>(&mOclFunc), sizeof(mOclFunc));
  s.write(reinterpret_cast<const char *>(&mDim), sizeof(mDim));
  s.write(reinterpret_cast<const char *>(&mPred),sizeof(mPred));
  mExpr->write(s);
}

void
GuardExpr::writeToFile(const std::string &name) const {
  std::ofstream out(name.c_str(), std::ofstream::trunc | std::ofstream::binary);
  std::stringstream ss;
  write(ss);
  out << ss;
  out.close();
}

GuardExpr *
GuardExpr::open(std::stringstream &s) {
  unsigned oclFunc;
  s.read(reinterpret_cast<char *>(&oclFunc), sizeof(oclFunc));
  unsigned dim;
  s.read(reinterpret_cast<char *>(&dim), sizeof(dim));
  unsigned pred;
  s.read(reinterpret_cast<char *>(&pred), sizeof(pred));
  IndexExpr *expr = IndexExpr::open(s);

  return new GuardExpr(oclFunc, dim, pred, true, expr);
}

GuardExpr *
GuardExpr::openFromFile(const std::string &name) {
  std::ifstream in(name.c_str(), std::ifstream::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  in.close();
  GuardExpr *ret = open(ss);
  return ret;
}
