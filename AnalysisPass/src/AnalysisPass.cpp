#include "AnalysisPass.h"
#include "ConditionBuilder.h"
#include "IndexExprBuilder.h"
#include "Loops.h"

#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Argument.h"
#include "llvm/Function.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/InstIterator.h"

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
  au.addRequired<LoopInfo>();
  au.addRequired<PostDominatorTree>();
  au.addRequired<ScalarEvolution>();
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

#ifdef DEBUG
  errs() << "\033[1;31mKernel " << F.getName() << ":\033[0m\n";
#endif /* DEBUG */

  // Get analysis passes
  loopInfo = &getAnalysis<LoopInfo>();
  scalarEvolution = &getAnalysis<ScalarEvolution>();
  PDT = &getAnalysis<PostDominatorTree>();

  // Get Data Layout
  const std::string &dataLayoutStr = F.getParent()->getDataLayout();
  dataLayout = new DataLayout(dataLayoutStr);


  indexExprBuilder = new IndexExprBuilder(loopInfo, scalarEvolution,
					  dataLayout, syclRangeType);
  conditionBuilder = new ConditionBuilder(*PDT, indexExprBuilder);

  analyze(&F);

  // Get global arguments (address space 1)
  TinyPtrVector<Argument *> args;
  getGlobalArguments(args, F);

  // Get constant arguments;
  getConstantArguments(args, F);

#ifdef DEBUG
  errs() << "\033[1;31m" << args.size() << " global args\033[0m\n";
#endif /* DEBUG */

  std::vector<ArgumentAnalysis *> argsAnalysis;

  for (Argument *arg : args) {
    Type *argTy = arg->getType();
    PointerType *PT = cast<PointerType>(argTy);
    Type *elemTy = PT->getElementType();
    unsigned sizeInBytes = dataLayout->getTypeAllocSize(elemTy);
    ArgumentAnalysis *argAnalysis =
      new ArgumentAnalysis(arg->getArgNo(), sizeInBytes,
			   arg2LoadExprs[arg], arg2StoreExprs[arg]);
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
				  bool isWrite) {
  if (!arg || !(isGlobalArgument(arg) || isConstantArgument(arg)))
    return;

  std::vector<GuardExpr *> *guards =
    conditionBuilder->buildBasicBlockGuards(inst->getParent());

  if (isWrite)
    arg2StoreExprs[arg].push_back(new WorkItemExpr(expr, guards));
  else
    arg2LoadExprs[arg].push_back(new WorkItemExpr(expr, guards));
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
      computeWorkItemExpr(inst, indexExpr, arg, false);
    }

    if (isa<StoreInst>(inst)) {
      StoreInst *SI = cast<StoreInst>(inst);
      IndexExpr *indexExpr = NULL;
      const llvm::Argument *arg = NULL;
      indexExprBuilder->buildStoreExpr(SI, &indexExpr, &arg);
      computeWorkItemExpr(inst, indexExpr, arg, true);
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
	computeWorkItemExpr(inst, storeExpr, storeArg, true);
	computeWorkItemExpr(inst, loadExpr, loadArg, false);
      }

      if (funcName.find("llvm.memset") != StringRef::npos) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->buildMemsetExpr(CI, &expr, &arg);
	computeWorkItemExpr(inst, expr, arg, true);
      }

      if (funcName.find("atomic_") != StringRef::npos ||
	  funcName.find("atom_") != StringRef::npos) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->build_atom_Expr(CI, &expr, &arg);
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
