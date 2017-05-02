#include "Utils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"

#include <map>
#include <sstream>

using namespace llvm;
using namespace std;

bool isKernel(const Function *func) {
  return func->getMetadata("kernel_arg_addr_space");
}

unsigned isOpenCLCall(const Instruction *inst) {
  const CallInst *callInst = dyn_cast<CallInst>(inst);

  if (!callInst)
    return UNKNOWN;

  StringRef funcName = callInst->getCalledFunction()->getName();

  if (funcName.find("get_global_id") != StringRef::npos)
    return GET_GLOBAL_ID;
  if (funcName.find("get_local_id") != StringRef::npos)
    return GET_LOCAL_ID;
  if (funcName.find("get_global_size") != StringRef::npos)
    return GET_GLOBAL_SIZE;
  if (funcName.find("get_local_size") != StringRef::npos)
    return GET_LOCAL_SIZE;
  if (funcName.find("get_group_id") != StringRef::npos)
    return GET_GROUP_ID;
  if (funcName.find("get_num_groups") != StringRef::npos)
    return GET_NUM_GROUPS;

  return UNKNOWN;
}

vector<string> splitString(string str) {
  stringstream ss(str);
  string buf;
  vector<string> tokens;

  while (ss >> buf)
    tokens.push_back(buf);

  return tokens;
}

void getGlobalArguments(TinyPtrVector<Argument *> &v, Function &F) {
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end();
       I != E; ++I) {
    Argument &arg = *I;

    if (isGlobalArgument(&arg))
      v.push_back(&arg);
  }
}

void getConstantArguments(TinyPtrVector<Argument *> &v, Function &F) {
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end();
       I != E; ++I) {
    Argument &arg = *I;

    if (isConstantArgument(&arg))
      v.push_back(&arg);
  }
}

bool isGlobalArgument(const Argument *A) {
  MDNode *mdNode = A->getParent()->getMetadata("kernel_arg_addr_space");
  if (!mdNode)
    return false;

  unsigned pos = A->getArgNo();

  assert(pos < mdNode->getNumOperands());

  ValueAsMetadata *VAM = dyn_cast<ValueAsMetadata>(mdNode->getOperand(pos).get());
  assert(VAM);
  Value *addrspaceValue = VAM->getValue();
  ConstantInt *addrspaceCstInt = dyn_cast<ConstantInt>(addrspaceValue);
  assert(addrspaceCstInt);

  return addrspaceCstInt->getSExtValue() == 1;
}

bool isConstantArgument(const Argument *A) {
  MDNode *mdNode = A->getParent()->getMetadata("kernel_arg_addr_space");
  if (!mdNode)
    return false;

  unsigned pos = A->getArgNo();

  assert(pos < mdNode->getNumOperands());

  ValueAsMetadata *VAM = dyn_cast<ValueAsMetadata>(mdNode->getOperand(pos).get());
  assert(VAM);
  Value *addrspaceValue = VAM->getValue();
  ConstantInt *addrspaceCstInt = dyn_cast<ConstantInt>(addrspaceValue);
  assert(addrspaceCstInt);

  return addrspaceCstInt->getSExtValue() == 2;
}

void getGetNumGroupsCalls(llvm::TinyPtrVector<llvm::CallInst *> &v,
			  llvm::Function &F) {
  // Iterate over the instructions
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;
    CallInst *callInst = dyn_cast<CallInst>(inst);

    if (!callInst)
      continue;

    // Check if it is a call to get_num_groups()
    unsigned oclFunc = isOpenCLCall(callInst);
    if (oclFunc == GET_NUM_GROUPS)
      v.push_back(callInst);
  }
}
void getGetGroupIDCalls(llvm::TinyPtrVector<llvm::CallInst *> &v,
			llvm::Function &F) {
  // Iterate over the instructions
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;
    CallInst *callInst = dyn_cast<CallInst>(inst);

    if (!callInst)
      continue;

    // Check if it is a call to get_group_id()
    unsigned oclFunc = isOpenCLCall(callInst);
    if (oclFunc == GET_GROUP_ID)
      v.push_back(callInst);
  }
}

void getGetGlobalSizeCalls(llvm::TinyPtrVector<llvm::CallInst *> &v,
			   llvm::Function &F) {
  // Iterate over the instructions
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;
    CallInst *callInst = dyn_cast<CallInst>(inst);

    if (!callInst)
      continue;

    // Check if it is a call to get_group_id()
    unsigned oclFunc = isOpenCLCall(callInst);
    if (oclFunc == GET_GLOBAL_SIZE)
      v.push_back(callInst);
  }
}


bool
isForLoopA(const PHINode *phi, Value **lowerBound, Value **higherBound) {
  if(phi->getNumOperands() != 2)
    return false;

  *lowerBound = phi->getIncomingValue(0);
  Value *inc = phi->getIncomingValue(1);
  ICmpInst *cmp = dyn_cast<ICmpInst>(*phi->getIncomingValue(1)->use_begin());
  if (!cmp)
    return false;

  if (inc != cmp->getOperand(0))
    return false;

  *higherBound = cmp->getOperand(1);

  if (cmp->getSignedPredicate() != CmpInst::ICMP_SLT)
    return false;

  return true;
}

bool
isForLoopB(const PHINode *phi, Value **lowerBound, Value **higherBound) {
  if(phi->getNumOperands() != 2)
    return false;

  *lowerBound = phi->getIncomingValue(1);
  Value *inc = phi->getIncomingValue(0);
  ICmpInst *cmp = dyn_cast<ICmpInst>(*phi->getIncomingValue(0)->use_begin());
  if (!cmp)
    return false;

  if (inc != cmp->getOperand(0))
    return false;

  *higherBound = cmp->getOperand(1);

  if (cmp->getSignedPredicate() != CmpInst::ICMP_SLT)
    return false;

  return true;
}

// Hack
bool isForLoop(const PHINode *phi, Value **lowerBound, Value **higherBound) {
  if (isForLoopA(phi, lowerBound, higherBound))
    return true;

  return isForLoopB(phi, lowerBound, higherBound);
}


bool
isWrite(const Value *value) {
  for (Value::const_user_iterator I = value->user_begin(),
	 E = value->user_end(); I != E; ++I) {
    if (isa<StoreInst>(*I))
      return true;

    const User *user = *I;

    if (const CallInst *callInst = dyn_cast<CallInst>(user)) {
      StringRef funcName = callInst->getCalledFunction()->getName();
      if (funcName.find("atomic_") ||
	  funcName.find("atom_"))
	return true;
    }
  }

  return false;
}

bool
hasGlobalBarrier(llvm::Function &F) {
  // Iterate over instructions
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    Instruction *inst = &*I;

    // Find call instructions
    if (CallInst *call = dyn_cast<CallInst>(inst)) {
      Function *callee = call->getCalledFunction();

      StringRef funcName = callee->getName();

      // Barrier
      if (funcName.find("_Z7barrierj") != StringRef::npos) {

	if (call->getNumOperands() != 2)
	  continue;

	if (!isa<ConstantInt>(call->getOperand(0)))
	  continue;

	ConstantInt *argValue = cast<ConstantInt>(call->getOperand(0));

	if (argValue->getSExtValue() == 2) {

	  return true;
	}
      }
    }
  }

  return false;
}

bool
hasAtomic(llvm::Function &F) {
  // Iterate over instructions
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    Instruction *inst = &*I;

    // Find call instructions
    if (CallInst *call = dyn_cast<CallInst>(inst)) {
      Function *callee = call->getCalledFunction();

      StringRef funcName = callee->getName();

      // Atomic inst
      if (funcName.find("atomic_") != StringRef::npos ||
	  funcName.find("atom_") != StringRef::npos) {

	return true;
      }
    }
  }

  return false;
}

