#ifndef INDEXEXPRUNKNOWN_H
#define INDEXEXPRUNKNOWN_H

#include "IndexExpr.h"

class IndexExprUnknown : public IndexExpr {
public:
  IndexExprUnknown(const std::string &str);
  virtual ~IndexExprUnknown();

  virtual void dump() const;
  virtual IndexExpr *clone() const;
  virtual void toDot(std::stringstream &stream) const;

 private:
  std::string str;
};

#endif /* INDEXEXPRUNKNOWN_H */
