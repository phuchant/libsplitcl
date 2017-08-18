#include "IndexExpr/IndexExpr.h"

#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprBinop.h"
#include "IndexExpr/IndexExprConst.h"
#include "IndexExpr/IndexExprLB.h"
#include "IndexExpr/IndexExprHB.h"
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

// IndexExpr *
// IndexExpr::getIntervalExpr() const {
//   return new IndexExprInterval(getLowerBound(), getHigherBound());
// }

// IndexExpr *
// IndexExpr::getReduceExpr() const {
//   IndexExpr *ret = computeExpr();
//   bool changed;
//   do {
//     IndexExpr *tmp = ret;
//     ret = tmp->removeNeutralElem(&changed);
//     delete tmp;
//   } while (changed);

//   return ret;
// }

IndexExpr *
IndexExpr::getWorkgroupExpr(const NDRange &ndRange) const {
  (void) ndRange;

  return clone();
}

IndexExpr *
IndexExpr::getKernelExpr(const NDRange &ndRange) const {
  (void) ndRange;

  return clone();
}

IndexExpr *
IndexExpr::getKernelExprWithGuards(const NDRange &ndRange,
				   const std::vector<GuardExpr *> &guards) const
{
  (void) ndRange;
  (void) guards;
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

// int
// IndexExpr::compareTo(const IndexExpr *e) const {
//   /* -1 <
//    *  0 =
//    *  1 >
//    * -2 ?
//    */

//   if (!e)
//     return -2;

//   if (tag != e->getTag())
//     return -2;

//   if (tag == IndexExpr::CONST) {
//     const IndexExprConst *e1 = static_cast<const IndexExprConst *>(this);
//     const IndexExprConst *e2 = static_cast<const IndexExprConst *>(e);

//     if (e1->getValue() == e2->getValue())
//       return 0;

//     if (e1->getValue() < e2->getValue())
//       return -1;

//     return 1;
//   }

//   if (tag == IndexExpr::ARG) {
//     const IndexExprArg *e1 = static_cast<const IndexExprArg *>(this);
//     const IndexExprArg *e2 = static_cast<const IndexExprArg *>(e);

//     if (e1->getName().compare(e2->getName()) == 0)
//       return 0;

//     return -2;
//   }

//   if (tag == IndexExpr::OCL) {
//     const IndexExprOCL *e1 = static_cast<const IndexExprOCL *>(this);
//     const IndexExprOCL *e2 = static_cast<const IndexExprOCL *>(e);

//     if (e1->getOCLFunc() != e2->getOCLFunc())
//       return -2;

//     if (e1->getArg() && e2->getArg() &&
// 	e1->getArg()->getTag() == IndexExpr::CONST &&
// 	e2->getArg()->getTag() == IndexExpr::CONST) {
//       int v1 = static_cast<const IndexExprConst *>(e1->getArg())->getValue();
//       int v2 = static_cast<const IndexExprConst *>(e2->getArg())->getValue();

//       if (v1 == v2)
// 	return 0;
//     }

//     return -2;
//   }

//   if (tag == IndexExpr::BINOP) {
//     const IndexExprBinop *e1 = static_cast<const IndexExprBinop *>(this);
//     const IndexExprBinop *e2 = static_cast<const IndexExprBinop *>(e);

//     if (e1->getOp() != e2->getOp())
//       return -2;

//     if (e1->getExpr1() == NULL ||
// 	e1->getExpr2() == NULL ||
// 	e2->getExpr1() == NULL ||
// 	e2->getExpr2() == NULL)
//       return -2;

//     int ret1 = e1->getExpr1()->compareTo(e2->getExpr1());
//     int ret2 = e1->getExpr2()->compareTo(e2->getExpr2());

//     if (ret1 == ret2)
//       return ret1;

//     if (ret1 == 0)
//       return ret2;

//     if (ret2 == 0)
//       return ret1;

//     return -2;
//   }

//   if (tag == IndexExpr::INTERVAL) {
//     const IndexExprInterval *e1 = static_cast<const IndexExprInterval *>(this);
//     const IndexExprInterval *e2 = static_cast<const IndexExprInterval *>(e);

//     if (e1->getLowerBound() == NULL ||
// 	e1->getHigherBound() == NULL ||
// 	e2->getLowerBound() == NULL ||
// 	e2->getHigherBound() == NULL)
//       return -2;

//     return e1->getHigherBound()->compareTo(e2->getLowerBound());
//   }

//   return -2;
// }

// int
// IndexExpr::areDisjoint(const IndexExpr &e1, const IndexExpr &e2) {
//   /*
//    *  1 disjoint
//    *  0 not disjoint
//    * -1 ?
//    */

//   IndexExpr *e1LowerBound = NULL;
//   IndexExpr *e1HigherBound = NULL;
//   IndexExpr *e2LowerBound = NULL;
//   IndexExpr *e2HigherBound = NULL;

//   int ret = -1;
//   int cmp;

//   e1LowerBound = e1.getLowerBound();
//   e1HigherBound = e1.getHigherBound();
//   e2LowerBound = e2.getLowerBound();
//   e2HigherBound = e2.getHigherBound();

//   if (!e1LowerBound ||
//       !e1HigherBound ||
//       !e2LowerBound ||
//       !e2HigherBound)
//     goto delete_and_ret;

//   // Expressions are disjoint if either e1LowerBound > e2HigherBound
//   //                                 or e2LowerBound > e1HigherBound

//   cmp = e1LowerBound->compareTo(e2HigherBound);
//   if (cmp == 1) { // e1LowerBound > e2HigherBound
//     ret = 1;
//     goto delete_and_ret;
//   }

//   cmp = e2LowerBound->compareTo(e1HigherBound);
//   switch (cmp) {
//   case 1: // e2LowerBound > e1HigherBound
//     ret = 1;
//     break;
//   case 0: // e2LowerBound = e1HigherBound
//   case -1:
//     ret = 0;
//     break;
//   default: // comparison impossible
//     ret = -1;
//   };

//  delete_and_ret:
//   if (e1LowerBound)
//     delete e1LowerBound;
//   if (e1HigherBound)
//     delete e1HigherBound;
//   if (e2LowerBound)
//     delete e2LowerBound;
//   if (e2HigherBound)
//     delete e2HigherBound;

//   return ret;
// }

// int
// IndexExpr::areDisjoint(const std::vector<IndexExpr *> &exprs) {
//   /*
//    *  1 disjoint
//    *  0 not disjoint
//    * -1 ?
//    */

//   for (unsigned i=0; i<exprs.size()-1; ++i) {
//     for (unsigned j=i+1; j<exprs.size(); ++j) {
//       if (!exprs[i] || !exprs[j])
// 	return -1;

//       int cmp = areDisjoint(*exprs[i], *exprs[j]);
//       switch(cmp) {
//       case 1:
// 	continue;
//       case 0:
// 	return 0;
//       default:
// 	return -1;
//       };
//     }
//   }

//   return 1;
// }

// IndexExpr *
// IndexExpr::mergeExprs(const std::vector<IndexExpr *> &exprs) {
//   unsigned size = exprs.size();
//   if (size == 0)
//     return NULL;

//   if (size == 1)
//     return exprs[0]->clone();

//   IndexExpr *lb = exprs[0]->getLowerBound();
//   IndexExpr *hb = exprs[0]->getHigherBound();

//   for (unsigned i=1; i<size; i++) {
//     IndexExpr *currentLowerBound = exprs[i]->getLowerBound();
//     IndexExpr *currentHigherBound = exprs[i]->getHigherBound();
//     int cmpLb = lb->compareTo(currentLowerBound);
//     int cmpHb = hb->compareTo(currentHigherBound);

//     if (cmpLb == 1) { // currentLowerBound < lb
//       delete lb;
//       delete currentHigherBound;
//       lb = currentLowerBound;
//       continue;
//     } else if (cmpLb == -2) { // comparison impossible
//       delete lb;
//       delete hb;
//       delete currentLowerBound;
//       delete currentHigherBound;
//       return NULL;
//     }

//     if (cmpHb == -1) { // currentHigherBound > hb
//       delete hb;
//       delete currentLowerBound;
//       hb = currentHigherBound;
//       continue;
//     } else if (cmpHb == -2) { // comparison impossible
//       delete lb;
//       delete hb;
//       delete currentLowerBound;
//       delete currentHigherBound;
//       return NULL;
//     }

//     delete currentLowerBound;
//     delete currentHigherBound;
//   }

//   return new IndexExprInterval(lb, hb);
// }

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

  default:

    return;
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

