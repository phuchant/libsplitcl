#include "IndexExpr/IndexExprUnknown.h"

#include <iostream>

IndexExprUnknown::IndexExprUnknown(const std::string &str)
  : IndexExpr(IndexExpr::UNKNOWN), str(str) {}

IndexExprUnknown::~IndexExprUnknown() {}

void
IndexExprUnknown::dump() const {
  std::cerr << str;
}

IndexExpr *
IndexExprUnknown::clone() const {
  return new IndexExprUnknown(str);
}

void
IndexExprUnknown::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n";

  stream << str;

  stream << " \"];\n";
}
