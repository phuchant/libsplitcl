#include "ConditionBuilder.h"

#include "llvm/ADT/SetVector.h"

using namespace llvm;
using namespace std;

static bool
isReachableRec(const BasicBlock *from, const BasicBlock *to,
	       SetVector<const BasicBlock *> &seenBlocks) {
  if (!seenBlocks.insert(from))
    return false;

  if (from == to)
    return true;

  bool ret = false;

  for (succ_const_iterator I = succ_begin(from), E = succ_end(from);
       I != E; ++I) {
    if (seenBlocks.count(*I) == 0)
      ret = ret || isReachableRec(*I, to, seenBlocks);
  }

  return ret;
}

static bool
isReachable(const BasicBlock *from, const BasicBlock *to) {
  SetVector<const BasicBlock *> seenBlocks;

  return isReachableRec(from, to, seenBlocks);
}

static map<BasicBlock *, set<BasicBlock *> *> pdfCache;

// PDF computation
static vector<BasicBlock * >
postdominance_frontier(PostDominatorTree &PDT, BasicBlock *BB) {
  vector<BasicBlock * > PDF;

  set<BasicBlock *> *cache = pdfCache[BB];
  if (cache) {
    for (BasicBlock *b : *cache)
      PDF.push_back(b);

    return PDF;
  }

  PDF.clear();
  DomTreeNode *DomNode = PDT.getNode(BB);

  for (auto it = pred_begin(BB), et = pred_end(BB); it != et; ++it){
    // does BB immediately dominate this predecessor?
    DomTreeNode *ID = PDT[*it]; //->getIDom();
    if(ID && ID->getIDom()!=DomNode && *it!=BB){
      PDF.push_back(*it);
    }
  }
  for (DomTreeNode::const_iterator NI = DomNode->begin(), NE = DomNode->end();
       NI != NE; ++NI) {
    DomTreeNode *IDominee = *NI;
    vector<BasicBlock * > ChildDF =
      postdominance_frontier(PDT, IDominee->getBlock());
    vector<BasicBlock * >::const_iterator CDFI
      = ChildDF.begin(), CDFE = ChildDF.end();
    for (; CDFI != CDFE; ++CDFI) {
      if (PDT[*CDFI]->getIDom() != DomNode && *CDFI!=BB){
	PDF.push_back(*CDFI);
      }
    }
  }

  pdfCache[BB] = new set<BasicBlock *>();
  pdfCache[BB]->insert(PDF.begin(), PDF.end());

  return PDF;
}

// PDF+ computation

static map<BasicBlock *, set<BasicBlock *> *> ipdfCache;

static vector<BasicBlock * >
iterated_postdominance_frontier(PostDominatorTree &PDT, BasicBlock *BB) {
  vector<BasicBlock *> iPDF;
  vector<BasicBlock *> PDF;

  set<BasicBlock *> *cache = ipdfCache[BB];
  if (cache) {
    for (BasicBlock *b : *cache)
      iPDF.push_back(b);
    return iPDF;
  }

  PDF=postdominance_frontier(PDT, BB);
  if(PDF.size()==0)
    return PDF;

  set<BasicBlock *> S;
  S.insert(PDF.begin(), PDF.end());

  set<BasicBlock *> toCompute;
  set<BasicBlock *> toComputeTmp;
  toCompute.insert(PDF.begin(), PDF.end());

  bool changed = true;
  while (changed) {
    changed = false;

    for (auto I = toCompute.begin(), E = toCompute.end(); I != E; ++I) {
      vector<BasicBlock *> tmp = postdominance_frontier(PDT, *I);
      for (auto J = tmp.begin(), F = tmp.end(); J != F; ++J) {
	if ((S.insert(*J)).second) {
	  toComputeTmp.insert(*J);
	  changed = true;
	}
      }
    }

    toCompute = toComputeTmp;
    toComputeTmp.clear();
  }

  iPDF.insert(iPDF.end(), S.begin(), S.end());

  ipdfCache[BB] = new set<BasicBlock *>();
  ipdfCache[BB]->insert(iPDF.begin(), iPDF.end());

  return iPDF;
}

static void
findConditionsRec(vector<const ICmpInst *> &conds,
		  vector<bool> &condsTruth,
		  const Value *v,
		  bool truth) {
  const BinaryOperator *bo = dyn_cast<BinaryOperator>(v);

  if (bo) {
    if (bo->getOpcode() == Instruction::Xor) {
      Value *op1 = bo->getOperand(1);
      Type *op1Ty = op1->getType();
      Value *op1True = ConstantInt::getTrue(op1Ty);
      if (op1 == op1True)
	truth = !truth;
    }
    findConditionsRec(conds, condsTruth, bo->getOperand(0), truth);
    findConditionsRec(conds, condsTruth, bo->getOperand(1), truth);
    return;
  }

  const ICmpInst *ci = dyn_cast<ICmpInst>(v);
  if (ci) {
    conds.push_back(ci);
    condsTruth.push_back(truth);
  }
}

void
ConditionBuilder::findConditions(BasicBlock *BB,
				 vector<const ICmpInst *> &conds,
				 vector<bool> &condsTruth) {
  // Get iterated post dominance frontier (conditions nodes).
  vector<BasicBlock *> IPDF = iterated_postdominance_frontier(PDT, BB);

  for (BasicBlock *condBlock : IPDF) {
    // Get conditional
    const TerminatorInst *ti = condBlock->getTerminator();
    const BranchInst *bi = dyn_cast<BranchInst>(ti);
    if (!bi || !bi->isConditional())
      continue;

    const Value *cond = bi->getCondition();
    assert(cond);

    // Get truth
    bool truth = isReachable(bi->getSuccessor(0), BB);

    // Get all ICmp conditions from conditional
    findConditionsRec(conds, condsTruth, cond, truth);
  }
}

ConditionBuilder::ConditionBuilder(PostDominatorTree &PDT,
				   IndexExprBuilder *indexExprBuilder)
  : PDT(PDT), indexExprBuilder(indexExprBuilder) {}

ConditionBuilder::~ConditionBuilder() {}

unsigned
ConditionBuilder::computeExprNumOclCalls(IndexExpr *expr) {
  switch(expr->getTag()) {
  case IndexExpr::OCL:
    {
      IndexExprOCL *oclExpr = static_cast<IndexExprOCL *>(expr);
      unsigned oclFunc = oclExpr->getOCLFunc();
      if (oclFunc == GET_GLOBAL_ID ||
	  oclFunc == GET_LOCAL_ID ||
	  oclFunc == GET_GROUP_ID)
	return 1;
      return 0;
    }

  case IndexExpr::BINOP:
    {
      IndexExprBinop *binop = static_cast<IndexExprBinop *>(expr);
      return computeExprNumOclCalls(binop->getExpr1()) +
	computeExprNumOclCalls(binop->getExpr2());
    }

  case IndexExpr::INTERVAL:
    {
      IndexExprInterval *interval = static_cast<IndexExprInterval *>(expr);
      return computeExprNumOclCalls(interval->getLb()) +
	computeExprNumOclCalls(interval->getHb());
    }

  case IndexExpr::MAX:
    {
      IndexExprMax *exprMax = static_cast<IndexExprMax *>(expr);
      unsigned n = 0;
      for (unsigned i=0; i<exprMax->getNumOperands(); i++)
	n += computeExprNumOclCalls(exprMax->getExprN(i));
      return n;
    }
  default:
    return 0;
  };
}

bool
ConditionBuilder::exprIsOCLId(IndexExpr *expr) {
  if (expr->getTag() != IndexExpr::OCL)
    return false;

  IndexExprOCL *oclExpr = static_cast<IndexExprOCL *>(expr);
  unsigned oclFunc = oclExpr->getOCLFunc();
  if (oclFunc == GET_GLOBAL_ID ||
      oclFunc == GET_LOCAL_ID ||
      oclFunc == GET_GROUP_ID)
    return true;
  return false;
}

bool
ConditionBuilder::normalizeOCLCondition(IndexExpr **exprId, IndexExpr **expr,
					bool *operandSwitched) {
  *operandSwitched = false;

  while(!exprIsOCLId(*exprId)) {
    if ((*exprId)->getTag() != IndexExpr::BINOP)
      return false;

    IndexExprBinop *binop = static_cast<IndexExprBinop *>(*exprId);
    switch(binop->getOp()) {
    case IndexExprBinop::Add:
      {
	if (computeExprNumOclCalls(binop->getExpr1())) {
	  *expr = new IndexExprBinop(IndexExprBinop::Sub,
				     *expr,
				     binop->getExpr2()->clone());
	  IndexExpr *tmp = *exprId;
	  *exprId = binop->getExpr1()->clone();
	  delete tmp;
	} else {
	  *expr = new IndexExprBinop(IndexExprBinop::Sub,
				     *expr,
				     binop->getExpr1()->clone());
	  IndexExpr *tmp = *exprId;
	  *exprId = binop->getExpr2()->clone();
	  delete tmp;
	}
	continue;
      }

    case IndexExprBinop::Sub:
      {
	if (computeExprNumOclCalls(binop->getExpr1())) {
	  *expr = new IndexExprBinop(IndexExprBinop::Add,
				     *expr,
				     binop->getExpr2()->clone());
	  IndexExpr *tmp = *exprId;
	  *exprId = binop->getExpr1()->clone();
	  delete tmp;
	} else {
	  IndexExpr *newExprId = binop->getExpr2()->clone();
	  IndexExpr *newOtherExpr =
	    new IndexExprBinop(IndexExprBinop::Sub,
			       binop->getExpr1()->clone(),
			       (*expr)->clone());
	  delete *exprId;
	  delete *expr;
	  *exprId = newExprId;
	  *expr = newOtherExpr;
	  *operandSwitched = !*operandSwitched;
	}
	continue;
      }

    case IndexExprBinop::Mul:
      {
	if (computeExprNumOclCalls(binop->getExpr1())) {
	  IndexExpr *tmp = *expr;
	  *expr = new IndexExprBinop(IndexExprBinop::Div,
				     (*expr)->clone(),
				     binop->getExpr2()->clone());
	  delete tmp;
	  *exprId = binop->getExpr1()->clone();
	  delete binop;
	} else {
	  IndexExpr *tmp = *expr;
	  *expr = new IndexExprBinop(IndexExprBinop::Div,
				     (*expr)->clone(),
				     binop->getExpr1()->clone());
	  delete tmp;
	  *exprId = binop->getExpr2()->clone();
	  delete binop;
	}
	continue;
      }

    case IndexExprBinop::Div:
      {
	if (computeExprNumOclCalls(binop->getExpr1())) {
	  IndexExpr *tmp = *expr;
	  *expr = new IndexExprBinop(IndexExprBinop::Mul,
				     (*expr)->clone(),
				     binop->getExpr2()->clone());
	  delete tmp;
	  *exprId = binop->getExpr1()->clone();
	  delete binop;
	} else {
	  IndexExpr *newExprId = binop->getExpr2()->clone();
	  IndexExpr *newOtherExpr
	    = new IndexExprBinop(IndexExprBinop::Div,
				 binop->getExpr1()->clone(),
				 (*expr)->clone());
	  delete binop;
	  delete *expr;
	  *exprId = newExprId;
	  *expr = newOtherExpr;
	  *operandSwitched = !*operandSwitched;
	}
	continue;
      }

    default:
      return false;
    }
  };

  return true;
}

void
ConditionBuilder::pushGuardExprLeft(std::vector<GuardExpr *> *guards,
				    IndexExprOCL *oclExpr, IndexExpr *expr,
				    const ICmpInst *cond, bool truth) {
  // Get work dim
  const IndexExpr *dimExpr = oclExpr->getArg();
  long dim;
  if (dimExpr->getTag() != IndexExpr::CONST ||
      !static_cast<const IndexExprConst *>(dimExpr)->getValue(&dim)) {
    delete oclExpr;
    delete expr;
    return;
  }

  unsigned pred;
  switch (cond->getSignedPredicate()) {
  case CmpInst::ICMP_SGT:
    pred = GuardExpr::GT;
    break;
  case CmpInst::ICMP_SGE:
    pred = GuardExpr::GE;
    break;
  case CmpInst::ICMP_SLE:
    pred = GuardExpr::LE;
    break;
  case CmpInst::ICMP_SLT:
    pred = GuardExpr::LT;
    break;
  case CmpInst::ICMP_EQ:
    pred = GuardExpr::EQ;
    break;
  case CmpInst::ICMP_NE:
    pred = GuardExpr::NEQ;
    break;
  default:
    delete oclExpr;
    delete expr;
    return;
  };

  guards->push_back(new GuardExpr(oclExpr->getOCLFunc(), (unsigned) dim,
				  pred, truth, expr));

  delete oclExpr;
}

void
ConditionBuilder::pushGuardExprRight(std::vector<GuardExpr *> *guards,
				     IndexExprOCL *oclExpr, IndexExpr *expr,
				     const ICmpInst *cond, bool truth) {
  // Get work dim
  const IndexExpr *dimExpr = oclExpr->getArg();
  long dim;
  if (dimExpr->getTag() != IndexExpr::CONST ||
      !static_cast<const IndexExprConst *>(dimExpr)->getValue(&dim)) {
    delete oclExpr;
    delete expr;
    return;
  }

  unsigned pred;
  switch (cond->getSignedPredicate()) {
  case CmpInst::ICMP_SGT:
    pred = GuardExpr::LT;
    break;
  case CmpInst::ICMP_SGE:
    pred = GuardExpr::LE;
    break;
  case CmpInst::ICMP_SLE:
    pred = GuardExpr::GE;
    break;
  case CmpInst::ICMP_SLT:
    pred = GuardExpr::GT;
    break;
  case CmpInst::ICMP_EQ:
    pred = GuardExpr::EQ;
    break;
  case CmpInst::ICMP_NE:
    pred = GuardExpr::NEQ;
    break;
  default:
    delete oclExpr;
    delete expr;
    return;
  };

  guards->push_back(new GuardExpr(oclExpr->getOCLFunc(), (unsigned) dim,
				  pred, truth, expr));

  delete oclExpr;
}

std::vector<GuardExpr *> *
ConditionBuilder::buildBasicBlockGuards(llvm::BasicBlock *BB) {
  indexExprBuilder->disableIndirections();


  // Get guards conditions and truths (i.e if the condition must be true or
  // false to execute expression).
  std::vector<const ICmpInst *> conds;
  std::vector<bool> condsTruth;
  findConditions(BB, conds, condsTruth);

  // Compute guards expressions
  std::vector<GuardExpr *> *guards =
    new std::vector<GuardExpr *>();

  for (unsigned i=0; i<conds.size(); ++i) {
    IndexExpr *e1 = indexExprBuilder->buildExpr(conds[i]->getOperand(0));
    IndexExpr *e2 = indexExprBuilder->buildExpr(conds[i]->getOperand(1));

    unsigned numIdsLHS = computeExprNumOclCalls(e1);
    unsigned numIdsRHS = computeExprNumOclCalls(e2);

    // We take into accound the guards where an OpenCL ID appears a single
    // time and only in one the the two operands of the conditions.
    // Example :
    // get_global_id(0) + 1 < N -> OK
    // N + 2 > get_global_id(0) + 1 -> OK
    // get_global_id(0) + get_global_id(1) < N -> NOK

    if (numIdsLHS + numIdsRHS != 1)
      continue;

    // Here the conditions is normalized in order to have only
    // the OpenCL ID to appear in one the two operands.
    // For example, the follwing condition :
    // get_global_id(0) + 1 < N
    // is normalized into :
    // get_global_id(0) < N - 1

    if (numIdsLHS == 1) {
      bool operandSwitched = false;
      if (normalizeOCLCondition(&e1, &e2, &operandSwitched)) {
	if (!operandSwitched) {
	  pushGuardExprLeft(guards,
			    static_cast<IndexExprOCL *>(e1), e2,
			    conds[i], condsTruth[i]);
	} else {
	  pushGuardExprRight(guards,
			     static_cast<IndexExprOCL *>(e1), e2,
			     conds[i], condsTruth[i]);
	}
      }
    } else {
      bool operandSwitched = false;
      if (normalizeOCLCondition(&e2, &e1, &operandSwitched)) {
	if (!operandSwitched) {
	  pushGuardExprRight(guards,
			     static_cast<IndexExprOCL *>(e2), e1,
			     conds[i], condsTruth[i]);
	} else {
	  pushGuardExprLeft(guards,
			    static_cast<IndexExprOCL *>(e2), e1,
			    conds[i], condsTruth[i]);
	}
      }
    }
  }

  indexExprBuilder->enableIndirections();
  return guards;
}
