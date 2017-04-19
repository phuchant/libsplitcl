#ifndef LISTINTERVAL_H
#define LISTINTERVAL_H

#include "Interval.h"

#include <vector>
#include <string>

class ListInterval {
 public:
  ListInterval();
  ~ListInterval();

  void add(const Interval &inter);
  void remove(const Interval &i);
  void clear();
  void clearList();
  void setUndefined();

  ListInterval *clone() const;

  void myUnion(const ListInterval &l);
  void difference(const ListInterval &l);

  static ListInterval *intersection(const ListInterval &i1,
				    const ListInterval &i2);

  static ListInterval *difference(const ListInterval &l1,
				  const ListInterval &l2);

  void debug() const;
  std::string toString() const;
  bool isUndefined() const;

  size_t total() const;

  // protected:
  std::vector<Interval> mList;
  bool undefined;
};

#endif /* LISTINTERVAL_H */
