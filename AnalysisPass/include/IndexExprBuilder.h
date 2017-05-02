#ifndef INDEXEXPRBUILDER_H
#define INDEXEXPRBUILDER_H

#include "IndexExpr/IndexExpr.h"

#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/DataLayout.h"

class IndexExprBuilder {
public:
  IndexExprBuilder(llvm::LoopInfo *loopInfo,
		   llvm::ScalarEvolution *scalarEvolution,
		   const llvm::DataLayout *dataLayout,
		   llvm::Type *syclRangeType);
  ~IndexExprBuilder();

  void buildLoadExpr(llvm::LoadInst *LI, IndexExpr **expr,
		     const llvm::Argument **arg);
  void buildStoreExpr(llvm::StoreInst *LI, IndexExpr **expr,
		      const llvm::Argument **arg);

  void buildMemcpyLoadExpr(llvm::CallInst *CI, IndexExpr **expr,
		     const llvm::Argument **arg);
  void buildMemcpyStoreExpr(llvm::CallInst *CI, IndexExpr **expr,
		     const llvm::Argument **arg);

  void buildMemsetExpr(llvm::CallInst *CI, IndexExpr **expr,
		     const llvm::Argument **arg);

  void build_atom_Expr(llvm::CallInst *CI, IndexExpr **expr,
		       const llvm::Argument **arg);

  // This function compute an IndexExpr for a value
  // Example:
  //
  // %call = call get_global_id(i32 0)
  // %add = add i64 %call, 1
  // %arrayidx = mul %add, %arg0
  //
  // buildExpr(arrayidx)
  // -> (get_global_id(0) + 1) * arg0
  IndexExpr *buildExpr(llvm::Value *value);

private:
  llvm::LoopInfo *loopInfo;
  llvm::ScalarEvolution *scalarEvolution;

  const llvm::DataLayout *dataLayout;
  llvm::Type *syclRangeType;

  void parseSCEV(const llvm::SCEV *scev, IndexExpr **indexExpr,
		 const llvm::Argument **arg);

  IndexExpr *tryComputeLoopBackedCount(llvm::Loop *L);
};

#endif /* INDEXEXPRBUILDER_H */
