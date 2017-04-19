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
#include "llvm/Constants.h"
#include "llvm/DataLayout.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Metadata.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Type.h"
#include "llvm/Value.h"

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

    llvm::DataLayout *dataLayout;
    llvm::Type *syclRangeType;

    std::map<const llvm::Argument *, std::vector<WorkItemExpr *> >
    arg2LoadExprs;
    std::map<const llvm::Argument *, std::vector<WorkItemExpr *> >
    arg2StoreExprs;

    void analyze(llvm::Function *F);

    ConditionBuilder *conditionBuilder;
    IndexExprBuilder *indexExprBuilder;

    // This function map the analysis.
    void mmapAnalysis(const KernelAnalysis &analysis);

    // This function compute the WorkItemExpr of an instruction.
    void computeWorkItemExpr(llvm::Instruction *inst,
			     IndexExpr *expr,
			     const llvm::Argument *arg,
			     bool isWrite);
  };
}

#endif /* ANALYSISPASS_H */
