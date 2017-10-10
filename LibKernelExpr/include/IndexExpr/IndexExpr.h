#ifndef INDEXEXPR_H
#define INDEXEXPR_H

#include "../GuardExpr.h"
#include "../NDRange.h"

#include <fstream>
#include <sstream>
#include <vector>

class GuardExpr;
class IndirectionValue;

class IndexExpr {
public:
  IndexExpr(unsigned tag);
  virtual ~IndexExpr();

  virtual void dump() const = 0;
  virtual IndexExpr *clone() const = 0;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const;


  virtual void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;

  void toDot(std::string filename) const;
  virtual void toDot(std::stringstream &stream) const;

  unsigned getTag() const;
  unsigned getID() const;

  static bool computeBounds(const IndexExpr *expr, long *lb, long *hb);

  static void injectArgsValues(IndexExpr *expr, const std::vector<int> &values);
  static void injectIndirValues(IndexExpr *expr,
				const std::vector<std::pair<int, int> >&values);
  static IndexExpr *open(std::stringstream &s);
  static IndexExpr *openFromFile(const std::string &name);

  enum tags {
    NIL = 0,
    CONST = 1,
    ARG = 2,
    OCL = 3,
    BINOP = 4,
    INTERVAL = 5,
    UNKNOWN = 6,
    MIN = 7,
    MAX = 8,
    LB = 9,
    HB = 10,
    INDIR = 11,
  };

protected:
  unsigned tag;
  unsigned id;
  static unsigned idIndex;

  void writeNIL(std::stringstream &s) const;
};

#endif /* INDEXEXPR_H */
