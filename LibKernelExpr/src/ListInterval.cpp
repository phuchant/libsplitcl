#include "ListInterval.h"

#include <stdlib.h>

#include <algorithm>
#include <iostream>
#include <stack>
#include <sstream>

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

ListInterval::ListInterval()
  : undefined(false)
{}

ListInterval::~ListInterval() {
}

void ListInterval::add(const Interval &inter) {
  if (mList.empty()) {
    mList.push_back(inter);
    return;
  }

  // Insert new interval in order
  unsigned index = 0;

  while (index < mList.size() && mList[index].lb < inter.lb)
    index++;
  mList.insert(mList.begin()+index, inter);

  std::vector<Interval> s;

  s.push_back(mList[0]);

  // Start from the next interval and merge it if necessary.
  for (unsigned i=1; i<mList.size(); i++) {
    Interval top = s[s.size()-1];

    // if current interval is not overlapping with stack top,
    // push_back it to the stack
    if (top.hb+1 < mList[i].lb) {
      s.push_back(mList[i]);
    }
    // otherwise update the ending time of top if ending of current interval is
    // more
    else if (top.hb < mList[i].hb) {
      top.hb = mList[i].hb;
      s.pop_back();
      s.push_back(top);
    }
  }

  // Finally replace mList with the stack
  mList = s;
}

void ListInterval::remove(const Interval &inter) {
  if (mList.empty() ||
      mList[0].lb > inter.hb ||
      mList[mList.size()-1].hb < inter.lb)
    return;

  // std::cerr << "Removing interval [" << inter.lb << ","
  // 	    << inter.hb << "]\n";
  // std::cerr << "list : ";
  // debug();
  // std::cerr << "\n";


  unsigned n = mList.size() * 2;
  int lIndex = 0;
  int hIndex = n;


  /*
   * blockid :        0                 1             2          3
   *
   * indexes :  0     1      2          3        4    5     6    7     8
   *
   *               [-----]       [-------------]   [------]   [-----]
   *
   * index before a block : blockid * 2
   * index inside a block : blockid * 2 + 1
   * index after  a block : blockid * 2 + 2
   */



  // Compute lower index
  for (unsigned i=0; i<= n; i++) {
    // outside intervals
    if (lIndex % 2 == 0) {
      if (inter.lb >= mList[lIndex/2].lb) {
	lIndex++;
      } else {
	break;
      }
    }
    // inside an interval
    else {
      if (inter.lb > mList[lIndex/2].hb) {
	lIndex++;
      } else {
	break;
      }
    }
  }

  // Compute higher index
  for (unsigned i=0; i<= n; i++) {
    // outside intervals
    if (hIndex % 2 == 0) {
      if (inter.hb <= mList[hIndex/2 - 1].hb) {
	hIndex--;
      } else {
	break;
      }
    }
    // inside an interval
    else {
      if (inter.hb < mList[hIndex/2].lb) {
	hIndex--;
      } else {
	break;
      }
    }
  }

  //  std::cerr << "lIndex=" << lIndex << " hIndex=" << hIndex << "\n";

  // Case where the bounds have the same index
  if (lIndex == hIndex) {
    // Inside an interval.
    if (lIndex % 2 == 1) {
      unsigned idx = lIndex / 2;

      // Exact match: remove the interval.
      if (mList[idx].lb == inter.lb && mList[idx].hb == inter.hb) {
	mList.erase(mList.begin() + idx);
	return;
      }

      // Inside not touching: we have to create a new interval.
      if (mList[idx].lb < inter.lb && mList[idx].hb > inter.hb) {
	mList.insert(mList.begin() + idx + 1,
		     Interval(inter.hb + 1, mList[idx].hb));
	mList[idx].hb = inter.lb - 1;

	return;
      }

      // Inside touching one side
      if (mList[idx].lb == inter.lb) {
	mList[idx].lb = inter.hb + 1;
      } else {
	mList[idx].hb = inter.lb - 1;
      }

      return;
    }

    // Outside any interval: Do nothing.
    else {
      return;
    }
  }

  // Bounds have different indexes.
  // First update bounds of intervals touched, then remove the
  // inbetween intervals.
  int lowerDelIdx;
  int higherDelIdx;

  // Lower bound inside an interval: update its higher bound.
  if (lIndex % 2 == 1) {
    // Handle case where lower bound is touching.
    if (mList[lIndex / 2].lb == inter.lb) {
      lowerDelIdx = lIndex / 2;
    } else {
      mList[lIndex / 2].hb = inter.lb - 1;
      lowerDelIdx = lIndex / 2 + 1;
    }
  } else {
    lowerDelIdx = lIndex / 2;
  }

  // Higher bound inside an interval: update its lower bound.
  if (hIndex % 2 == 1) {
    // Handle case where higher bound is touching.
    if (mList[hIndex / 2].hb == inter.hb) {
      higherDelIdx = hIndex / 2;
    } else {
      mList[hIndex / 2].lb = inter.hb + 1;
      higherDelIdx = hIndex / 2 - 1;
    }
  } else {
    higherDelIdx = hIndex / 2 - 1;
  }

  // std::cerr << "lowerDelIdx=" << lowerDelIdx
  // 	    << " higherDelIdx=" << higherDelIdx << "\n";

  // Remove intervals between.
  if (higherDelIdx >= lowerDelIdx)
    mList.erase(mList.begin() + lowerDelIdx,
		mList.begin() + higherDelIdx + 1);
  return;
}

void
ListInterval::clear() {
  mList.clear();
  undefined = false;
}

void
ListInterval::clearList() {
  mList.clear();
}

ListInterval *
ListInterval::clone() const {
  ListInterval *ret = new ListInterval();
  ret->mList = std::vector<Interval>(mList);
  ret->undefined = undefined;
  return ret;
}

void
ListInterval::myUnion(const ListInterval &l) {
  for (unsigned i=0; i<l.mList.size(); i++)
    add(l.mList[i]);
}

void
ListInterval::difference(const ListInterval &l) {
  for (unsigned i=0; i<l.mList.size(); i++)
    remove(l.mList[i]);
}

ListInterval *
ListInterval::intersection(const ListInterval &l1, const ListInterval &l2) {
  ListInterval *ret = new ListInterval();

  for (unsigned i=0; i<l1.mList.size(); i++) {
    for (unsigned j=0; j<l2.mList.size(); j++) {
      Interval res(0,0);
      if(Interval::intersection(l1.mList[i], l2.mList[j], res))
  	ret->add(res);
    }
  }

  return ret;
}

ListInterval *
ListInterval::difference(const ListInterval &l1,
			 const ListInterval &l2) {
  ListInterval *ret = l1.clone();

  for (unsigned i=0; i<l2.mList.size(); i++)
    ret->remove(l2.mList[i]);

  return ret;
}

void
ListInterval::setUndefined() {
  undefined = true;
}

bool
ListInterval::isUndefined() const {
  return undefined;
}

size_t
ListInterval::total() const {
  size_t total = 0;
  for (unsigned i=0;i<mList.size(); ++i)
    total += mList[i].hb - mList[i].lb;

  return total;
}

void ListInterval::debug() const {
  std::cerr << "{";
  for (unsigned i=0; i<mList.size(); ++i)
    mList[i].debug();
  std::cerr << "}";
}

std::string ListInterval::toString() const {
  std::stringstream ss;
  ss << "{";
  for (unsigned i=0; i<mList.size(); ++i)
    ss << mList[i].toString();
  ss << "}";
  return ss.str();
}
