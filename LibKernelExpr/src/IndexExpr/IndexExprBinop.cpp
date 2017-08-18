#include "IndexExpr/IndexExprArg.h"
#include "IndexExpr/IndexExprBinop.h"
#include "IndexExpr/IndexExprConst.h"
#include "IndexExpr/IndexExprInterval.h"
#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprBinop::IndexExprBinop(unsigned op, IndexExpr *expr1, IndexExpr *expr2)
  : IndexExpr(IndexExpr::BINOP), op(op), expr1(expr1), expr2(expr2) {}

IndexExprBinop::~IndexExprBinop() {
  if (expr1)
    delete expr1;
  if (expr2)
    delete expr2;
}

void
IndexExprBinop::dump() const {
  if (expr1) {
    if (expr1->getTag() == IndexExpr::BINOP)
      std::cerr << "(";
    expr1->dump();
    if (expr1->getTag() == IndexExpr::BINOP)
      std::cerr << ")";
  }
  else {
    std::cerr << "NULL";
  }

  switch(op) {
  case IndexExprBinop::Add:
    std::cerr << " + ";
    break;
  case IndexExprBinop::Sub:
    std::cerr << " - ";
    break;
  case IndexExprBinop::Mul:
    std::cerr << " * ";
    break;
  case IndexExprBinop::Div:
    std::cerr << " / ";
    break;
  case IndexExprBinop::Mod:
    std::cerr << " % ";
    break;
  case IndexExprBinop::Or:
    std::cerr << " | ";
    break;
  case IndexExprBinop::Xor:
    std::cerr << " ^ ";
    break;
  case IndexExprBinop::And:
    std::cerr << " & ";
    break;
  case IndexExprBinop::Shl:
    std::cerr << " << ";
    break;
  case IndexExprBinop::Shr:
    std::cerr << " >> ";
    break;
  default:
    std::cerr << " ? ";
    break;
  };

  if (expr2) {
    if (expr2->getTag() == IndexExpr::BINOP)
      std::cerr << "(";
    expr2->dump();
    if (expr2->getTag() == IndexExpr::BINOP)
      std::cerr << ")";
  } else {
    std::cerr << "NULL";
  }
}

IndexExpr *
IndexExprBinop::clone() const {
  IndexExpr *expr1Clone = NULL;
  if (expr1)
    expr1Clone = expr1->clone();

  IndexExpr *expr2Clone = NULL;
  if (expr2)
    expr2Clone = expr2->clone();

  return new IndexExprBinop(op, expr1Clone, expr2Clone);
}

IndexExpr *
IndexExprBinop::getWorkgroupExpr(const NDRange &ndRange) const {
  IndexExpr *wg_expr1;
  IndexExpr *wg_expr2;

  if (expr1)
    wg_expr1 = expr1->getWorkgroupExpr(ndRange);
  else
    wg_expr1 = new IndexExprUnknown("null");

  if (expr2)
    wg_expr2 = expr2->getWorkgroupExpr(ndRange);
  else
    wg_expr2 = new IndexExprUnknown("null");

  return new IndexExprBinop(op, wg_expr1, wg_expr2);
}

IndexExpr *
IndexExprBinop::getKernelExpr(const NDRange &ndRange) const {
  IndexExpr *kl_expr1;
  IndexExpr *kl_expr2;

  if (expr1)
    kl_expr1 = expr1->getKernelExpr(ndRange);
  else
    kl_expr1 = new IndexExprUnknown("null");

  if (expr2)
    kl_expr2 = expr2->getKernelExpr(ndRange);
  else
    kl_expr2 = new IndexExprUnknown("null");

  return new IndexExprBinop(op, kl_expr1, kl_expr2);
}

IndexExpr *
IndexExprBinop::getKernelExprWithGuards(const NDRange &ndRange,
					const std::vector<GuardExpr *> &guards)
  const {
  IndexExpr *kl_expr1;
  IndexExpr *kl_expr2;

  if (expr1)
    kl_expr1 = expr1->getKernelExprWithGuards(ndRange, guards);
  else
    kl_expr1 = new IndexExprUnknown("null");

  if (expr2)
    kl_expr2 = expr2->getKernelExprWithGuards(ndRange, guards);
  else
    kl_expr2 = new IndexExprUnknown("null");

  return new IndexExprBinop(op, kl_expr1, kl_expr2);
}

// IndexExpr *
// IndexExprBinop::computeExpr() const {
//   IndexExpr *computedExpr1 = NULL;
//   if (expr1)
//     computedExpr1 = expr1->computeExpr();
//   IndexExpr *computedExpr2 = NULL;
//   if (expr2)
//     computedExpr2 = expr2->computeExpr();

//   if (computedExpr1 == NULL ||
//       computedExpr2 == NULL ||
//       (op != IndexExprBinop::Add &&
//        op != IndexExprBinop::Sub &&
//        op != IndexExprBinop::Mul &&
//        op != IndexExprBinop::Div &&
//        op != IndexExprBinop::Or))
//     return new IndexExprBinop(op, computedExpr1, computedExpr2);

//   /* reductions possible :
//      interval - constant ou arg avec valueSet
//      interval - interval
//      constant - constant
//      constant - interval */

//   if (computedExpr1->getTag() == IndexExpr::INTERVAL) {
//     IndexExprInterval *interval1 = static_cast<IndexExprInterval *>
//       (computedExpr1);

//     IndexExpr *interval1Lb = interval1->getLowerBound();
//     IndexExpr *interval1Hb = interval1->getHigherBound();
//     if (!interval1Lb ||
// 	interval1Lb->getTag() != IndexExpr::CONST ||
// 	!interval1Hb ||
// 	interval1Hb->getTag() != IndexExpr::CONST) {
//       if (interval1Lb)
// 	delete interval1Lb;
//       if (interval1Hb)
// 	delete interval1Hb;

//       return new IndexExprBinop(op, computedExpr1, computedExpr2);
//     }

//     IndexExprConst *constLowerBound1 = static_cast<IndexExprConst *>
//       (interval1Lb);
//     IndexExprConst *constHigherBound1 = static_cast<IndexExprConst *>
//       (interval1Hb);

//     int lowerBound1 = constLowerBound1->getValue();
//     int higherBound1 = constHigherBound1->getValue();

//     /* Interval - Constant */

//     if (computedExpr2->getTag() == IndexExpr::CONST) {
//       IndexExprConst *constExpr2 = static_cast<IndexExprConst *>(computedExpr2);
//       int valueExpr2 = constExpr2->getValue();

//       int newLowerBound = 0;
//       int newHigherBound = 0;

//       switch (op) {
//       case IndexExprBinop::Add:
// 	newLowerBound = lowerBound1 + valueExpr2;
// 	newHigherBound = higherBound1 + valueExpr2;
// 	break;
//       case IndexExprBinop::Sub:
// 	newLowerBound = lowerBound1 - valueExpr2;
// 	newHigherBound = higherBound1 - valueExpr2;
// 	break;
//       case IndexExprBinop::Mul:
// 	newLowerBound = lowerBound1 * valueExpr2;
// 	newHigherBound = higherBound1 * valueExpr2;
// 	break;
//       case IndexExprBinop::Div:
// 	newLowerBound = lowerBound1 / valueExpr2;
// 	newHigherBound = higherBound1 / valueExpr2;
// 	break;
//       case IndexExprBinop::Or:
// 	newLowerBound = lowerBound1 | valueExpr2;
// 	newHigherBound = higherBound1 | valueExpr2;
// 	break;
//       }

//       delete computedExpr1;
//       delete computedExpr2;
//       delete interval1Lb;
//       delete interval1Hb;

//       return new IndexExprInterval(new IndexExprConst(newLowerBound),
// 				   new IndexExprConst(newHigherBound));
//     }

//     /* Interval - Interval */
//     if (computedExpr2->getTag() == IndexExpr::INTERVAL) {
//       IndexExprInterval *interval2 = static_cast<IndexExprInterval *>
// 	(computedExpr2);

//       IndexExpr *interval2Lb = interval2->getLowerBound();
//       IndexExpr *interval2Hb = interval2->getHigherBound();

//       if (!interval2Lb ||
// 	  interval2Lb->getTag() != IndexExpr::CONST ||
// 	  !interval2Hb ||
// 	  interval2Hb->getTag() != IndexExpr::CONST) {
// 	if (interval2Lb)
// 	  delete interval2Lb;
// 	if (interval2Hb)
// 	  delete interval2Hb;
// 	delete interval1Lb;
// 	delete interval1Hb;

// 	return new IndexExprBinop(op, computedExpr1, computedExpr2);
//       }

//       IndexExprConst *constLowerBound2 = static_cast<IndexExprConst *>
// 	(interval2Lb);
//       IndexExprConst *constHigherBound2 = static_cast<IndexExprConst *>
// 	(interval2Hb);

//       int lowerBound2 = constLowerBound2->getValue();
//       int higherBound2 = constHigherBound2->getValue();

//       int newLowerBound = 0;
//       int newHigherBound = 0;

//       switch (op) {
//       case IndexExprBinop::Add:
// 	newLowerBound = lowerBound1 + lowerBound2;
// 	newHigherBound = higherBound1 + higherBound2;
// 	break;
//       case IndexExprBinop::Sub:
// 	newLowerBound = lowerBound1 - lowerBound2;
// 	newHigherBound = higherBound1 - higherBound2;
// 	break;
//       case IndexExprBinop::Mul:
// 	newLowerBound = lowerBound1 * lowerBound2;
// 	newHigherBound = higherBound1 * higherBound2;
// 	break;
//       case IndexExprBinop::Div:
// 	newLowerBound = lowerBound1 / lowerBound2;
// 	newHigherBound = higherBound1 / higherBound2;
// 	break;
//       case IndexExprBinop::Or:
// 	newLowerBound = lowerBound1 | lowerBound2;
// 	newHigherBound = higherBound1 | higherBound2;
// 	break;
//       }

//       delete computedExpr1;
//       delete computedExpr2;
//       delete interval1Lb;
//       delete interval1Hb;
//       delete interval2Lb;
//       delete interval2Hb;

//       return new IndexExprInterval(new IndexExprConst(newLowerBound),
// 				   new IndexExprConst(newHigherBound));
//     }
//   }

//   if (computedExpr1->getTag() == IndexExpr::CONST) {
//     IndexExprConst *constExpr1 = static_cast<IndexExprConst *>(computedExpr1);
//     int valueExpr1 = constExpr1->getValue();

//     /* Constant - Constant */
//     if (computedExpr2->getTag() == IndexExprConst::CONST) {
//       IndexExprConst *constExpr2 = static_cast<IndexExprConst *>(computedExpr2);
//       int valueExpr2 = constExpr2->getValue();

//       int newValue = 0;

//       switch (op) {
//       case IndexExprBinop::Add:
// 	newValue = valueExpr1 + valueExpr2;
// 	break;
//       case IndexExprBinop::Sub:
// 	newValue = valueExpr1 - valueExpr2;
// 	break;
//       case IndexExprBinop::Mul:
// 	newValue = valueExpr1 * valueExpr2;
// 	break;
//       case IndexExprBinop::Div:
// 	newValue = valueExpr1 / valueExpr2;
// 	break;
//       case IndexExprBinop::Or:
// 	newValue = valueExpr1 | valueExpr2;
// 	break;
//       }

//       delete computedExpr1;
//       delete computedExpr2;

//       return new IndexExprConst(newValue);
//     }

//     /* Constant - Interval */
//     if (computedExpr2->getTag() == IndexExpr::INTERVAL) {
//       IndexExprInterval *interval2 = static_cast<IndexExprInterval *>
// 	(computedExpr2);

//       IndexExpr *interval2Lb = interval2->getLowerBound();
//       IndexExpr *interval2Hb = interval2->getHigherBound();
//       if (!interval2Lb ||
// 	  interval2Lb->getTag() != IndexExpr::CONST ||
// 	  !interval2Hb ||
// 	  interval2Hb->getTag() != IndexExpr::CONST) {
// 	if (interval2Lb)
// 	  delete interval2Lb;
// 	if (interval2Hb)
// 	  delete interval2Hb;

// 	return new IndexExprBinop(op, computedExpr1, computedExpr2);
//       }

//       IndexExprConst *constLowerBound2 = static_cast<IndexExprConst *>
// 	(interval2Lb);
//       IndexExprConst *constHigherBound2 = static_cast<IndexExprConst *>
// 	(interval2Hb);

//       int lowerBound2 = constLowerBound2->getValue();
//       int higherBound2 = constHigherBound2->getValue();

//       int newLowerBound = 0;
//       int newHigherBound = 0;

//       switch (op) {
//       case IndexExprBinop::Add:
// 	newLowerBound = valueExpr1 + lowerBound2;
// 	newHigherBound = valueExpr1 + higherBound2;
// 	break;
//       case IndexExprBinop::Sub:
// 	newLowerBound = valueExpr1 - lowerBound2;
// 	newHigherBound = valueExpr1 - higherBound2;
// 	break;
//       case IndexExprBinop::Mul:
// 	newLowerBound = valueExpr1 * lowerBound2;
// 	newHigherBound = valueExpr1 * higherBound2;
// 	break;
//       case IndexExprBinop::Div:
// 	newLowerBound = valueExpr1 / lowerBound2;
// 	newHigherBound = valueExpr1 / higherBound2;
// 	break;
//       case IndexExprBinop::Or:
// 	newLowerBound = valueExpr1 | lowerBound2;
// 	newHigherBound = valueExpr1 | higherBound2;
// 	break;
//       }

//       delete computedExpr1;
//       delete computedExpr2;
//       delete interval2Lb;
//       delete interval2Hb;

//       return new IndexExprInterval(new IndexExprConst(newLowerBound),
// 				   new IndexExprConst(newHigherBound));
//     }
//   }

//   return new IndexExprBinop(op, computedExpr1, computedExpr2);
// }

// IndexExpr *
// IndexExprBinop::removeNeutralElem(bool *changed) const {
//   // Addition
//   if (op == IndexExprBinop::Add) {
//     if (expr1 &&
// 	expr1->getTag() == IndexExpr::CONST &&
// 	static_cast<IndexExprConst *>(expr1)->getValue() == 0) {
//       *changed = true;
//       return expr2->clone();
//     }

//     if (expr2 &&
// 	expr2->getTag() == IndexExpr::CONST &&
// 	static_cast<IndexExprConst *>(expr2)->getValue() == 0) {
//       *changed = true;
//       return expr1->clone();
//     }
//   }

//   // Substraction
//   if (op == IndexExprBinop::Sub) {
//     if (expr2 &&
// 	expr2->getTag() == IndexExpr::CONST &&
// 	static_cast<IndexExprConst *>(expr2)->getValue() == 0) {
//       *changed = true;
//       return expr1->clone();
//     }
//   }

//   // Multiplication
//   if (op == IndexExprBinop::Mul) {
//     if (expr1 &&
// 	expr1->getTag() == IndexExpr::CONST &&
// 	static_cast<IndexExprConst *>(expr1)->getValue() == 1) {
//       *changed = true;
//       return expr2->clone();
//     }

//     if (expr2 &&
// 	expr2->getTag() == IndexExpr::CONST &&
// 	static_cast<IndexExprConst *>(expr2)->getValue() == 1) {
//       *changed = true;
//       return expr1->clone();
//     }

//     if ((expr1 &&
// 	 expr1->getTag() == IndexExpr::CONST &&
// 	 static_cast<IndexExprConst *>(expr1)->getValue() == 0) ||
// 	(expr2 &&
// 	 expr2->getTag() == IndexExpr::CONST &&
// 	 static_cast<IndexExprConst *>(expr2)->getValue() == 0)) {
//       *changed = true;
//       return new IndexExprConst(0);
//     }
//   }

//   // Division
//   if (op == IndexExprBinop::Div) {
//     if (expr2 &&
// 	expr2->getTag() == IndexExpr::CONST &&
// 	static_cast<IndexExprConst *>(expr2)->getValue() == 1) {
//       *changed = true;
//       return expr1->clone();
//     }
//   }

//   *changed = false;
//   return clone();
// }

// IndexExpr *
// IndexExprBinop::getLowerBound() const {
//   IndexExpr *expr1LowerBound = NULL;
//   if (expr1)
//     expr1LowerBound = expr1->getLowerBound();

//   IndexExpr *expr2LowerBound = NULL;
//   if (expr2) {
//     if (op == IndexExprBinop::Sub ||
// 	op == IndexExprBinop::Div) {
//       expr2LowerBound = expr2->getHigherBound();
//     } else {
//       expr2LowerBound = expr2->getLowerBound();
//     }
//   }

//   return new IndexExprBinop(op, expr1LowerBound, expr2LowerBound);
// }

// IndexExpr *
// IndexExprBinop::getHigherBound() const {
//   IndexExpr *expr1HigherBound = NULL;
//   if (expr1)
//     expr1HigherBound = expr1->getHigherBound();

//   IndexExpr *expr2HigherBound = NULL;
//   if (expr2) {
//     if (op == IndexExprBinop::Sub ||
// 	op == IndexExprBinop::Div) {
//       expr2HigherBound = expr2->getLowerBound();
//     } else {
//       expr2HigherBound = expr2->getHigherBound();
//     }
//   }

//   return new IndexExprBinop(op, expr1HigherBound, expr2HigherBound);
// }

void
IndexExprBinop::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  switch (op) {
  case IndexExprBinop::Add:
    stream << " + ";
    break;
  case IndexExprBinop::Sub:
    stream << " - ";
    break;
  case IndexExprBinop::Mul:
    stream << " * ";
    break;
  case IndexExprBinop::Div:
    stream << " / ";
    break;
  case IndexExprBinop::Mod:
    stream << " % ";
    break;
  case IndexExprBinop::Unknown:
    stream << " ? ";
    break;
  case IndexExprBinop::Or:
    stream << " or ";
    break;
  case IndexExprBinop::Xor:
    stream << " ^ ";
    break;
  case IndexExprBinop::And:
    stream << " & ";
    break;
  case IndexExprBinop::Shl:
    stream << " << ";
    break;
  case IndexExprBinop::Shr:
    stream << " >> ";
    break;
  }

  stream << " \"];\n";

  if (expr1) {
    expr1->toDot(stream);
    stream << id << " -> " << expr1->getID() << " [label=\"expr1\"];\n";
  }

  if (expr2) {
    expr2->toDot(stream);
    stream << id << " -> " << expr2->getID() << " [label=\"expr2\"];\n";
  }
}

int
IndexExprBinop::getOp() const {
  return op;
}

const IndexExpr *
IndexExprBinop::getExpr1() const {
  return expr1;
}

const IndexExpr *
IndexExprBinop::getExpr2() const {
  return expr2;
}

IndexExpr *
IndexExprBinop::getExpr1() {
  return expr1;
}

IndexExpr *
IndexExprBinop::getExpr2() {
  return expr2;
}

void
IndexExprBinop::setExpr1(IndexExpr *expr) {
  expr1 = expr;
}

void
IndexExprBinop::setExpr2(IndexExpr *expr) {
  expr2 = expr;
}

void
IndexExprBinop::write(std::stringstream &s) const {
  IndexExpr::write(s);
  s.write(reinterpret_cast<const char *>(&op), sizeof(op));
  if(expr1)
    expr1->write(s);
  else
    writeNIL(s);
  if (expr2)
    expr2->write(s);
  else
    writeNIL(s);
}
