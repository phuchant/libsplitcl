#include "IndexExpr/IndexExpr.h"

#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprBinop.h"
#include "IndexExpr/IndexExprConst.h"
#include "IndexExpr/IndexExprLB.h"
#include "IndexExpr/IndexExprHB.h"
#include "IndexExpr/IndexExprIndirection.h"
#include "IndexExpr/IndexExprInterval.h"
#include "IndexExpr/IndexExprMax.h"
#include "IndexExpr/IndexExprOCL.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <cassert>
#include <iostream>
#include <fstream>

unsigned IndexExpr::idIndex = 0;

#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define MIN(A,B) ((A) < (B) ? (A) : (B))

IndexExpr::IndexExpr(unsigned tag)
  : tag(tag), id(idIndex++) {}

IndexExpr::~IndexExpr() {}

unsigned
IndexExpr::getTag() const {
  return tag;
}

IndexExpr *
IndexExpr::getWorkgroupExpr(const NDRange &ndRange) const {
  (void) ndRange;

  return clone();
}

IndexExpr *
IndexExpr::getKernelExpr(const NDRange &ndRange,
			 const std::vector<GuardExpr *> & guards,
			 const std::vector<IndirectionValue> &
			 indirValues) const {
  (void) ndRange;
  (void) guards;
  (void) indirValues;

  return clone();
}

unsigned
IndexExpr::getID() const {
  return id;
}

void
IndexExpr::toDot(std::string filename) const {
  std::ofstream out;
  out.open(filename.c_str());
  std::stringstream stream;
  toDot(stream);
  out << "digraph Expression {";
  out << "graph [fontname=\"fixed\"];\n";
  out << "node [fontname=\"fixed\"];\n";
  out << "edge [fontname=\"fixed\"];\n";
  out << stream.str();
  out << "}";
  out.close();
}

void
IndexExpr::toDot(std::stringstream &stream) const {
  stream << id << " [label=\"";

  switch (tag) {
  case IndexExpr::CONST:
    stream << "const";
    break;
  case IndexExpr::ARG:
    stream << "arg";
  case IndexExpr::OCL:
    stream << "ocl";
    break;
  case IndexExpr::BINOP:
    stream << "binop";
    break;
  case IndexExpr::INTERVAL:
    stream << "interval";
    break;
  case IndexExpr::INDIR:
    stream << "interval";
    break;
  case IndexExpr::UNKNOWN:
    stream << "unknown";
    break;
  case IndexExpr::MAX:
    stream << "max";
    break;
  case IndexExpr::LB:
    stream << "lb";
    break;
  case IndexExpr::HB:
    stream << "hb";
    break;
  }
}

void
IndexExpr::write(std::stringstream &s) const {
  s.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
}

void
IndexExpr::writeNIL(std::stringstream &s) const {
  unsigned nilTag = NIL;
  s.write(reinterpret_cast<const char*>(&nilTag), sizeof(nilTag));
}

void
IndexExpr::writeToFile(const std::string &name) const {
  std::ofstream out(name.c_str(), std::ofstream::trunc | std::ofstream::binary);
  std::stringstream ss;
  write(ss);
  out << ss;
  out.close();
}

IndexExpr *
IndexExpr::open(std::stringstream &s) {
  unsigned file_tag;
  s.read(reinterpret_cast<char *>(&file_tag), sizeof(file_tag));
  switch(file_tag) {
  case NIL:
    return NULL;
    break;
  case CONST:
    {
      long intValue;
      s.read(reinterpret_cast<char *>(&intValue), sizeof(intValue));
      return new IndexExprConst(intValue);
    }
  case ARG:
    {
      unsigned pos;
      s.read(reinterpret_cast<char *>(&pos), sizeof(pos));
      return new IndexExprArg("arg", pos);
    }
  case OCL:
    {
      unsigned oclFunc;
      s.read(reinterpret_cast<char *>(&oclFunc), sizeof(oclFunc));
      IndexExpr *arg = open(s);
      return new IndexExprOCL(oclFunc, arg);
    }
  case BINOP:
    {
      unsigned op;
      s.read(reinterpret_cast<char *>(&op), sizeof(op));
      IndexExpr *expr1 = open(s);
      IndexExpr *expr2 = open(s);
      return new IndexExprBinop(op, expr1, expr2);
    }
  case INTERVAL:
    {
      IndexExpr *lb = open(s);
      IndexExpr *hb = open(s);
      return new IndexExprInterval(lb, hb);
    }
  case INDIR:
    {
      unsigned no;
      s.read(reinterpret_cast<char *>(&no), sizeof(no));
      IndexExpr *lb = open(s);
      IndexExpr *hb = open(s);
      return new IndexExprIndirection(no, lb, hb);
    }
  case UNKNOWN:
    return new IndexExprUnknown("unknown");
  case MAX:
    {
      unsigned numOperands;
      s.read(reinterpret_cast<char *>(&numOperands), sizeof(numOperands));
      IndexExpr *exprs[numOperands];
      for (unsigned i=0; i<numOperands; i++)
	exprs[i] = open(s);
      return new IndexExprMax(numOperands, exprs);
    }
  case LB:
    {
      IndexExpr *expr = open(s);
      return new IndexExprLB(expr);
    }
  case HB:
    {
      IndexExpr *expr = open(s);
      return new IndexExprHB(expr);
    }
  };

  return NULL;
}

IndexExpr *
IndexExpr::openFromFile(const std::string &name) {
  std::ifstream in(name.c_str(), std::ifstream::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  in.close();
  IndexExpr *ret = open(ss);
  return ret;
}

void
IndexExpr::injectArgsValues(IndexExpr *expr, const std::vector<int> &values) {

  if (!expr)
    return;

  switch (expr->getTag()) {
  case ARG:
    {
      IndexExprArg *argExpr = static_cast<IndexExprArg *>(expr);
      if (values.size() > argExpr->getPos())
	argExpr->setValue(values[argExpr->getPos()]);
      return;
    }

  case OCL:
    {
      IndexExprOCL *oclExpr = static_cast<IndexExprOCL *>(expr);
      IndexExpr *arg = oclExpr->getArg();
      injectArgsValues(arg, values);
      return;
    }

  case BINOP:
    {
      IndexExprBinop *binExpr = static_cast<IndexExprBinop *>(expr);
      IndexExpr *expr1 = binExpr->getExpr1();
      injectArgsValues(expr1, values);
      IndexExpr *expr2 = binExpr->getExpr2();
      injectArgsValues(expr2, values);
      return;
    }

  case INTERVAL:
    {
      IndexExprInterval *intervalExpr = static_cast<IndexExprInterval *>(expr);
      IndexExpr *lb = intervalExpr->getLb();
      injectArgsValues(lb, values);
      IndexExpr *hb = intervalExpr->getHb();
      injectArgsValues(hb, values);
      return;
    }
  case INDIR:
    {
      IndexExprIndirection *indirExpr
	= static_cast<IndexExprIndirection *>(expr);
      IndexExpr *lb = indirExpr->getLb();
      injectArgsValues(lb, values);
      IndexExpr *hb = indirExpr->getHb();
      injectArgsValues(hb, values);
      return;
    }
  case MAX:
    {
      IndexExprMax *maxExpr = static_cast<IndexExprMax *>(expr);
      unsigned numOperands = maxExpr->getNumOperands();
      for (unsigned i=0; i<numOperands; i++) {
	IndexExpr *current = maxExpr->getExprN(i);
	injectArgsValues(current, values);
      }
      return;
    }
  case LB:
    {
      IndexExprLB *lbExpr = static_cast<IndexExprLB *>(expr);
      IndexExpr *expr = lbExpr->getExpr();
      injectArgsValues(expr, values);
      return;
    }
  case HB:
    {
      IndexExprHB *hbExpr = static_cast<IndexExprHB *>(expr);
      IndexExpr *expr = hbExpr->getExpr();
      injectArgsValues(expr, values);
      return;
    }
  };
}

void
IndexExpr::injectIndirValues(IndexExpr *expr,
			     const std::vector<std::pair<int, int> >&values) {

  if (!expr)
    return;

  switch (expr->getTag()) {
  case ARG:
    return;

  case OCL:
    {
      IndexExprOCL *oclExpr = static_cast<IndexExprOCL *>(expr);
      IndexExpr *arg = oclExpr->getArg();
      injectIndirValues(arg, values);
      return;
    }

  case BINOP:
    {
      IndexExprBinop *binExpr = static_cast<IndexExprBinop *>(expr);
      IndexExpr *expr1 = binExpr->getExpr1();
      injectIndirValues(expr1, values);
      IndexExpr *expr2 = binExpr->getExpr2();
      injectIndirValues(expr2, values);
      return;
    }

  case INTERVAL:
    {
      IndexExprInterval *intervalExpr = static_cast<IndexExprInterval *>(expr);
      IndexExpr *lb = intervalExpr->getLb();
      injectIndirValues(lb, values);
      IndexExpr *hb = intervalExpr->getHb();
      injectIndirValues(hb, values);
      return;
    }
  case INDIR:
    {
      IndexExprIndirection *indirExpr
	= static_cast<IndexExprIndirection *>(expr);
      unsigned no = indirExpr->getNo();
      assert(no < values.size());
      delete indirExpr->lb;
      delete indirExpr->hb;
      indirExpr->lb = new IndexExprConst(values[no].first);
      indirExpr->hb = new IndexExprConst(values[no].second);
      return;
    }
  case MAX:
    {
      IndexExprMax *maxExpr = static_cast<IndexExprMax *>(expr);
      unsigned numOperands = maxExpr->getNumOperands();
      for (unsigned i=0; i<numOperands; i++) {
	IndexExpr *current = maxExpr->getExprN(i);
	injectIndirValues(current, values);
      }
      return;
    }
  case LB:
    {
      IndexExprLB *lbExpr = static_cast<IndexExprLB *>(expr);
      IndexExpr *expr = lbExpr->getExpr();
      injectIndirValues(expr, values);
      return;
    }
  case HB:
    {
      IndexExprHB *hbExpr = static_cast<IndexExprHB *>(expr);
      IndexExpr *expr = hbExpr->getExpr();
      injectIndirValues(expr, values);
      return;
    }
  };
}

bool
IndexExpr::computeBounds(const IndexExpr *expr, long *lb, long *hb) {
  if (!expr)
    return false;

  switch (expr->getTag()) {
  case ARG:
  case CONST:
    {
    const IndexExprValue *valueExpr = static_cast<const IndexExprValue *>(expr);
    long value;
    if (!valueExpr->getValue(&value)) {
      std::cerr << "value not set!\n";
      return false;
    }
    *lb = value;
    *hb = value;
    assert(*lb <= *hb);
    return true;
    }

  case BINOP:
    {
      const IndexExprBinop *binExpr = static_cast<const IndexExprBinop *>(expr);
      const IndexExpr *expr1 = binExpr->getExpr1();
      const IndexExpr *expr2 = binExpr->getExpr2();

      long lb1, hb1, lb2, hb2;
      if (!computeBounds(expr1, &lb1, &hb1) ||
	  !computeBounds(expr2, &lb2, &hb2))
	return false;

      assert(lb1 <= hb1);
      assert(lb2 <= hb2);

      switch (binExpr->getOp()) {

	// See https://en.wikipedia.org/wiki/Interval_arithmetic

      case IndexExprBinop::Add:
	{
	  *lb = lb1 + lb2;
	  *hb = hb1 + hb2;
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::Sub:
	{
	  *lb = lb1 - hb2;
	  *hb = hb1 - lb2;
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::Mul:
	{
	  *lb = MIN( MIN(lb1*lb2,lb1*hb2) , MIN(hb1*lb2, hb1*hb2) );
	  *hb = MAX( MAX(lb1*lb2,lb1*hb2) , MAX(hb1*lb2, hb1*hb2) );
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::Div:
	{
	  // First compute (1 / [lb2, hb2]) = (1/hb2, 1/lb2)
	  double invLb = 1.0 / hb2;
	  double invHb = 1.0 / lb2;

	  // Then res = [lb1,hb1] * [invLb, invHb]
	  *lb = MIN( MIN(lb1*invLb,lb1*invHb) , MIN(hb1*invLb, hb1*invHb) );
	  *hb = MAX( MAX(lb1*invLb,lb1*invHb) , MAX(hb1*invLb, hb1*invHb) );
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::Mod:
	{
	  *lb = (hb1 >= lb2) ? 0 : lb1;
	  *hb = (hb1 >= hb2) ? hb2-1 : hb1;
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::Or:
	{
	  *lb = lb1 | lb2;
	  *hb = hb1 | hb2;
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::Xor:
	{
	  *lb = lb1 ^ lb2;
	  *hb = hb1 ^ hb2;
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::And:
	{
	  *lb = lb1 & lb2;
	  *hb = hb1 & hb2;
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::Shl:
	{
	  *lb = MIN( MIN(lb1<<lb2,lb1<<hb2) , MIN(hb1<<lb2, hb1<<hb2) );
	  *hb = MAX( MAX(lb1<<lb2,lb1<<hb2) , MAX(hb1<<lb2, hb1<<hb2) );
	  assert(*lb <= *hb);
	  return true;
	}

      case IndexExprBinop::Shr:
	{
	  *lb = MIN( MIN(lb1>>lb2,lb1>>hb2) , MIN(hb1>>lb2, hb1>>hb2) );
	  *hb = MAX( MAX(lb1>>lb2,lb1>>hb2) , MAX(hb1>>lb2, hb1>>hb2) );
	  assert(*lb <= *hb);
	  return true;
	}

      default:
	return false;
      };
    }

  case INTERVAL:
    {
      const IndexExprInterval *intervalExpr
	= static_cast<const IndexExprInterval *>(expr);
      const IndexExpr *expr1 = intervalExpr->getLb();
      const IndexExpr *expr2 = intervalExpr->getHb();

      long lb1, hb1, lb2, hb2;
      if (!computeBounds(expr1, &lb1, &hb1) ||
	  !computeBounds(expr2, &lb2, &hb2))
	return false;

      assert(lb1 <= hb1);
      assert(lb2 <= hb2);

      *lb = MIN(lb1, lb2);
      *hb = MAX(hb1, hb2);
      assert(*lb <= *hb);
      return true;
    }

  case INDIR:
    {
      const IndexExprIndirection *indirExpr
	= static_cast<const IndexExprIndirection *>(expr);
      const IndexExpr *expr1 = indirExpr->getLb();
      const IndexExpr *expr2 = indirExpr->getHb();

      long lb1, hb1, lb2, hb2;
      if (!computeBounds(expr1, &lb1, &hb1) ||
	  !computeBounds(expr2, &lb2, &hb2))
	return false;

      assert(lb1 <= hb1);
      assert(lb2 <= hb2);

      *lb = MIN(lb1, lb2);
      *hb = MAX(hb1, hb2);
      assert(*lb <= *hb);
      return true;
    }

  case MAX:
    {
      const IndexExprMax *maxExpr = static_cast<const IndexExprMax *>(expr);
      unsigned numOperands = maxExpr->getNumOperands();
      long lbs[numOperands];
      long hbs[numOperands];

      for (unsigned i=0; i<numOperands; i++) {
	if (!computeBounds(maxExpr->getExprN(i), &lbs[i], &hbs[i]))
	  return false;
	assert(lbs[i] <= hbs[i]);
      }

      long maxHb = hbs[0];
      long maxLb = lbs[0];
      for (unsigned i=1; i<numOperands; i++) {
	maxHb = MAX(hbs[i], maxHb);
	maxLb = MAX(lbs[i], maxLb);
      }

      *lb = maxLb;
      *hb = maxHb;
      assert(*lb <= *hb);
      return true;
    }
  case LB:
    {
      const IndexExprLB *lbExpr = static_cast<const IndexExprLB *>(expr);
      long lbL, hbL;

      if (!computeBounds(lbExpr->getExpr(), &lbL, &hbL))
	  return false;
      assert(lbL <= hbL);


      *lb = lbL;
      *hb = lbL;
      assert(*lb <= *hb);
      return true;
    }
  case HB:
    {
      const IndexExprHB *hbExpr = static_cast<const IndexExprHB *>(expr);
      long lbH, hbH;

      if (!computeBounds(hbExpr->getExpr(), &lbH, &hbH))
	  return false;
      assert(lbH <= hbH);


      *lb = hbH;
      *hb = hbH;
      assert(*lb <= *hb);
      return true;
    }

  default:
    return false;
  };
}

