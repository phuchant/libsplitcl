#ifndef INTERVAL_H
#define INTERVAL_H

#include <string>

#include <stdlib.h>

struct Interval {
 public:
  Interval(size_t l, size_t h);
  ~Interval();

  void debug() const;
  std::string toString() const;

  size_t lb;
  size_t hb;

  static bool intersection(const Interval &i1, const Interval &i2,
			   Interval &res);
  static Interval getInfinity();
 };

#endif /* INTERVAL_H */
