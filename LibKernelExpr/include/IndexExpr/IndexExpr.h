#ifndef INDEXEXPR_H
#define INDEXEXPR_H

#include <fstream>
#include <sstream>
#include <vector>

class GuardExpr;
class IndirectionValue;
class IndexExprValue;
class NDRange;

class IndexExpr {
public:
  enum tagTy {
    NIL = 0,
    VALUE = 1,
    CAST = 2,
    ARG = 3,
    OCL = 4,
    BINOP = 5,
    INTERVAL = 6,
    UNKNOWN = 7,
    MIN = 8,
    MAX = 9,
    LB = 10,
    HB = 11,
    INDIR = 12,
  };

  explicit IndexExpr(tagTy tag);
  virtual ~IndexExpr();

  virtual void dump() const = 0;
  virtual IndexExpr *clone() const = 0;
  virtual IndexExpr *getWorkgroupExpr(const NDRange &ndRange) const;
  virtual IndexExpr *getKernelExpr(const NDRange &ndRange,
				   const std::vector<GuardExpr *> & guards,
				   const std::vector<IndirectionValue> &
				   indirValues) const = 0;


  virtual void write(std::stringstream &s) const;
  void writeToFile(const std::string &name) const;

  void toDot(std::string filename) const;
  virtual void toDot(std::stringstream &stream) const;

  tagTy getTag() const;
  unsigned getID() const;

  static bool computeBounds(const IndexExpr *expr, long *lb, long *hb);

  static void injectArgsValues(IndexExpr *expr,
			       const std::vector<IndexExprValue *> &values);
  static void injectIndirValues(IndexExpr *expr,
				const std::vector<std::pair<IndexExprValue *,
							    IndexExprValue *>>

				  &values);
  static IndexExpr *open(std::stringstream &s);
  static IndexExpr *openFromFile(const std::string &name);


  enum value_type {
    LONG,
    FLOAT,
    DOUBLE
  };

protected:

  typedef union {
    long n;
    float f;
    double d;
  } value;

  tagTy tag;
  unsigned id;
  static unsigned idIndex;

  void writeNIL(std::stringstream &s) const;

private:

  static bool computeBoundsRec(const IndexExpr *expr, value *lb, value *hb,
			       value_type *type);
};

#endif /* INDEXEXPR_H */
