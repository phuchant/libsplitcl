#include "IndexExprBuilder.h"
#include "Utils.h"

#include "IndexExpr/IndexExprs.h"

#include "llvm/Analysis/ScalarEvolutionExpressions.h"

using namespace llvm;
using namespace std;

IndexExprBuilder::IndexExprBuilder(llvm::LoopInfo *loopInfo,
				   llvm::ScalarEvolution *scalarEvolution,
				   llvm::DataLayout *dataLayout,
				   llvm::Type *syclRangeType)
 : loopInfo(loopInfo),
   scalarEvolution(scalarEvolution),
   dataLayout(dataLayout),
   syclRangeType(syclRangeType) {}

IndexExprBuilder::~IndexExprBuilder() {}

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
  (void) CI;
  (void) expr;
  (void) arg;

  errs() << "atom_* not handled yet !\n";
  assert(false);
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

    errs() << "buildexpr unknown constant : " << *user << "\n";

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
	if (oclFunc != UNKNOWN)
	  return new IndexExprOCL(oclFunc, buildExpr(user->getOperand(0)));

	errs() << "buildexpr unknown function : " << *inst << "\n";
	return new IndexExprUnknown("unknown function");
      }

    case Instruction::PHI:
      {
	// PHINode *phi = cast<PHINode>(inst);
	// errs() << "phi = " << phi << "\n";
	// errs() << "phi = " << *phi << "\n";

	// // Try to compute phi with SCEV
	// const SCEV *scev = scalarEvolution->getSCEV(phi);
	// errs() << "scev : " << *scev << "\n";
	// IndexExpr *ret = NULL;
	// const Argument *arg = NULL;
	// errs() << "avant parse\n";
	// parseSCEV(scev, &ret, &arg);
	// errs() << "apres parse\n";

	// if (arg != NULL)
	  return new IndexExprUnknown("PHI");
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

    case Instruction::Load:
      {
	if (!syclRangeType)
	  return new IndexExprUnknown("load");

	LoadInst *load = cast<LoadInst>(user);

	// If its a sycl range buildExpr, otherwise return unknown instr.

	if (load->getPointerAddressSpace() != 0) {
	  return new IndexExprUnknown("load");
	}


	BitCastInst *bitcast = dyn_cast<BitCastInst>(load->getPointerOperand());

	if (!bitcast)
	  return new IndexExprUnknown("load");

	PointerType *ptrTy = dyn_cast<PointerType>(bitcast->getSrcTy());

	if (!ptrTy)
	  return new IndexExprUnknown("load");


	if (ptrTy->getElementType() != syclRangeType)
	  return new IndexExprUnknown("load");


	if (!isa<Argument>(bitcast->getOperand(0)))
	  return new IndexExprUnknown("load");

	return buildExpr(bitcast->getOperand(0));
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
  BinaryOperator *bo = dyn_cast<BinaryOperator>(v);
  if (!bo)
    return NULL;

  if (bo->getOpcode() != Instruction::Add)
    return NULL;

  Value *op0 = bo->getOperand(0);
  Value *op1 = bo->getOperand(1);

  PHINode *phi = dyn_cast<PHINode>(op0);
  if (!phi)
    return NULL;

  if (phi->getIncomingValue(0) != bo)
    return NULL;

  IndexExpr *start = buildExpr(phi->getIncomingValue(1));
  IndexExpr *step = buildExpr(op1);
  IndexExpr *hb = buildExpr(icmp->getOperand(1));

  // nbiter = (higherbound - start + step - 1) / step
  // backedgecount = (higherbound - start + step - 1) / step - 1
  IndexExpr *hb_minus_start = new IndexExprBinop(IndexExprBinop::Sub,
						 hb,
						 start);
  IndexExpr *step_minus_one = new IndexExprBinop(IndexExprBinop::Sub,
						 step,
						 new IndexExprConst(1));
  IndexExpr *add = new IndexExprBinop(IndexExprBinop::Add,
				      hb_minus_start,
				      step_minus_one);
  IndexExpr *div = new IndexExprBinop(IndexExprBinop::Div,
				      add,
				      step->clone());
  IndexExpr *backedgeCount = new IndexExprBinop(IndexExprBinop::Sub,
						div,
						new IndexExprConst(1));
  return backedgeCount;
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

      IndexExpr *opExprs[numOperands];

      for (size_t i=0; i<numOperands; i++)
	parseSCEV(scNExpr->getOperand(i), &opExprs[i], arg);

      IndexExpr *ret = opExprs[numOperands-1];
      for (unsigned i = numOperands-2; i >= 1; i--)
	ret = new IndexExprBinop(op, opExprs[i], ret);

      *indexExpr = new IndexExprBinop(op, opExprs[0], ret);
      return;
    }

  case scUMaxExpr:
  case scSMaxExpr:
    {
      // Not handled yet !
      *indexExpr = new IndexExprUnknown("MAX");
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
      	parseSCEV(scevBackedgeCount, &backedgeCount, arg);
      } else {
      	// Fallback, not sure if it works in all cases.
	backedgeCount = tryComputeLoopBackedCount(const_cast<Loop *>(L));
	if (!backedgeCount)
	  backedgeCount = new IndexExprUnknown("loop");
      }

      IndexExpr *end
	= new IndexExprBinop(IndexExprBinop::Add,
			     start->clone(),
			     new IndexExprBinop(IndexExprBinop::Mul,
						step, backedgeCount));
      *indexExpr = new IndexExprInterval(start, end);
      return;
    }

  default:
    *indexExpr = new IndexExprUnknown("unknown scev");
    return;
  };
}
