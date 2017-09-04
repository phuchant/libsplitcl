#include "AnalysisPass.h"
#include "ConditionBuilder.h"
#include "IndexExprBuilder.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/InstIterator.h"

#include <GuardExpr.h>
#include <ArgumentAnalysis.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

using namespace llvm;
using namespace std;

static cl::opt<std::string> KernelName("kernelname",
				       cl::init(""),
				       cl::desc("Specify kernel name"),
				       cl::value_desc("kernel name"));

AnalysisPass::AnalysisPass() : FunctionPass(ID), conditionBuilder(NULL),
			       indexExprBuilder(NULL) {}

void
AnalysisPass::getAnalysisUsage(AnalysisUsage &au) const {
  au.setPreservesAll();
  au.addRequired<LoopInfoWrapperPass>();
  au.addRequired<PostDominatorTreeWrapperPass>();
  au.addRequired<ScalarEvolutionWrapperPass>();
}

bool
AnalysisPass::runOnFunction(Function &F) {
  MD = F.getParent();

  syclRangeType = MD->getTypeByName("class.cl::sycl::accessor_range");

  if (syclRangeType)
    errs() << "sycl range type = " << *syclRangeType << "\n";

  // Check if it is a kernel
  if (!isKernel(&F))
    return false;

  // Check kernel name if defined
  if (KernelName.compare("") && KernelName.compare(F.getName()))
    return false;

// #ifdef DEBUG
//   errs() << "\033[1;31mKernel " << F.getName() << ":\033[0m\n";
// #endif /* DEBUG */

  // Get analysis passes
  loopInfo = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  scalarEvolution = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  // Get Data Layout
  dataLayout = &F.getParent()->getDataLayout(); //new DataLayout(dataLayoutStr);


  indexExprBuilder = new IndexExprBuilder(loopInfo, scalarEvolution,
					  dataLayout, syclRangeType);
  conditionBuilder = new ConditionBuilder(*PDT, indexExprBuilder);

  analyze(&F);

  // Get global arguments (address space 1)
  TinyPtrVector<Argument *> args;
  getGlobalArguments(args, F);

  // Get constant arguments;
  getConstantArguments(args, F);

// #ifdef DEBUG
//   errs() << "\033[1;31m" << args.size() << " global args\033[0m\n";
// #endif /* DEBUG */

  std::vector<ArgumentAnalysis *> argsAnalysis;

  for (Argument *arg : args) {
    Type *argTy = arg->getType();
    PointerType *PT = cast<PointerType>(argTy);
    Type *elemTy = PT->getElementType();
    unsigned sizeInBytes = dataLayout->getTypeAllocSize(elemTy);

    // FIXME: we have to differentiate unsigned and signed integer type.
    ArgumentAnalysis::TYPE enumType;
    if (elemTy->isIntegerTy()) {
      if (sizeInBytes == 1) {
	enumType = ArgumentAnalysis::BOOL;
      } else if (sizeInBytes == 2) {
	enumType = ArgumentAnalysis::SHORT;
      } else if (sizeInBytes == 4) {
	enumType = ArgumentAnalysis::INT;
      } else if (sizeInBytes == 8) {
	enumType = ArgumentAnalysis::LONG;
      } else {
	enumType = ArgumentAnalysis::UNKNOWN;
      }
    } else if (elemTy->isFloatTy()) {
      enumType = ArgumentAnalysis::FLOAT;
    } else if (elemTy->isDoubleTy()) {
      enumType = ArgumentAnalysis::DOUBLE;
    } else {
      enumType = ArgumentAnalysis::UNKNOWN;
    }

    ArgumentAnalysis *argAnalysis =
      new ArgumentAnalysis(arg->getArgNo(), enumType, sizeInBytes,
			   arg2LoadExprs[arg], arg2StoreExprs[arg],
			   arg2OrExprs[arg],
			   arg2AtomicSumExprs[arg], arg2AtomicMaxExprs[arg]);
    argsAnalysis.push_back(argAnalysis);
  }

#ifdef DEBUG
  for (unsigned i=0; i<argsAnalysis.size(); i++) {
    argsAnalysis[i]->dump();
  }
#endif

  std::vector<size_t> argsSizes;
  for (Argument &arg : F.getArgumentList()) {
    Type *argTy = arg.getType();
    unsigned sizeInBytes;
    if (!argTy->isPointerTy()) {
      sizeInBytes = dataLayout->getTypeAllocSize(argTy);
    } else {
      PointerType *PT = cast<PointerType>(argTy);
      Type *elemTy = PT->getElementType();
      sizeInBytes = dataLayout->getTypeAllocSize(elemTy);
    }
    argsSizes.push_back(sizeInBytes);
  }

  KernelAnalysis *analysis =
    new KernelAnalysis(F.getName().data(),
		       F.arg_size(),
		       argsSizes,
		       argsAnalysis,
		       hasAtomic(F) || hasGlobalBarrier(F));

  // Pass number of global arguments, ArgumentAnalysis objects,
  // number of constant arguments and their positions
  // to libhookocl using mmap
  mmapAnalysis(*analysis);

  delete analysis;

  return false;
}

void
AnalysisPass::computeWorkItemExpr(llvm::Instruction *inst,
				  IndexExpr *expr,
				  const llvm::Argument *arg,
				  WorkItemExpr::TYPE type) {
  if (!arg || !(isGlobalArgument(arg) || isConstantArgument(arg)))
    return;

  std::vector<GuardExpr *> *guards =
    conditionBuilder->buildBasicBlockGuards(inst->getParent());

  switch (type) {
  case WorkItemExpr::STORE:
    arg2StoreExprs[arg].push_back(new WorkItemExpr(expr, guards));
    return;
  case WorkItemExpr::LOAD:
    arg2LoadExprs[arg].push_back(new WorkItemExpr(expr, guards));
    return;
  case WorkItemExpr::OR:
    arg2OrExprs[arg].push_back(new WorkItemExpr(expr, guards));
    return;
  case WorkItemExpr::ATOMICSUM:
    arg2AtomicSumExprs[arg].push_back(new WorkItemExpr(expr, guards));
    return;
  case WorkItemExpr::ATOMICMAX:
    arg2AtomicMaxExprs[arg].push_back(new WorkItemExpr(expr, guards));
    return;
  }
}


void
AnalysisPass::analyze(Function *F) {
  for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;

    // We are only interested in store/load instructions and memory instrinsics.

    if (isa<LoadInst>(inst)) {
      LoadInst *LI = cast<LoadInst>(inst);
      IndexExpr *indexExpr = NULL;
      const llvm::Argument *arg = NULL;
      indexExprBuilder->buildLoadExpr(LI, &indexExpr, &arg);
      computeWorkItemExpr(inst, indexExpr, arg, WorkItemExpr::LOAD);
    }

    if (isa<StoreInst>(inst)) {
      StoreInst *SI = cast<StoreInst>(inst);
      IndexExpr *indexExpr = NULL;
      const llvm::Argument *arg = NULL;
      indexExprBuilder->buildStoreExpr(SI, &indexExpr, &arg);

      WorkItemExpr::TYPE exprType = WorkItemExpr::STORE;

      // Hack to handle stop variable.
      // This is necessary as long as no pragma is implemented.
      if ( (F->getName().equals("vector_diff") &&
	    arg->getName().equals("stop")) ||
	   (F->getName().equals("color") && arg->getName().equals("stop")) ||
	   (F->getName().equals("mis1") && arg->getName().equals("stop")) ||
	   (F->getName().equals("bfs_kernel") && arg->getName().equals("cont")))
	exprType = WorkItemExpr::OR;

      computeWorkItemExpr(inst, indexExpr, arg, exprType);
    }

    if (isa<CallInst>(inst)) {
      CallInst *CI = cast<CallInst>(inst);
      // memcpy, memset, memmove, atomic_

      StringRef funcName = CI->getCalledFunction()->getName();

      if (funcName.find("llvm.memcpy") != StringRef::npos) {
	IndexExpr *storeExpr = NULL;
	IndexExpr *loadExpr = NULL;
	const Argument *storeArg = NULL;
	const Argument *loadArg = NULL;
	indexExprBuilder->buildMemcpyLoadExpr(CI, &loadExpr, &loadArg);
	indexExprBuilder->buildMemcpyStoreExpr(CI, &storeExpr, &storeArg);
	computeWorkItemExpr(inst, storeExpr, storeArg, WorkItemExpr::STORE);
	computeWorkItemExpr(inst, loadExpr, loadArg, WorkItemExpr::LOAD);
      }

      else if (funcName.find("llvm.memset") != StringRef::npos) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->buildMemsetExpr(CI, &expr, &arg);
	computeWorkItemExpr(inst, expr, arg, WorkItemExpr::STORE);
      }

      else if (funcName.equals("_Z10atomic_maxPU7CLlocalVii")) {
	// Do nothing
      }

      else if (funcName.equals("atomic_add_float") ||
	       funcName.equals("_Z10atomic_incPU8CLglobalVj")) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->build_atom_Expr(CI, &expr, &arg);
	computeWorkItemExpr(inst, expr, arg, WorkItemExpr::ATOMICSUM);
      }

      // Only int and long (signed or unsigned)
      else if (funcName.equals("_Z10atomic_maxPU8CLglobalVii")) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->build_atom_Expr(CI, &expr, &arg);
	computeWorkItemExpr(inst, expr, arg, WorkItemExpr::ATOMICMAX);
      }

      else {
	errs() << "WARNING: unhandled call inst : " << *CI << "\n";
      }
    }
  }
}

void
AnalysisPass::mmapAnalysis(const KernelAnalysis &analysis) {
  std::stringstream ss;
  analysis.write(ss);
  std::string str = ss.str();

  int fd;
  char *data;
  const char *c_str = str.c_str();

  unsigned allocSize = 64 * getpagesize();

  if (str.size() > allocSize) {
    errs() << "Kernel Analysis ERROR: allocSize=" << allocSize
	   << " stringSize=" << str.size() << "\n";
    exit(EXIT_FAILURE);
  }

  if ((fd = open("analysis.txt",
		 O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR)) == -1) {
    perror("analysis open");
  } else {
    if ((lseek(fd, allocSize + 1, SEEK_SET)) == -1)
      perror("lseek");
    if (write(fd, "", 1) != 1)
      perror("write");
    if (lseek(fd, 0, SEEK_SET) == -1)
      perror("lseek");

    data = static_cast<char *>(mmap((caddr_t) 0, allocSize,
				    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    if (data == (char *) (caddr_t) -1) {
      perror("analysis mmap");
    } else {
      memcpy(data, c_str, str.size());
    }
  }
}

char AnalysisPass::ID = 0;
static RegisterPass<AnalysisPass>
X("klanalysis", "Kernel Analysis Pass", false, false);
