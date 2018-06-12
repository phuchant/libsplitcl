#ifndef INDEXEXPRBUILDER_H
#define INDEXEXPRBUILDER_H

#include "IndexExpr/IndexExpr.h"

#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/DataLayout.h"

struct LoadIndirectionExpr {
  LoadIndirectionExpr(unsigned id,
		      const llvm::Argument *arg,
		      unsigned numBytes,
		      IndexExpr *expr,
		      llvm::LoadInst *inst)
    : id(id), arg(arg), numBytes(numBytes), expr(expr), inst(inst) {}
  ~LoadIndirectionExpr() {
    delete expr;
  }

  unsigned id;
  const llvm::Argument *arg;
  unsigned numBytes;
  IndexExpr *expr;
  llvm::LoadInst *inst;
};

class IndexExprBuilder {
public:
  IndexExprBuilder(llvm::LoopInfo *loopInfo,
		   llvm::ScalarEvolution *scalarEvolution,
		   const llvm::DataLayout *dataLayout);
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

  void disableIndirections();
  void enableIndirections();
  bool areIndirectionsEnabled();

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

  unsigned getNumIndirections() const;
  const LoadIndirectionExpr *getIndirection(unsigned n);

private:
  llvm::LoopInfo *loopInfo;
  llvm::ScalarEvolution *scalarEvolution;

  const llvm::DataLayout *dataLayout;

  void parseSCEV(const llvm::SCEV *scev, IndexExpr **indexExpr,
		 const llvm::Argument **arg);

  IndexExpr *tryComputeLoopBackedCount(llvm::Loop *L);
  IndexExpr *tryComputeLoopStart(llvm::Loop *L);
  IndexExpr *tryComputeLoopStep(llvm::Loop *L);
  bool computingBackedge;
  bool computingMemcpy;

  /* Indirections */
  bool indirectionsDisabled;
  bool buildingIndirection;
  bool doubleIndirectionReached;
  unsigned numIndirections;
  std::map<llvm::LoadInst *, unsigned> load2IndirectionID;
  std::vector<LoadIndirectionExpr *> indirections;
};

#endif /* INDEXEXPRBUILDER_H */
