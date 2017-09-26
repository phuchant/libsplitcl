#include "IndexExpr/IndexExprArg.h"

#include <iostream>

IndexExprArg::IndexExprArg(const std::string &name, unsigned pos)
  : IndexExprValue(IndexExpr::ARG), name(name), pos(pos), mIsValueSet(false),
    mValue(42) {}

IndexExprArg::~IndexExprArg() {}

void
IndexExprArg::dump() const {
  std::cerr << "(arg no " << pos;
  if (mIsValueSet)
    std::cerr << " value=" << mValue;
  std::cerr << ")";
}

IndexExpr *
IndexExprArg::clone() const {
  IndexExprArg *ret = new IndexExprArg(name, pos);
  ret->setValue(mValue);

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
IndexExprArg::setValue(long value) {
  mValue = value;
  mIsValueSet = true;
}

bool
IndexExprArg::getValue(long *value) const {
  if (!mIsValueSet)
    return false;

    *value = mValue;
  return true;
}
