#include "IndexExpr/IndexExprValue.h"

#include <cassert>
#include <iostream>

IndexExprValue *
IndexExprValue::createLong(long v) {
  IndexExprValue *ret = new IndexExprValue(LONG);
  ret->value.n = v;
  return ret;
}

IndexExprValue *
IndexExprValue::createFloat(float v) {
  IndexExprValue *ret = new IndexExprValue(FLOAT);
  ret->value.f = v;
  return ret;
}

IndexExprValue *
IndexExprValue::createDouble(double v) {
  IndexExprValue *ret = new IndexExprValue(DOUBLE);
  ret->value.d = v;
  return ret;
}

IndexExprValue::IndexExprValue(value_type t)
  : IndexExpr(IndexExpr::VALUE), type(t) {}

IndexExprValue::~IndexExprValue() {}

IndexExpr *
IndexExprValue::getWorkgroupExpr(const NDRange &ndRange) const {
  (void) ndRange;
  return clone();
}

IndexExpr *
IndexExprValue::getKernelExpr(const NDRange &ndRange,
			      const std::vector<GuardExpr *> & guards,
			      const std::vector<IndirectionValue> &
			      indirValues) const {
  (void) ndRange;
  (void) guards;
  (void) indirValues;

  return clone();
}


void
IndexExprValue::write(std::stringstream &s) const {
  IndexExpr::write(s);
  s.write(reinterpret_cast<const char *>(&type), sizeof(type));
  switch (type) {
  case LONG:
    {
      long val = value.n;
      s.write(reinterpret_cast<const char *>(&val), sizeof(val));
      break;
    }
  case FLOAT:
    {
      float val = value.f;
      s.write(reinterpret_cast<const char *>(&val), sizeof(val));
      break;
    }
  case DOUBLE:
    {
      double val = value.d;
      s.write(reinterpret_cast<const char *>(&val), sizeof(val));
      break;
    }
  };

}

void
IndexExprValue::dump() const {
  switch(type) {
  case LONG:
    std::cerr << value.n;
    break;
  case FLOAT:
    std::cerr << value.f;
    break;
  case DOUBLE:
    std::cerr << value.d;
  };
}

IndexExpr *
IndexExprValue::clone() const {
  IndexExprValue *ret = new IndexExprValue(type);
  ret->value = value;
  return ret;
}

void
IndexExprValue:: toDot(std::stringstream &stream) const {
  IndexExpr::toDot(stream);

  switch(type) {
  case LONG:
    stream << "\\n" << value.n << " \"];\n";
    break;
  case FLOAT:
    stream << "\\n" << value.f << " \"];\n";
    break;
  case DOUBLE:
    stream << "\\n" << value.d << " \"];\n";
  };
}

long
IndexExprValue::getLongValue() const {
  assert(type == LONG);
  return value.n;
}

float
IndexExprValue::getFloatValue() const {
  assert(type == FLOAT);
  return value.f;
}

double
IndexExprValue::getDoubleValue() const {
  assert(type == DOUBLE);
  return value.d;
}


