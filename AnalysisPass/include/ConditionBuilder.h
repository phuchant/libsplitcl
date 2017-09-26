#ifndef CONDITIONBUILDER_H
#define CONDITIONBUILDER_H

#include "IndexExprBuilder.h"

#include "GuardExpr.h"
#include "IndexExpr/IndexExprs.h"

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

class ConditionBuilder {
public:
  ConditionBuilder(llvm::PostDominatorTree &PDT,
		   IndexExprBuilder *indexExprBuilder);
  ~ConditionBuilder();

  std::vector<GuardExpr *> * buildBasicBlockGuards(llvm::BasicBlock *BB);

private:
  llvm::PostDominatorTree &PDT;
  IndexExprBuilder *indexExprBuilder;

  void findConditions(llvm::BasicBlock *BB,
		      std::vector<const llvm::ICmpInst *> &conds,
		      std::vector<bool> &condsTruth);

  unsigned computeExprNumOclCalls(IndexExpr *expr);

  bool normalizeOCLCondition(IndexExpr **exprId, IndexExpr **expr,
			     bool *operandSwitched);

  void pushGuardExprLeft(std::vector<GuardExpr *> *guards,
			 IndexExprOCL *oclExpr, IndexExpr *expr,
			 const llvm::ICmpInst *cond, bool truth);

  void pushGuardExprRight(std::vector<GuardExpr *> *guards,
			  IndexExprOCL *oclExpr, IndexExpr *expr,
			  const llvm::ICmpInst *cond, bool truth);
};


#endif /* CONDITIONBUILDER_H */
