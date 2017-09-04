#ifndef ANALYSISPASS_H
#define ANALYSISPASS_H

#include "ConditionBuilder.h"
#include "IndexExprBuilder.h"

#include "IndexExpr/IndexExprs.h"
#include "KernelAnalysis.h"
#include "NDRange.h"
#include "Utils.h"

#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include <map>
#include <set>
#include <vector>

namespace {
  class AnalysisPass : public llvm::FunctionPass {
  public:
    static char ID;
    AnalysisPass();

    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const;
    virtual bool runOnFunction(llvm::Function &F);

  private:
    llvm::Module *MD;

    llvm::PostDominatorTree *PDT;
    llvm::LoopInfo *loopInfo;
    llvm::ScalarEvolution *scalarEvolution;

    const llvm::DataLayout *dataLayout;
    llvm::Type *syclRangeType;

    std::map<const llvm::Argument *, std::vector<WorkItemExpr *> >
    arg2LoadExprs;
    std::map<const llvm::Argument *, std::vector<WorkItemExpr *> >
    arg2StoreExprs;
    std::map<const llvm::Argument *, std::vector<WorkItemExpr *> >
    arg2OrExprs;
    std::map<const llvm::Argument *, std::vector<WorkItemExpr *> >
    arg2AtomicSumExprs;
    std::map<const llvm::Argument *, std::vector<WorkItemExpr *> >
    arg2AtomicMaxExprs;

    void analyze(llvm::Function *F);

    ConditionBuilder *conditionBuilder;
    IndexExprBuilder *indexExprBuilder;

    // This function map the analysis.
    void mmapAnalysis(const KernelAnalysis &analysis);

    // This function compute the WorkItemExpr of an instruction.
    void computeWorkItemExpr(llvm::Instruction *inst,
			     IndexExpr *expr,
			     const llvm::Argument *arg,
			     WorkItemExpr::TYPE type);
  };
}

#endif /* ANALYSISPASS_H */
