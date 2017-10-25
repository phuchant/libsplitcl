#include "IndexExpr/IndexExprArg.h"

#include <cassert>
#include <iostream>

IndexExprArg::IndexExprArg(const std::string &name, unsigned pos)
  : IndexExpr(IndexExpr::ARG), name(name), pos(pos), mIsValueSet(false),
    value(nullptr) {}

IndexExprArg::~IndexExprArg() {
  delete value;
}

IndexExpr *
IndexExprArg::getKernelExpr(const NDRange &ndRange,
			    const std::vector<GuardExpr *> & guards,
			    const std::vector<IndirectionValue> &
			    indirValues) const {
  (void) ndRange;
  (void) guards;
  (void) indirValues;

  return clone();
}


void
IndexExprArg::dump() const {
  std::cerr << "(arg no[ " << pos << "] ";
  if (mIsValueSet) {
    std::cerr << "val = ";
    value->dump();
    std::cerr << "\n";
  }
  std::cerr << ")";
}

IndexExpr *
IndexExprArg::clone() const {
  IndexExprArg *ret = new IndexExprArg(name, pos);
  if (value)
    ret->setValue(value);
  return ret;
}

void
IndexExprArg::toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  stream << "\\n" << name << " \"];\n";
}

std::string
IndexExprArg::getName() const {
  return name;
}

void
IndexExprArg::write(std::stringstream &s) const {
  IndexExpr::write(s);
  s.write(reinterpret_cast<const char *>(&pos), sizeof(pos));
}

unsigned
IndexExprArg::getPos() const {
  return pos;
}

void
IndexExprArg::setValue(const IndexExprValue *v) {
  delete value;
  value = static_cast<IndexExprValue *>(v->clone());
  mIsValueSet = true;
}

const IndexExprValue *
IndexExprArg::getValue() const {
  return value;
}
