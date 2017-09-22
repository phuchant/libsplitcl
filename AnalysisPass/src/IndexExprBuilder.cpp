#include "IndexExprBuilder.h"
#include "Utils.h"

#include "IndexExpr/IndexExprs.h"

#include "llvm/Analysis/ScalarEvolutionExpressions.h"

using namespace llvm;
using namespace std;

IndexExprBuilder::IndexExprBuilder(llvm::LoopInfo *loopInfo,
				   llvm::ScalarEvolution *scalarEvolution,
				   const llvm::DataLayout *dataLayout)
 : loopInfo(loopInfo),
   scalarEvolution(scalarEvolution),
   dataLayout(dataLayout),
   computingBackedge(false),
   indirectionsDisabled(false),
   buildingIndirection(false),
   doubleIndirectionReached(false),
   numIndirections(0) {}

IndexExprBuilder::~IndexExprBuilder() {
  for (unsigned i=0; i<indirections.size(); i++)
    delete indirections[i];
}

void
IndexExprBuilder::buildLoadExpr(llvm::LoadInst *LI, IndexExpr **expr,
				const llvm::Argument **arg) {
  const SCEV *scev = scalarEvolution->getSCEV(LI->getPointerOperand());
  parseSCEV(scev, expr, arg);
  assert(*expr);
  unsigned sizeInBytes = dataLayout->getTypeAllocSize(LI->getType());
  IndexExpr *elemSizeInterval =
    new IndexExprInterval(new IndexExprConst(0),
			  new IndexExprConst(sizeInBytes-1));
  *expr = new IndexExprBinop(IndexExprBinop::Add,
			     *expr, elemSizeInterval);
}

void
IndexExprBuilder::buildStoreExpr(llvm::StoreInst *SI, IndexExpr **expr,
			  const llvm::Argument **arg) {
  const SCEV *scev = scalarEvolution->getSCEV(SI->getPointerOperand());
  parseSCEV(scev, expr, arg);
  assert(*expr);
  unsigned sizeInBytes =
    dataLayout->getTypeAllocSize(SI->getValueOperand()->getType());
  IndexExpr *elemSizeInterval =
    new IndexExprInterval(new IndexExprConst(0),
			  new IndexExprConst(sizeInBytes-1));
  *expr = new IndexExprBinop(IndexExprBinop::Add, *expr, elemSizeInterval);
}

void
IndexExprBuilder::buildMemcpyLoadExpr(llvm::CallInst *CI, IndexExpr **expr,
			       const llvm::Argument **arg) {
  Value *src = CI->getOperand(1);
  Value *size = CI->getOperand(2);

  const SCEV *scSrc = scalarEvolution->getSCEV(src);
  const SCEV *scSize = scalarEvolution->getSCEV(size);
  IndexExpr *srcExpr, *sizeExpr;
  const Argument *srcArg = NULL, *sizeArg = NULL;
  parseSCEV(scSrc, &srcExpr, &srcArg); assert(srcExpr);
  parseSCEV(scSize, &sizeExpr, &sizeArg); assert(sizeExpr);
  IndexExpr *elemSizeInterval =
    new IndexExprInterval(new IndexExprConst(0),
			  new IndexExprBinop(IndexExprBinop::Sub,
					     sizeExpr,
					     new IndexExprConst(1)));
  srcExpr = new IndexExprBinop(IndexExprBinop::Add,
			       srcExpr,
			       elemSizeInterval->clone());
  *expr = srcExpr;
  *arg = srcArg;
}
void
IndexExprBuilder::buildMemcpyStoreExpr(llvm::CallInst *CI, IndexExpr **expr,
				const llvm::Argument **arg) {
  Value *dst = CI->getOperand(0);
  Value *size = CI->getOperand(2);

  const SCEV *scDst = scalarEvolution->getSCEV(dst);
  const SCEV *scSize = scalarEvolution->getSCEV(size);
  IndexExpr *dstExpr, *sizeExpr;
  const Argument *dstArg = NULL, *sizeArg = NULL;
  parseSCEV(scDst, &dstExpr, &dstArg); assert(dstExpr);
  parseSCEV(scSize, &sizeExpr, &sizeArg); assert(sizeExpr);
  IndexExpr *elemSizeInterval =
    new IndexExprInterval(new IndexExprConst(0),
			  new IndexExprBinop(IndexExprBinop::Sub,
					     sizeExpr,
					     new IndexExprConst(1)));
  dstExpr = new IndexExprBinop(IndexExprBinop::Add,
			       dstExpr,
			       elemSizeInterval);
  *expr = dstExpr;
  *arg = dstArg;
}

void
IndexExprBuilder::buildMemsetExpr(llvm::CallInst *CI, IndexExpr **expr,
			   const llvm::Argument **arg) {
  Value *dst = CI->getOperand(0);
  Value *len = CI->getOperand(2);

  const SCEV *scDst = scalarEvolution->getSCEV(dst);
  const SCEV *scLen = scalarEvolution->getSCEV(len);
  IndexExpr *dstExpr, *lenExpr;
  const Argument *dstArg = NULL, *lenArg = NULL;
  parseSCEV(scDst, &dstExpr, &dstArg); assert(dstExpr);
  parseSCEV(scLen, &lenExpr, &lenArg); assert(lenExpr);
  IndexExpr *elemSizeInterval =
    new IndexExprInterval(new IndexExprConst(0),
			  new IndexExprBinop(IndexExprBinop::Sub,
					     lenExpr,
					     new IndexExprConst(1)));
  dstExpr = new IndexExprBinop(IndexExprBinop::Add,
			       dstExpr,
			       elemSizeInterval);
  *expr = dstExpr;
  *arg = dstArg;
}

void
IndexExprBuilder::build_atom_Expr(llvm::CallInst *CI, IndexExpr **expr,
			   const llvm::Argument **arg) {
  const SCEV *scev = scalarEvolution->getSCEV(CI->getOperand(0));
  parseSCEV(scev, expr, arg);
  assert(*expr);
  Type *opTy = CI->getOperand(0)->getType();
  assert(isa<PointerType>(opTy));
  PointerType *PT = cast<PointerType>(opTy);
  Type *elemTy = PT->getElementType();
  unsigned sizeInBytes = dataLayout->getTypeAllocSize(elemTy);
  IndexExpr *elemSizeInterval =
    new IndexExprInterval(new IndexExprConst(0),
			    new IndexExprConst(sizeInBytes-1));
  *expr = new IndexExprBinop(IndexExprBinop::Add, *expr, elemSizeInterval);
}

IndexExpr *
IndexExprBuilder::buildExpr(Value *value) {
    /* Argument */
  if (Argument *arg = dyn_cast<Argument>(value))
    return new IndexExprArg(value->getName().str(), arg->getArgNo());

  User *user = dyn_cast<User>(value);

  /* Non user value */
  if (!user) {
    errs() << "buildexpr unknown value : " << *value << "\n";
    return new IndexExprUnknown("unknown value");
  }

  /* Constant */
  if (isa<Constant>(user)) {
    /* Integer */
    if (isa<ConstantInt>(user))
      return new IndexExprConst(cast<ConstantInt>(user)->getSExtValue());

    if (isa<UndefValue>(user))
      return new IndexExprUnknown("undef");

    /* Non integer */
    return new IndexExprUnknown("unknown constant");
  }

  /* Instruction */
  if (isa<Instruction>(user)) {
    Instruction *inst = cast<Instruction>(user);

    switch (inst->getOpcode()) {
    case Instruction::Call:
      {
	unsigned oclFunc = isOpenCLCall(inst);
	if (oclFunc != UNKNOWN) {
	  IndexExpr *ret = new IndexExprOCL(oclFunc, buildExpr(user->getOperand(0)));
	  if (computingBackedge)
	    ret = new IndexExprHB(ret);
	  return ret;
	}

	errs() << "buildexpr unknown function : " << *inst << "\n";
	return new IndexExprUnknown("unknown function");
      }

    case Instruction::PHI:
      {
	// TODO: check this code.
	Loop *L = loopInfo->getLoopFor(inst->getParent());
	if (!L)
	  return new IndexExprUnknown(inst->getName());

	IndexExpr *start, *step, *backedgeCount;

	computingBackedge = true;
	backedgeCount = tryComputeLoopBackedCount(L);
	computingBackedge = false;
	if (!backedgeCount)
	  return new IndexExprUnknown("phi");

	start = tryComputeLoopStart(L);
	if (!start) {
	  delete backedgeCount;
	  return new IndexExprUnknown("phi");
	}

	step = tryComputeLoopStep(L);
	if (!step) {
	  delete backedgeCount;
	  delete start;
	  return new IndexExprUnknown("phi");
	}

	IndexExpr *end
	  = new IndexExprBinop(IndexExprBinop::Add,
			       new IndexExprHB(start->clone()),
			       new IndexExprBinop(IndexExprBinop::Mul,
						  new IndexExprHB(step), new IndexExprLB(backedgeCount)));
	return new IndexExprInterval(start, end);
      }

    case Instruction::Add:
      return new IndexExprBinop(IndexExprBinop::Add,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::Sub:
      return new IndexExprBinop(IndexExprBinop::Sub,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::Mul:
      return new IndexExprBinop(IndexExprBinop::Mul,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::UDiv:
    case Instruction::SDiv:
      return new IndexExprBinop(IndexExprBinop::Div,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::SRem:
    case Instruction::URem:
      return new IndexExprBinop(IndexExprBinop::Mod,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));
    case Instruction::Or:
      return new IndexExprBinop(IndexExprBinop::Or,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::Xor:
      return new IndexExprBinop(IndexExprBinop::Xor,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::And:
      return new IndexExprBinop(IndexExprBinop::And,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::Shl:
      return new IndexExprBinop(IndexExprBinop::Shl,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::LShr:
    case Instruction::AShr:
      return new IndexExprBinop(IndexExprBinop::Shr,
				buildExpr(user->getOperand(0)),
				buildExpr(user->getOperand(1)));

    case Instruction::BitCast:
      return new IndexExprConst(0);

      /* Ignored instructions */
    case Instruction::Trunc:
    case Instruction::SExt:
    case Instruction::ZExt:
      return buildExpr(user->getOperand(0));

      /* Load instruction :
       *
       * Build workitem indirection expression if the argument loaded is
       * global and if this is not a double indirection.
       *
       * e.g.
       * __kernel void k(__global int *A,
       *                 __global int *B
       *                 __global int *C
       *                  __local int *D) {
       *  x = A[B[id]];    // OK
       *  y = A[B[C[id]]]; // NOK
       *  z = A[D[id]];    // NOK
       * }
       */
    case Instruction::Load:
      {

	if (indirectionsDisabled)
	  return new IndexExprUnknown("load");

	if (buildingIndirection) {
	  doubleIndirectionReached = true;
	  return new IndexExprUnknown("doubleindir");
	}

	LoadInst *load = cast<LoadInst>(user);

	// Check if an index expression has alreaby been computed for this load.
	if (load2IndirectionID.find(load) != load2IndirectionID.end()) {
	  if (computingBackedge)
	    return new IndexExprHB(new IndexExprIndirection(load2IndirectionID[load]));
	  return new IndexExprIndirection(load2IndirectionID[load]);
	}

	IndexExpr *e = NULL;
	const Argument *arg = NULL;
	buildingIndirection = true;
	buildLoadExpr(load, &e, &arg);
	buildingIndirection = false;

	if (doubleIndirectionReached) {
	  doubleIndirectionReached = false;
	  return new IndexExprUnknown("doubleindir");
	}

	if (!arg) {
	  delete e;
	  return new IndexExprUnknown("load");
	}

	// This is a single indirection for a global argument.
	// We save the indirection into a map and return an
	// IndexExprIndirection.
	unsigned numBytes = dataLayout->getTypeAllocSize(load->getType());
	unsigned id = numIndirections++;
	indirections.push_back(new LoadIndirectionExpr(id, arg, numBytes, e,
						       load));
	load2IndirectionID[load] = id;

	if (computingBackedge)
	  return new IndexExprHB(new IndexExprIndirection(id));

	return new IndexExprIndirection(id);
      }

    case Instruction::Select:
      {
	// TODO:
	// Use getCondition(), getTrueValue(), getFalseValue() to create 2
	// WorkItemExprs with different guards.
	return new IndexExprUnknown("select");
      }

    default:
      errs() << "unknown instr : " << *inst << "\n";
      return new IndexExprUnknown("Unknown instr");
    }
  }

  return new IndexExprUnknown("Unknown user");
}

IndexExpr *
IndexExprBuilder::tryComputeLoopBackedCount(Loop *L) {
  BasicBlock *exitingBlock = L->getExitingBlock();
  if (!exitingBlock)
    return NULL;

  TerminatorInst *TI = exitingBlock->getTerminator();
  BranchInst *BI = dyn_cast<BranchInst>(TI);
  if (!BI)
    return NULL;

  if (BI->isUnconditional())
    return NULL;

  if (BI->getNumSuccessors() != 2 ||
      L->contains(BI->getSuccessor(1)) ||
      !L->contains(BI->getSuccessor(0)))
    return NULL;

  Value *condition = BI->getCondition();

  ICmpInst *icmp = dyn_cast<ICmpInst>(condition);
  if (!icmp)
    return NULL;

  if (icmp->getSignedPredicate() != CmpInst::ICMP_SLT)
    return NULL;

  Value *v = icmp->getOperand(0);
  Value *truncV = NULL;
  if (isa<TruncInst>(v)) {
    TruncInst *trInst = cast<TruncInst>(v);
    truncV = trInst->getOperand(0);
  }

  BinaryOperator *bo = dyn_cast<BinaryOperator>(v);

  if (truncV)
    bo = dyn_cast<BinaryOperator>(truncV);
  else
    bo = dyn_cast<BinaryOperator>(v);

  if (!bo)
    return NULL;

  if (bo->getOpcode() != Instruction::Add)
    return NULL;

  Value *op0 = bo->getOperand(0);
  Value *op1 = bo->getOperand(1);
  if (isa<SExtInst>(op0)) {
    SExtInst *sextInst = cast<SExtInst>(op0);
    op0 = sextInst->getOperand(0);
  }
  if (isa<SExtInst>(op1)) {
    SExtInst *sextInst = cast<SExtInst>(op1);
    op1 = sextInst->getOperand(0);
  }

  PHINode *phi = NULL;
  Value *stepValue = NULL;

  if (isa<PHINode>(op0)) {
    phi = cast<PHINode>(op0);
    stepValue = op1;
  } else if (isa<PHINode>(op1)) {
    phi = cast<PHINode>(op1);
    stepValue = op0;
  } else {
    return NULL;
  }

  IndexExpr *start = NULL;

  if (phi->getIncomingValue(0) == v) {
    start = buildExpr(phi->getIncomingValue(1));
  } else if (phi->getIncomingValue(1) == v) {
    start = buildExpr(phi->getIncomingValue(0));
  } else {
    return NULL;
  }

  IndexExpr *step = buildExpr(stepValue);
  IndexExpr *hb = buildExpr(icmp->getOperand(1));

  // for (i=start; i<higherbound; i += step)
  // nbiter = (higherbound - start + step - 1) / step
  // backedgecount = (higherbound - start + step - 1) / step - 1
  IndexExpr *hb_minus_start = new IndexExprBinop(IndexExprBinop::Sub,
						 new IndexExprHB(hb),
						 start);
  IndexExpr *step_minus_one = new IndexExprBinop(IndexExprBinop::Sub,
						 new IndexExprHB(step),
						 new IndexExprConst(1));
  IndexExpr *add = new IndexExprBinop(IndexExprBinop::Add,
				      hb_minus_start,
				      step_minus_one);
  IndexExpr *div = new IndexExprBinop(IndexExprBinop::Div,
				      add,
				      new IndexExprHB(step->clone()));
  IndexExpr *backedgeCount = new IndexExprBinop(IndexExprBinop::Sub,
						div,
						new IndexExprConst(1));

  return backedgeCount;
}

IndexExpr *
IndexExprBuilder::tryComputeLoopStep(Loop *L) {
  BasicBlock *exitingBlock = L->getExitingBlock();
  if (!exitingBlock)
    return NULL;

  TerminatorInst *TI = exitingBlock->getTerminator();
  BranchInst *BI = dyn_cast<BranchInst>(TI);
  if (!BI)
    return NULL;

  if (BI->isUnconditional())
    return NULL;

  if (BI->getNumSuccessors() != 2 ||
      L->contains(BI->getSuccessor(1)) ||
      !L->contains(BI->getSuccessor(0)))
    return NULL;

  Value *condition = BI->getCondition();

  ICmpInst *icmp = dyn_cast<ICmpInst>(condition);
  if (!icmp)
    return NULL;

  if (icmp->getSignedPredicate() != CmpInst::ICMP_SLT)
    return NULL;

  Value *v = icmp->getOperand(0);
  Value *truncV = NULL;
  if (isa<TruncInst>(v)) {
    TruncInst *trInst = cast<TruncInst>(v);
    truncV = trInst->getOperand(0);
  }

  BinaryOperator *bo = dyn_cast<BinaryOperator>(v);

  if (truncV)
    bo = dyn_cast<BinaryOperator>(truncV);
  else
    bo = dyn_cast<BinaryOperator>(v);

  if (!bo)
    return NULL;

  if (bo->getOpcode() != Instruction::Add)
    return NULL;

  Value *op0 = bo->getOperand(0);
  Value *op1 = bo->getOperand(1);
  if (isa<SExtInst>(op0)) {
    SExtInst *sextInst = cast<SExtInst>(op0);
    op0 = sextInst->getOperand(0);
  }
  if (isa<SExtInst>(op1)) {
    SExtInst *sextInst = cast<SExtInst>(op1);
    op1 = sextInst->getOperand(0);
  }

  PHINode *phi = NULL;
  Value *stepValue = NULL;

  if (isa<PHINode>(op0)) {
    phi = cast<PHINode>(op0);
    stepValue = op1;
  } else if (isa<PHINode>(op1)) {
    phi = cast<PHINode>(op1);
    stepValue = op0;
  } else {
    return NULL;
  }

  if (phi->getIncomingValue(0) == v) {
  } else if (phi->getIncomingValue(1) == v) {
  } else {
    return NULL;
  }

  IndexExpr *step = buildExpr(stepValue);
  return step;
}

IndexExpr *
IndexExprBuilder::tryComputeLoopStart(Loop *L) {
  BasicBlock *exitingBlock = L->getExitingBlock();
  if (!exitingBlock)
    return NULL;

  TerminatorInst *TI = exitingBlock->getTerminator();
  BranchInst *BI = dyn_cast<BranchInst>(TI);
  if (!BI)
    return NULL;

  if (BI->isUnconditional())
    return NULL;

  if (BI->getNumSuccessors() != 2 ||
      L->contains(BI->getSuccessor(1)) ||
      !L->contains(BI->getSuccessor(0)))
    return NULL;

  Value *condition = BI->getCondition();

  ICmpInst *icmp = dyn_cast<ICmpInst>(condition);
  if (!icmp)
    return NULL;

  if (icmp->getSignedPredicate() != CmpInst::ICMP_SLT)
    return NULL;

  Value *v = icmp->getOperand(0);
  Value *truncV = NULL;
  if (isa<TruncInst>(v)) {
    TruncInst *trInst = cast<TruncInst>(v);
    truncV = trInst->getOperand(0);
  }

  BinaryOperator *bo = dyn_cast<BinaryOperator>(v);

  if (truncV)
    bo = dyn_cast<BinaryOperator>(truncV);
  else
    bo = dyn_cast<BinaryOperator>(v);

  if (!bo)
    return NULL;

  if (bo->getOpcode() != Instruction::Add)
    return NULL;

  Value *op0 = bo->getOperand(0);
  Value *op1 = bo->getOperand(1);
  if (isa<SExtInst>(op0)) {
    SExtInst *sextInst = cast<SExtInst>(op0);
    op0 = sextInst->getOperand(0);
  }
  if (isa<SExtInst>(op1)) {
    SExtInst *sextInst = cast<SExtInst>(op1);
    op1 = sextInst->getOperand(0);
  }

  PHINode *phi = NULL;

  if (isa<PHINode>(op0)) {
    phi = cast<PHINode>(op0);
  } else if (isa<PHINode>(op1)) {
    phi = cast<PHINode>(op1);
  } else {
    return NULL;
  }

  IndexExpr *start = NULL;

  if (phi->getIncomingValue(0) == v) {
    start = buildExpr(phi->getIncomingValue(1));
  } else if (phi->getIncomingValue(1) == v) {
    start = buildExpr(phi->getIncomingValue(0));
  } else {
    return NULL;
  }

  return start;
}

void
IndexExprBuilder::parseSCEV(const llvm::SCEV *scev, IndexExpr **indexExpr,
			    const llvm::Argument **arg) {
  unsigned type = scev->getSCEVType();

  switch(type) {
  case scConstant:
    {
      const SCEVConstant *scConst = cast<SCEVConstant>(scev);
      *indexExpr = buildExpr(scConst->getValue());
      return;
    }

  case scUnknown:
    {
      const SCEVUnknown *scUnknown = cast<SCEVUnknown>(scev);

      const Argument *argument = dyn_cast<Argument>(scUnknown->getValue());
      if (argument &&
	  (isGlobalArgument(argument) || isConstantArgument(argument))) {
	assert(*arg == NULL || *arg == argument);
	*arg = argument;
	*indexExpr = new IndexExprConst(0);
	return;

      }

      *indexExpr =  buildExpr(scUnknown->getValue());

      // When computing the number of loop backedge taken for an SCEV with
      // OpenCL ID (e.g. get_global_id, get_group_id) we wants the higher bound
      // of the ID interval.
      //
      // example :
      // int y = get_group_id(1) * get_local_size(0) + get_local_id(1) + 1;
      // int step = get_local_size(1);
      // int end = (get_group_id(1) + 1) * get_local_size(0) + 1;
      // for (int yy = y; yy < end; yy += step)
      // array[yy] = ...

      if (computingBackedge)
	*indexExpr = new IndexExprHB(*indexExpr);

      return;
    }

  case scTruncate:
  case scZeroExtend:
  case scSignExtend:
    {
      const SCEVCastExpr *scCastExpr = cast<SCEVCastExpr>(scev);
      parseSCEV(scCastExpr->getOperand(), indexExpr, arg);
      return;
    }

  case scAddExpr:
  case scMulExpr:
    {
      const SCEVNAryExpr *scNExpr = cast<SCEVNAryExpr>(scev);

      unsigned op;

      switch(type) {
      case scAddExpr:
	op = IndexExprBinop::Add;
	break;
      default:
	op = IndexExprBinop::Mul;
      };

      size_t numOperands = scNExpr->getNumOperands();

      IndexExpr **opExprs = new IndexExpr *[numOperands];

      for (size_t i=0; i<numOperands; i++)
	parseSCEV(scNExpr->getOperand(i), &opExprs[i], arg);

      IndexExpr *ret = opExprs[numOperands-1];
      for (unsigned i = numOperands-2; i >= 1; i--)
	ret = new IndexExprBinop(op, opExprs[i], ret);

      *indexExpr = new IndexExprBinop(op, opExprs[0], ret);
      delete[] opExprs;
      return;
    }

  case scUMaxExpr:
  case scSMaxExpr:
    {
      const SCEVNAryExpr *scNExpr = cast<SCEVNAryExpr>(scev);
      size_t numOperands = scNExpr->getNumOperands();
      IndexExpr **opExprs = new IndexExpr *[numOperands];
      for (size_t i=0; i<numOperands; i++)
	parseSCEV(scNExpr->getOperand(i), &opExprs[i], arg);
      *indexExpr =  new IndexExprMax(numOperands, opExprs);
      delete[] opExprs;
      return;
    }
  case scUDivExpr:
    {
      const SCEVUDivExpr *scDivExpr = cast<SCEVUDivExpr>(scev);

      IndexExpr *LHS, *RHS;
      parseSCEV(scDivExpr->getLHS(), &LHS, arg);
      parseSCEV(scDivExpr->getRHS(), &RHS, arg);
      *indexExpr =  new IndexExprBinop(IndexExprBinop::Div,
				       LHS, RHS);
      return;
    }

  case scAddRecExpr:
    {
      const SCEVAddRecExpr *scAddRecExpr = cast<SCEVAddRecExpr>(scev);
      IndexExpr *start, *step, *backedgeCount;
      parseSCEV(scAddRecExpr->getStart(), &start, arg);
      parseSCEV(scAddRecExpr->getStepRecurrence(*scalarEvolution), &step, arg);
      const Loop *L = scAddRecExpr->getLoop();
      const SCEV *scevBackedgeCount = scalarEvolution->getBackedgeTakenCount(L);

      if (scevBackedgeCount->getSCEVType() != scCouldNotCompute) {
	computingBackedge = true;
      	parseSCEV(scevBackedgeCount, &backedgeCount, arg);
	computingBackedge = false;
      } else {
      	// Fallback, not sure if it works in all cases.
	backedgeCount = tryComputeLoopBackedCount(const_cast<Loop *>(L));
	if (!backedgeCount)
	  backedgeCount = new IndexExprUnknown("loop");
      }

      IndexExpr *end
	= new IndexExprBinop(IndexExprBinop::Add,
			     new IndexExprHB(start->clone()),
			     new IndexExprBinop(IndexExprBinop::Mul,
						new IndexExprHB(step), new IndexExprLB(backedgeCount)));
      *indexExpr = new IndexExprInterval(start, end);
#define DEBUG_TYPE "loop"
      DEBUG(
	    errs() << "LOOP EXPR: ";
	    (*indexExpr)->dump();
	    errs() << "\n";);
#undef DEBUG_TYPE
      return;
    }

  default:
    *indexExpr = new IndexExprUnknown("unknown scev");
    return;
  };
}

unsigned
IndexExprBuilder::getNumIndirections() {
  return numIndirections;
}

const LoadIndirectionExpr *
IndexExprBuilder::getIndirection(unsigned n) {
  assert(n < indirections.size());
  return indirections[n];
}

void
IndexExprBuilder::disableIndirections() {
  indirectionsDisabled = true;
}

void
IndexExprBuilder::enableIndirections() {
  indirectionsDisabled = false;
}
