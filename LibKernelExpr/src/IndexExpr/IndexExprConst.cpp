#include "IndexExpr/IndexExprConst.h"

#include <iostream>

IndexExprConst::IndexExprConst(long i)
  : IndexExprValue(IndexExpr::CONST) {
  intValue = i;
}

IndexExprConst::~IndexExprConst() {}

void
IndexExprConst::dump() const {
  std::cerr << intValue;
}

IndexExpr *
IndexExprConst::clone() const {
  return new IndexExprConst(intValue);
}

bool
IndexExprConst::getValue(long *value) const {
  *value = intValue;
  return true;
}

void
IndexExprConst::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n" << intValue << " \"];\n";
}

void
IndexExprConst::write(std::stringstream &s) const {
  IndexExpr::write(s);
  s.write(reinterpret_cast<const char*>(&intValue), sizeof(intValue));
}
