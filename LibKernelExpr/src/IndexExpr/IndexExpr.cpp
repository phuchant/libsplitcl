#include "IndexExpr/IndexExpr.h"

#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprBinop.h"
#include "IndexExpr/IndexExprCast.h"
#include "IndexExpr/IndexExprLB.h"
#include "IndexExpr/IndexExprHB.h"
#include "IndexExpr/IndexExprIndirection.h"
#include "IndexExpr/IndexExprInterval.h"
#include "IndexExpr/IndexExprMin.h"
#include "IndexExpr/IndexExprMax.h"
#include "IndexExpr/IndexExprOCL.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>

unsigned IndexExpr::idIndex = 0;

#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define MIN(A,B) ((A) < (B) ? (A) : (B))

IndexExpr::IndexExpr(tagTy tag)
  : tag(tag), id(idIndex++) {}

IndexExpr::~IndexExpr() {}

IndexExpr::tagTy
IndexExpr::getTag() const {
  return tag;
}

IndexExpr *
IndexExpr::getWorkgroupExpr(const NDRange &ndRange) const {
  (void) ndRange;

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
  case IndexExpr::NIL:
    stream << "NIL";
    break;
  case IndexExpr::VALUE:
    stream << "value";
    break;
  case IndexExpr::CAST:
    stream << "cast";
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
  case IndexExpr::UNKNOWN:
    stream << "unknown";
    break;
  case IndexExpr::MIN:
    stream << "min";
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
  case IndexExpr::INDIR:
    stream << "interval";
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
  tagTy file_tag;
  s.read(reinterpret_cast<char *>(&file_tag), sizeof(file_tag));
  switch(file_tag) {
  case NIL:
    return NULL;
    break;
  case VALUE:
    {
      value_type valTy;
      s.read(reinterpret_cast<char *>(&valTy), sizeof(valTy));
      switch (valTy) {
      case LONG:
	{
	  long val;
	  s.read(reinterpret_cast<char *>(&val), sizeof(val));
	  return IndexExprValue::createLong(val);

	}
      case FLOAT:
	{
	  float val;
	  s.read(reinterpret_cast<char *>(&val), sizeof(val));
	  return IndexExprValue::createFloat(val);
	}
      case DOUBLE:
	{
	  double val;
	  s.read(reinterpret_cast<char *>(&val), sizeof(val));
	  return IndexExprValue::createDouble(val);
	}
      };
    }
  case CAST:
    {
      IndexExprCast::castTy c;
      s.read(reinterpret_cast<char *>(&c), sizeof(c));
      IndexExpr *expr = open(s);
      return new IndexExprCast(expr, c);
    }
  case ARG:
    {
      unsigned pos;
      s.read(reinterpret_cast<char *>(&pos), sizeof(pos));
      return new IndexExprArg("arg", pos);
    }
  case OCL:
    {
      IndexExprOCL::OpenclFunction oclFunc;
      s.read(reinterpret_cast<char *>(&oclFunc), sizeof(oclFunc));
      IndexExpr *arg = open(s);
      return new IndexExprOCL(oclFunc, arg);
    }
  case BINOP:
    {
      IndexExprBinop::BinOp op;
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
  case UNKNOWN:
    return new IndexExprUnknown("unknown");
  case MIN:
    {
      unsigned numOperands;
      s.read(reinterpret_cast<char *>(&numOperands), sizeof(numOperands));
      IndexExpr *exprs[numOperands];
      for (unsigned i=0; i<numOperands; i++)
	exprs[i] = open(s);
      return new IndexExprMin(numOperands, exprs);
    }
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
  case INDIR:
    {
      unsigned no;
      s.read(reinterpret_cast<char *>(&no), sizeof(no));
      IndexExpr *lb = open(s);
      IndexExpr *hb = open(s);
      return new IndexExprIndirection(no, lb, hb);
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
IndexExpr::injectArgsValues(IndexExpr *expr, const std::vector<IndexExprValue *> &values) {

  if (!expr)
    return;

  switch (expr->getTag()) {
  case NIL:
  case VALUE:
  case UNKNOWN:
    /* Do nothin */
    return;
  case CAST:
    {
      IndexExprCast *castExpr = static_cast<IndexExprCast *>(expr);
      injectArgsValues(castExpr->getExpr(), values);
      return;
    }
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
  case MIN:
    {
      IndexExprMin *minExpr = static_cast<IndexExprMin *>(expr);
      unsigned numOperands = minExpr->getNumOperands();
      for (unsigned i=0; i<numOperands; i++) {
	IndexExpr *current = minExpr->getExprN(i);
	injectArgsValues(current, values);
      }
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
  };
}

void
IndexExpr::injectIndirValues(IndexExpr *expr,
			     const std::vector<std::pair<IndexExprValue *,
							 IndexExprValue *>>
			       &values) {

  if (!expr)
    return;

  switch (expr->getTag()) {
  case NIL:
  case VALUE:
  case ARG:
  case UNKNOWN:
    /* Do nothing */
    return;

  case CAST:
    {
      IndexExprCast *castExpr = static_cast<IndexExprCast *>(expr);
      injectIndirValues(castExpr->getExpr(), values);
    }

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
      indirExpr->lb = values[no].first->clone();
      indirExpr->hb = values[no].second->clone();
      return;
    }
  case MIN:
    {
      IndexExprMin *minExpr = static_cast<IndexExprMin *>(expr);
      unsigned numOperands = minExpr->getNumOperands();
      for (unsigned i=0; i<numOperands; i++) {
	IndexExpr *current = minExpr->getExprN(i);
	injectIndirValues(current, values);
      }
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
  value lb_val, hb_val;
  value_type type;

  if (!computeBoundsRec(expr, &lb_val, &hb_val, &type))
    return false;

  assert(type == LONG);

  *lb = lb_val.n;
  *hb = hb_val.n;

  return true;
}

template<typename T>
void computeBinopBounds(IndexExprBinop::BinOp op,
			T lb1, T hb1, T lb2, T hb2, T *lb, T *hb) {
  assert(lb1 <= hb1);
  assert(lb2 <= hb2);

  switch (op) {

    // See https://en.wikipedia.org/wiki/Interval_arithmetic

  case IndexExprBinop::Add:
    {
      *lb = lb1 + lb2;
      *hb = hb1 + hb2;
      assert(*lb <= *hb);
      return;
    }

  case IndexExprBinop::Sub:
    {
      *lb = lb1 - hb2;
      *hb = hb1 - lb2;
      assert(*lb <= *hb);
      return;
    }

  case IndexExprBinop::Mul:
    {
      *lb = MIN( MIN(lb1*lb2,lb1*hb2) , MIN(hb1*lb2, hb1*hb2) );
      *hb = MAX( MAX(lb1*lb2,lb1*hb2) , MAX(hb1*lb2, hb1*hb2) );
      assert(*lb <= *hb);
      return;
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
      return;
    }

  case IndexExprBinop::Mod:
    {
      *lb = (hb1 >= lb2) ? 0 : lb1;
      *hb = (hb1 >= hb2) ? hb2-1 : hb1;
      assert(*lb <= *hb);
      return;
    }

  case IndexExprBinop::Or:
    {
      *lb = (long) lb1 | (long) lb2;
      *hb = (long) hb1 | (long) hb2;
      assert(*lb <= *hb);
      return;
    }

  case IndexExprBinop::Xor:
    {
      *lb = (long) lb1 ^ (long) lb2;
      *hb = (long) hb1 ^ (long) hb2;
      assert(*lb <= *hb);
      return;
    }

  case IndexExprBinop::And:
    {
      *lb = (long) lb1 & (long) lb2;
      *hb = (long) hb1 & (long) hb2;
      assert(*lb <= *hb);
      return;
    }

  case IndexExprBinop::Shl:
    {
      *lb = MIN( MIN((long) lb1 << (long) lb2, (long) lb1 << (long) hb2),
		 MIN((long) hb1 << (long) lb2, (long) hb1 << (long) hb2) );
      *hb = MAX( MAX((long) lb1 << (long) lb2, (long) lb1 << (long) hb2),
		 MAX((long) hb1 << (long) lb2, (long) hb1 << (long) hb2) );
      assert(*lb <= *hb);
      return;
    }

  case IndexExprBinop::Shr:
    {
      *lb = MIN( MIN((long) lb1 >> (long)lb2, (long)lb1 >> (long)hb2) ,
		 MIN((long) hb1 >> (long)lb2, (long)hb1 >> (long)hb2) );
      *hb = MAX( MAX((long) lb1 >> (long)lb2, (long)lb1 >> (long)hb2) ,
		 MAX((long) hb1 >> (long)lb2, (long)hb1 >> (long)hb2) );
      assert(*lb <= *hb);
      return;
    }
  };
}

template<typename T>
void computeIntervalBounds(T lb1, T hb1, T lb2, T hb2, T *lb, T *hb) {
  assert(lb1 <= hb1);
  assert(lb2 <= hb2);

  *lb = MIN(lb1, lb2);
  *hb = MAX(hb1, hb2);
  assert(*lb <= *hb);
}

template<typename T>
void computeIndirBounds(T lb1, T hb1, T lb2, T hb2, T *lb, T *hb) {
  assert(lb1 <= hb1);
  assert(lb2 <= hb2);

  *lb = MIN(lb1, lb2);
  *hb = MAX(hb1, hb2);
  assert(*lb <= *hb);
}

template<typename T>
void computeMinBounds(T *lbs, T *hbs, int n, T *lb, T *hb) {
  T minHb = hbs[0];
  T minLb = lbs[0];
  for (int i=1; i<n; i++) {
    assert(lbs[i] <= hbs[i]);
    minHb = MIN(hbs[i], minHb);
    minLb = MIN(lbs[i], minLb);
  }

  *lb = minLb;
  *hb = minHb;
  assert(*lb <= *hb);
}

template<typename T>
void computeMaxBounds(T *lbs, T *hbs, int n, T *lb, T *hb) {
  T maxHb = hbs[0];
  T maxLb = lbs[0];
  for (int i=1; i<n; i++) {
    assert(lbs[i] <= hbs[i]);
    maxHb = MAX(hbs[i], maxHb);
    maxLb = MAX(lbs[i], maxLb);
  }

  *lb = maxLb;
  *hb = maxHb;
  assert(*lb <= *hb);
}

template<typename T>
void computeLbBounds(T lbL, T hbL, T *lb, T *hb) {
  assert(lbL <= hbL);
  *lb = lbL;
  *hb = lbL;
  assert(*lb <= *hb);
}

template<typename T>
void computeHbBounds(T lbH, T hbH, T *lb, T *hb) {
  assert(lbH <= hbH);
  *lb = hbH;
  *hb = hbH;
  assert(*lb <= *hb);
}

bool
IndexExpr::computeBoundsRec(const IndexExpr *expr, value *lb, value *hb,
			    value_type *type) {
  if (!expr)
    return false;

  switch (expr->getTag()) {
  case ARG:
    {
      const IndexExprArg *argExpr = static_cast<const IndexExprArg *>(expr);
      const IndexExprValue *valueExpr = argExpr->getValue();
      if (!valueExpr)
	return false;

    *type = valueExpr->type;
    switch(*type) {
    case LONG:
      lb->n = hb->n = valueExpr->getLongValue();
      break;
    case FLOAT:
      lb->f = hb->f = valueExpr->getFloatValue();
      break;
    case DOUBLE:
      lb->d = hb->d = valueExpr->getDoubleValue();
      break;
    }

    return true;
    }

  case VALUE:
    {
    const IndexExprValue *valueExpr = static_cast<const IndexExprValue *>(expr);
    *type = valueExpr->type;
    switch(*type) {
    case LONG:
      lb->n = hb->n = valueExpr->getLongValue();
      break;
    case FLOAT:
      lb->f = hb->f = valueExpr->getFloatValue();
      break;
    case DOUBLE:
      lb->d = hb->d = valueExpr->getDoubleValue();
      break;
    }

    return true;
    }

  case BINOP:
    {
      const IndexExprBinop *binExpr = static_cast<const IndexExprBinop *>(expr);
      const IndexExpr *expr1 = binExpr->getExpr1();
      const IndexExpr *expr2 = binExpr->getExpr2();

      value lb1, hb1, lb2, hb2;
      value_type t1, t2;

      if (!computeBoundsRec(expr1, &lb1, &hb1, &t1) ||
  	  !computeBoundsRec(expr2, &lb2, &hb2, &t2))
  	return false;

      assert(t1 == t2);

      int op = binExpr->getOp();

      if (t1 != LONG)
	assert(op != IndexExprBinop::Or &&
	       op != IndexExprBinop::Xor &&
	       op != IndexExprBinop::And &&
	       op != IndexExprBinop::Shl &&
	       op != IndexExprBinop::Shr);

      switch(t1) {
      case LONG:
	computeBinopBounds<long>(binExpr->getOp(), lb1.n, hb1.n,
				 lb2.n, hb2.n, &lb->n, &hb->n);
	//	assert(*lb.n <= *hb.n);
	break;
      case FLOAT:
	computeBinopBounds<float>(binExpr->getOp(), lb1.f, hb1.f,
				 lb2.f, hb2.f, &lb->f, &hb->f);
	break;
      case DOUBLE:
	computeBinopBounds<double>(binExpr->getOp(), lb1.d, hb1.d,
				 lb2.d, hb2.d, &lb->d, &hb->d);
	break;
      };

      *type = t1;
      return true;
    }

  case INTERVAL:
    {
      const IndexExprInterval *intervalExpr
  	= static_cast<const IndexExprInterval *>(expr);
      const IndexExpr *expr1 = intervalExpr->getLb();
      const IndexExpr *expr2 = intervalExpr->getHb();

      value lb1, hb1, lb2, hb2;
      value_type t1, t2;
      if (!computeBoundsRec(expr1, &lb1, &hb1, &t1) ||
  	  !computeBoundsRec(expr2, &lb2, &hb2, &t2))
  	return false;

      assert(t1 == t2);
      switch(t1) {
      case LONG:
	computeIntervalBounds<long>(lb1.n, hb1.n, lb2.n, hb2.n,
				    &lb->n, &hb->n);
	break;
      case FLOAT:
	computeIntervalBounds<float>(lb1.f, hb1.f, lb2.f, hb2.f,
				    &lb->f, &hb->f);
	break;
      case DOUBLE:
	computeIntervalBounds<double>(lb1.d, hb1.d, lb2.d, hb2.d,
				    &lb->d, &hb->d);
	break;
      };

      *type = t1;
      return true;
    }

  case INDIR:
    {
      const IndexExprIndirection *indirExpr
  	= static_cast<const IndexExprIndirection *>(expr);
      const IndexExpr *expr1 = indirExpr->getLb();
      const IndexExpr *expr2 = indirExpr->getHb();

      value lb1, hb1, lb2, hb2;
      value_type t1, t2;
      if (!computeBoundsRec(expr1, &lb1, &hb1, &t1) ||
  	  !computeBoundsRec(expr2, &lb2, &hb2, &t2))
  	return false;
      assert(t1 == t2);

      switch (t1) {
      case LONG:
	computeIntervalBounds<long>(lb1.n, hb1.n, lb2.n, hb2.n, &lb->n, &hb->n);
	break;
      case FLOAT:
	computeIntervalBounds<float>(lb1.f, hb1.f, lb2.f, hb2.f,
				     &lb->f, &hb->f);
	break;
      case DOUBLE:
	computeIntervalBounds<double>(lb1.d, hb1.d, lb2.d, hb2.d,
				      &lb->d, &hb->d);
	break;
      }

      *type = t1;
      return true;
    }

  case MIN:
    {
      const IndexExprMin *minExpr = static_cast<const IndexExprMin *>(expr);
      unsigned numOperands = minExpr->getNumOperands();
      value lbs[numOperands];
      value hbs[numOperands];
      value_type ops_type[numOperands];

      for (unsigned i=0; i<numOperands; i++) {
  	if (!computeBoundsRec(minExpr->getExprN(i), &lbs[i], &hbs[i],
			      &ops_type[i]))
  	  return false;
      }

      for (unsigned i=1; i<numOperands; i++)
	assert(ops_type[i] == ops_type[0]);

      switch (ops_type[0]) {
      case LONG:
	{
	  long lbs_long[numOperands], hbs_long[numOperands];
	  for (unsigned i=0; i<numOperands; i++) {
	    lbs_long[i] = lbs[i].n;
	    hbs_long[i] = hbs[i].n;
	  }
	  computeMinBounds<long>(lbs_long, hbs_long, numOperands,
				 &lb->n, &hb->n);
	break;
	}
      case FLOAT:
	{
	  float lbs_float[numOperands], hbs_float[numOperands];
	  for (unsigned i=0; i<numOperands; i++) {
	    lbs_float[i] = lbs[i].f;
	    hbs_float[i] = hbs[i].f;
	  }
	  computeMinBounds<float>(lbs_float, hbs_float, numOperands,
				  &lb->f, &hb->f);
	break;
	}
      case DOUBLE:
	{
	  double lbs_double[numOperands], hbs_double[numOperands];
	  for (unsigned i=0; i<numOperands; i++) {
	    lbs_double[i] = lbs[i].d;
	    hbs_double[i] = hbs[i].d;
	  }
	  computeMinBounds<double>(lbs_double, hbs_double, numOperands,
				   &lb->d, &hb->d);
	break;
	}
      };

      *type = ops_type[0];
      return true;
    }

  case MAX:
    {
      const IndexExprMax *maxExpr = static_cast<const IndexExprMax *>(expr);
      unsigned numOperands = maxExpr->getNumOperands();
      value lbs[numOperands];
      value hbs[numOperands];
      value_type ops_type[numOperands];

      for (unsigned i=0; i<numOperands; i++) {
  	if (!computeBoundsRec(maxExpr->getExprN(i), &lbs[i], &hbs[i],
			      &ops_type[i]))
  	  return false;
      }

      for (unsigned i=1; i<numOperands; i++)
	assert(ops_type[i] == ops_type[0]);

      switch (ops_type[0]) {
      case LONG:
	{
	  long lbs_long[numOperands], hbs_long[numOperands];
	  for (unsigned i=0; i<numOperands; i++) {
	    lbs_long[i] = lbs[i].n;
	    hbs_long[i] = hbs[i].n;
	  }
	  computeMaxBounds<long>(lbs_long, hbs_long, numOperands,
				 &lb->n, &hb->n);
	break;
	}
      case FLOAT:
	{
	  float lbs_float[numOperands], hbs_float[numOperands];
	  for (unsigned i=0; i<numOperands; i++) {
	    lbs_float[i] = lbs[i].f;
	    hbs_float[i] = hbs[i].f;
	  }
	  computeMaxBounds<float>(lbs_float, hbs_float, numOperands,
				  &lb->f, &hb->f);
	break;
	}
      case DOUBLE:
	{
	  double lbs_double[numOperands], hbs_double[numOperands];
	  for (unsigned i=0; i<numOperands; i++) {
	    lbs_double[i] = lbs[i].d;
	    hbs_double[i] = hbs[i].d;
	  }
	  computeMaxBounds<double>(lbs_double, hbs_double, numOperands,
				   &lb->d, &hb->d);
	break;
	}
      };

      *type = ops_type[0];
      return true;
    }

  case LB:
    {
      const IndexExprLB *lbExpr = static_cast<const IndexExprLB *>(expr);
      value lbL, hbL;
      value_type t;

      if (!computeBoundsRec(lbExpr->getExpr(), &lbL, &hbL, &t))
  	  return false;

      switch (t) {
      case LONG:
	computeLbBounds<long>(lbL.n, hbL.n, &lb->n, &hb->n);
	break;
      case FLOAT:
	computeLbBounds<float>(lbL.f, hbL.f, &lb->f, &hb->f);
	break;
      case DOUBLE:
	computeLbBounds<double>(lbL.d, hbL.d, &lb->d, &hb->d);
	break;
      };

      *type = t;
      return true;
    }

  case HB:
    {
      const IndexExprHB *hbExpr = static_cast<const IndexExprHB *>(expr);
      value lbH, hbH;
      value_type t;

      if (!computeBoundsRec(hbExpr->getExpr(), &lbH, &hbH, &t))
  	  return false;

      switch (t) {
      case LONG:
	computeHbBounds<long>(lbH.n, hbH.n, &lb->n, &hb->n);
	break;
      case FLOAT:
	computeHbBounds<float>(lbH.f, hbH.f, &lb->f, &hb->f);
	break;
      case DOUBLE:
	computeHbBounds<double>(lbH.d, hbH.d, &lb->d, &hb->d);
	break;
      };

      *type = t;
      return true;
    }

  case CAST:
    {
      const IndexExprCast *castExpr = static_cast<const IndexExprCast *>(expr);
      value_type t;

      if (!computeBoundsRec(castExpr->getExpr(), lb, hb, &t))
	  return false;

      switch (castExpr->cast) {
      case IndexExprCast::F2D:
	assert(t == FLOAT);
	lb->d = (double) lb->f;
	hb->d = (double) hb->f;
	*type = DOUBLE;
	break;
      case IndexExprCast::D2F:
	assert(t == DOUBLE);
	lb->f = (float) lb->d;
	hb->f = (float) hb->d;
	*type = FLOAT;
	break;
      case IndexExprCast::I2F:
	assert(t == LONG);
	lb->f = (float) lb->n;
	hb->f = (float) hb->n;
	*type = FLOAT;
	break;
      case IndexExprCast::F2I:
	assert(t == FLOAT);
	lb->n = (long) lb->f;
	hb->n = (long) hb->f;
	*type = LONG;
	break;
      case IndexExprCast::I2D:
	assert(t == LONG);
	lb->d = (double) lb->n;
	hb->d = (double) hb->n;
	*type = DOUBLE;
	break;
      case IndexExprCast::D2I:
	assert(t == DOUBLE);
	lb->n = (long) lb->d;
	hb->n = (long) hb->d;
	*type = LONG;
	break;
      case IndexExprCast::FLOOR:
	switch(t) {
	case LONG:
	  assert(false);
	case FLOAT:
	  lb->n = floorf(lb->f);
	  hb->n = floorf(hb->f);
	  break;
	case DOUBLE:
	  lb->n = floor(lb->d);
	  hb->n = floor(hb->d);
	  break;
	};
	*type = LONG;
	break;
      };
    }

  default:
    return false;
  };
}

