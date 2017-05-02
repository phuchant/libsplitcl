#ifndef UTILS_H
#define UTILS_H

#include <IndexExpr/IndexExprOCL.h>

#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

bool isKernel(const llvm::Function *func);

unsigned isOpenCLCall(const llvm::Instruction *inst);

std::vector<std::string> splitString(std::string str);

bool isGlobalArgument(const llvm::Argument *A);
bool isConstantArgument(const llvm::Argument *A);

void getGlobalArguments(llvm::TinyPtrVector<llvm::Argument *> &v,
			llvm::Function &F);
void getConstantArguments(llvm::TinyPtrVector<llvm::Argument *> &v,
			llvm::Function &F);
void getGetNumGroupsCalls(llvm::TinyPtrVector<llvm::CallInst *> &v,
			  llvm::Function &F);
void getGetGroupIDCalls(llvm::TinyPtrVector<llvm::CallInst *> &v,
			llvm::Function &F);
void getGetGlobalSizeCalls(llvm::TinyPtrVector<llvm::CallInst *> &v,
			llvm::Function &F);

bool isForLoop(const llvm::PHINode *phi,
	       llvm::Value **lowerBound,
	       llvm::Value **higherBound);

bool isWrite(const llvm::Value *value);

bool hasGlobalBarrier(llvm::Function &F);
bool hasAtomic(llvm::Function &F);

#endif /* UTILS_H */
