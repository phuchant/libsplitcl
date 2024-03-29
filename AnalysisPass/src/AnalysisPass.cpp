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

static cl::opt<bool> optDump("dump",
				       cl::init(false),
				       cl::desc("Dump analysis"),
				       cl::value_desc("dump analysis"));

static cl::opt<bool> optList("list",
			     cl::init(false),
			     cl::desc("List kernel names"),
			     cl::value_desc("list kernel names"));

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

  // Check if it is a kernel
  if (!isKernel(&F))
    return false;

  if (optList) {
    errs() << F.getName() << ",";
    return false;
  }

  // Check kernel name if defined
  if (KernelName.compare("") && KernelName.compare(F.getName()))
    return false;

  // Get analysis passes
  loopInfo = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  scalarEvolution = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  // Get Data Layout
  dataLayout = &F.getParent()->getDataLayout();


  indexExprBuilder = new IndexExprBuilder(loopInfo, scalarEvolution,
					  dataLayout);

  conditionBuilder = new ConditionBuilder(*PDT, indexExprBuilder);

  analyze(&F);

  // Get global arguments (address space 1)
  TinyPtrVector<Argument *> args;
  getGlobalArguments(args, F);

  // Get constant arguments;
  getConstantArguments(args, F);

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
			   arg2OrExprs[arg], arg2AtomicSumExprs[arg],
			   arg2AtomicMinExprs[arg], arg2AtomicMaxExprs[arg]);
    argsAnalysis.push_back(argAnalysis);
  }

  bool *argIsScalar = new bool(F.arg_size());
  std::vector<size_t> scalarArgsSizes;
  std::vector<ArgumentAnalysis::TYPE> scalarArgsTypes;
  unsigned argIdx = 0;

#if LLVM_VERSION_MAJOR >= 5
  for (Argument &arg : F.args()) {
#else
  for (Argument &arg : F.getArgumentList()) {
#endif
    Type *argTy = arg.getType();
    unsigned sizeInBytes;

    // Non scalar args
    if (argTy->isPointerTy()) {
      argIsScalar[argIdx] = false;
      scalarArgsSizes.push_back(0);
      scalarArgsTypes.push_back(ArgumentAnalysis::UNKNOWN);
    }

    // Scalar args
    else {
      argIsScalar[argIdx] = true;
      sizeInBytes = dataLayout->getTypeAllocSize(argTy);
      scalarArgsSizes.push_back(sizeInBytes);
      if (argTy->isDoubleTy()) {
	scalarArgsTypes.push_back(ArgumentAnalysis::DOUBLE);
      } else if (argTy->isFloatTy()) {
	scalarArgsTypes.push_back(ArgumentAnalysis::FLOAT);
      } else if (argTy->isIntegerTy()) {
	ArgumentAnalysis::TYPE scalarTy;
	switch (sizeInBytes) {
	case 1:
	  scalarTy = ArgumentAnalysis::CHAR;
	  break;
	case 2:
	  scalarTy = ArgumentAnalysis::SHORT;
	  break;
	case 4:
	  scalarTy = ArgumentAnalysis::INT;
	  break;
	case 8:
	  scalarTy = ArgumentAnalysis::LONG;
	  break;
	default:
	  scalarTy = ArgumentAnalysis::UNKNOWN;
	};
	scalarArgsTypes.push_back(scalarTy);
      } else {
	scalarArgsTypes.push_back(ArgumentAnalysis::UNKNOWN);
      }
    }

    argIdx++;
  }

  std::vector<ArgIndirectionRegionExpr *> indirectionExprs;
  unsigned numIndirections = indexExprBuilder->getNumIndirections();
  for (unsigned i=0; i<numIndirections; i++) {
    const LoadIndirectionExpr *expr = indexExprBuilder->getIndirection(i);
    const std::vector<GuardExpr *> *guards =
      conditionBuilder->buildBasicBlockGuards(expr->inst->getParent());
    unsigned id = expr->id;
    unsigned pos = expr->arg->getArgNo();
    unsigned numBytes = expr->numBytes;
    IndirectionType indirTy = UNDEF;
    LoadInst *LI = expr->inst;
    Type *ptrOpTy = LI->getPointerOperand()->getType();
    PointerType *ptrTy = dyn_cast<PointerType>(ptrOpTy);
    assert(ptrTy);
    Type *elemTy = ptrTy->getElementType();
    if (elemTy->isIntegerTy())
      indirTy = INT;
    else if (elemTy->isFloatTy())
      indirTy = FLOAT;
    else if (elemTy->isDoubleTy())
      indirTy = DOUBLE;

    WorkItemExpr *wi_expr = new WorkItemExpr(*(expr->expr), *guards);
    indirectionExprs.push_back(new ArgIndirectionRegionExpr(id, pos, numBytes,
							    indirTy, wi_expr));
  }

  KernelAnalysis *analysis =
    new KernelAnalysis(F.getName().data(),
		       F.arg_size(),
		       argIsScalar,
		       scalarArgsSizes,
		       scalarArgsTypes,
		       argsAnalysis,
		       indirectionExprs);
  delete argIsScalar;

  if (optDump)
    analysis->debug();

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
  case WorkItemExpr::ATOMICMIN:
    arg2AtomicMinExprs[arg].push_back(new WorkItemExpr(expr, guards));
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

      // Atomic local
      else if (funcName.equals("_Z10atomic_maxPU7CLlocalVii") ||
	       funcName.equals("_Z10atomic_addPU7CLlocalVjj") ||
	       funcName.equals("_Z10atomic_addPU7CLlocalVii") ||
	       funcName.equals("_Z8atom_addPU7CLlocalVii")) {
	// Do nothing
      }

      // Atomic XCHG (considered as a store)
      else if (funcName.equals("_Z11atomic_xchgPU8CLglobalVii") ||
	       funcName.equals("_Z9atom_xchgPU8CLglobalVii")) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->build_atom_Expr(CI, &expr, &arg);
	computeWorkItemExpr(inst, expr, arg, WorkItemExpr::STORE);
      }

      // Atomic Sum global
      else if (funcName.equals("atomic_add_float") ||
	       funcName.equals("_Z10atomic_incPU8CLglobalVj") ||
	       funcName.equals("_Z10atomic_incPU8CLglobalVi") ||
	       funcName.equals("_Z10atomic_addPU8CLglobalVii") ||
	       funcName.equals("_Z10atomic_addPU8CLglobalVjj") ||
	       funcName.equals("_Z8atom_addPU8CLglobalVii") ||
	       funcName.equals("_Z8atom_incPU8CLglobalVi") ||
	       funcName.equals("_Z8atom_decPU8CLglobalVi")) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->build_atom_Expr(CI, &expr, &arg);
	computeWorkItemExpr(inst, expr, arg, WorkItemExpr::ATOMICSUM);
      }

      // Atomic Min global
      else if (funcName.equals("_Z8atom_minPU8CLglobalVii")) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->build_atom_Expr(CI, &expr, &arg);
	computeWorkItemExpr(inst, expr, arg, WorkItemExpr::ATOMICMIN);
      }

      // Atomic Max global
      // Only int and long (signed or unsigned)
      else if (funcName.equals("_Z10atomic_maxPU8CLglobalVii")) {
	IndexExpr *expr = NULL; const Argument *arg = NULL;
	indexExprBuilder->build_atom_Expr(CI, &expr, &arg);
	computeWorkItemExpr(inst, expr, arg, WorkItemExpr::ATOMICMAX);
      }

      // Ignored functions
      else if (funcName.equals("_Z13get_global_idj") ||
	       funcName.equals("_Z12get_group_idj") ||
	       funcName.equals("_Z12get_local_idj") ||
	       funcName.equals("_Z15get_global_sizej") ||
	       funcName.equals("_Z14get_local_sizej") ||
	       funcName.equals("_Z14get_num_groupsj") ||
	       funcName.equals("_Z7barrierj") ||
	       funcName.equals("llvm.fmuladd.f32") ||
	       funcName.equals("llvm.fmuladd.f64") ||
	       funcName.equals("_Z3expf") ||
	       funcName.equals("_Z3expd") ||
	       funcName.equals("_Z3sqrtf") ||
	       funcName.equals("_Z4sqrtd") ||
	       funcName.equals("_Z3logf") ||
	       funcName.equals("_Z3logd") ||
	       funcName.equals("_Z3powff") ||
	       funcName.equals("_Z3powdd") ||
	       funcName.equals("_Z4fmodff") ||
	       funcName.equals("_Z5log10f") ||
	       funcName.equals("_Z15convert_int_rtnd") ||
	       funcName.equals("llvm.fmuladd.v3f32") ||
	       funcName.equals("_Z8distanceDv3_fS_") ||
	       funcName.equals("_Z9normalizeDv3_f") ||
	       funcName.equals("_Z3minii") ||
	       funcName.equals("_Z5hypotff") ||
	       funcName.equals("_Z3maxii") ||
	       funcName.equals("_Z5atan2ff") ||
	       funcName.equals("_Z3minjj") ||
	       funcName.equals("_Z4fabsf") ||
	       funcName.equals("_Z4fmaxff") ||
	       funcName.equals("_Z4sqrtf") ||
	       funcName.equals("_Z3maxff") ||
	       funcName.equals("_Z3dotDv4_fS_") ||
	       funcName.equals("llvm.fmuladd.v2f32") ||
	       funcName.equals("llvm.lifetime.start") ||
	       funcName.equals("llvm.lifetime.end") ||
	       funcName.equals("_Z4acosf") ||
	       funcName.equals("_Z4asinf") ||
	       funcName.equals("_Z4atanf") ||
	       funcName.equals("_Z4ceilf") ||
	       funcName.equals("_Z3cosf") ||
	       funcName.equals("_Z4coshf") ||
	       funcName.equals("_Z3sinf") ||
	       funcName.equals("_Z3tanf") ||
	       funcName.equals("_Z4tanhf") ||
	       funcName.equals("_Z4sinhf") ||
	       funcName.equals("_Z5floorf") ||
	       funcName.equals("_Z3maxjj") ||
	       funcName.equals("_Z3dotDv3_fS_")) {
	// Do nothing
	continue;
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
