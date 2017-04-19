#include "Interval.h"

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

#include <sstream>

#include <cassert>
#include <climits>
#include <iostream>

Interval::Interval(size_t l, size_t h)
  : lb(l), hb(h)
{
   assert(h >= l);
}

Interval::~Interval()
{}

bool
Interval::intersection(const Interval &i1, const Interval &i2, Interval &res) {
  res.lb = MAX(i1.lb, i2.lb);
  res.hb = MIN(i1.hb, i2.hb);

  if (res.lb > res.hb)
    return false;

  return true;
}

Interval
Interval::getInfinity() {
  return Interval(0, UINT_MAX);
}

void Interval::debug() const {
  std::cerr << "[" << lb << "," << hb << "]";
}

std::string Interval::toString() const {
  std::stringstream ss;
  ss << "[" << lb << "," << hb << "]";
  return ss.str();
}
