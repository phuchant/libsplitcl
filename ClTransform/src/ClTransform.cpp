#include <cstdio>
#include <memory>
#include <string>
#include <sstream>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

#define NUMGROUPSVAR "__libsplit_num_groups_"
#define SPLITDIMVAR  "__libsplit_split_dim_"

using namespace clang;

std::set<FunctionDecl *> functionsSet;
std::set<FunctionDecl *> functionsSet2;

// Second visitor :
// add parameters numgroups and splitdim to stored non kernel functions

class SecondVisitor : public RecursiveASTVisitor<SecondVisitor> {
public:
  SecondVisitor(Rewriter &R, CompilerInstance &ci, SourceManager &sm)
    : TheRewriter(R), ci(ci), sm(sm) {}

  bool VisitStmt(Stmt *s) {
    // Only care about CallExpr statements
    CallExpr *call = dyn_cast<CallExpr>(s);
    if (!call)
      return true;

    FunctionDecl *func = call->getDirectCallee();
    if (!func)
      return true;

    std::set<FunctionDecl*>::iterator it = functionsSet.find(func);
    std::set<FunctionDecl*>::iterator it2 = functionsSet2.find(func);
    if (it == functionsSet.end() && it2 == functionsSet2.end())
      return true;

    // If the function is in the set, add the two new parameters.

    SourceLocation start = call->getLocStart();
    SourceLocation end = call->getLocEnd();

    CharSourceRange argSourceRange =
      CharSourceRange::getTokenRange(start,
				     end);
    StringRef strArg = Lexer::getSourceText(argSourceRange, sm,
					    ci.getLangOpts());
    std::string str = strArg.str();
    std::string str2 = str.substr(0, str.length()-1);
    if (call->getNumArgs() > 0) {
      str2.append(std::string(std::string(",") + NUMGROUPSVAR +
			      "," + SPLITDIMVAR + ")"));
    } else {
      str2.append(std::string(std::string(" ") + NUMGROUPSVAR +
			      "," + SPLITDIMVAR + ")"));
    }

    TheRewriter.ReplaceText(SourceRange(start, end), str2);

    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *f) {
    std::set<FunctionDecl*>::iterator it = functionsSet.find(f);
    if (it == functionsSet.end())
      return true;

    // Add parameters
    TypeLoc TL = f->getTypeSourceInfo()->getTypeLoc();
    FunctionTypeLoc FTL = TL.getAs<FunctionTypeLoc>();

    if (f->param_size() == 0) { // kernel with no parameter
      std::string str(std::string("(const int ") + NUMGROUPSVAR +
    		      ", const int " + SPLITDIMVAR + ")");
      TheRewriter.ReplaceText(FTL.getLocalSourceRange(), str);
    } else {
      std::string str(std::string(", const int ") + NUMGROUPSVAR + 
    		      ", const int " + SPLITDIMVAR);
      TheRewriter.InsertText(FTL.getLocalRangeEnd(), str);
    }

    return true;
  }

private:
  Rewriter &TheRewriter;
  CompilerInstance &ci;
  SourceManager &sm;
};

// First visitor :
// transform call to get_group_id, get_num_groups and get_global_size
// store non kernel functions using one of these calls
// add parameters numgroups and splitdim to kernels

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R, CompilerInstance &ci, SourceManager &sm)
    : TheRewriter(R), ci(ci), sm(sm) {}

  FunctionDecl *currentFunction;

  // Transform each call to get_group_id, get_num_groups, get_global_size
  // Save all non-kernel functions using one of those call in functionsSet.

  bool VisitStmt(Stmt *s) {
    // Only care about CallExpr statements
    CallExpr *call = dyn_cast<CallExpr>(s);
    if (!call)
      return true;

    FunctionDecl *func = call->getDirectCallee();
    if (!func)
      return true;

    // Get function name
    std::string funcname = func->getNameInfo().getAsString();
    llvm::errs() << "funcname : " << funcname << "\n";

    // get_group_id(workDim) -> get_globalid(workDim) / get_local_id(workDim)
    if (funcname.compare("get_group_id") == 0) {
      if (!currentFunction->hasAttr<OpenCLKernelAttr>())
	functionsSet.insert(currentFunction);
      else
	functionsSet2.insert(currentFunction);

      // Get work dim argument as a string
      Expr *arg = call->getArg(0);
      CharSourceRange argSourceRange =
	CharSourceRange::getTokenRange(arg->getLocStart(),
				       arg->getLocEnd());
      StringRef strArg = Lexer::getSourceText(argSourceRange, sm,
					      ci.getLangOpts());

      // Transformation

      // get_group_id(arg) -> ((arg) == SPLITDIMVAR ?
      // (get_global_id(SPLITDIMVAR) / get_local_size(SPLITDIMVAR))
      // : get_group_id(arg))
      std::string str = "((" + std::string(strArg) + std::string(") == ") +
	SPLITDIMVAR + "? (get_global_id(" + SPLITDIMVAR +
	") / get_local_size(" + SPLITDIMVAR + ")) : get_group_id(" +
	std::string(strArg) + "))";

      TheRewriter.ReplaceText(SourceRange(call->getLocStart(),
					  call->getLocEnd()),
			      str);
    }

    // get_num_groups(workDim) -> NEWVARNAME
    if (!funcname.compare("get_num_groups")) {
      if (!currentFunction->hasAttr<OpenCLKernelAttr>())
	functionsSet.insert(currentFunction);
      else
	functionsSet2.insert(currentFunction);

      // Get work dim argument as a string
      Expr *arg = call->getArg(0);
      CharSourceRange argSourceRange =
	CharSourceRange::getTokenRange(arg->getLocStart(),
				       arg->getLocEnd());
      StringRef strArg = Lexer::getSourceText(argSourceRange, sm,
					      ci.getLangOpts());

      // Transformation

      // get_num_groups(arg) -> ((arg) == SPLITDIMVAR ?
      // NUMGROUPSVAR : get_num_groups(arg))
      std::string str = "((" + std::string(strArg) + std::string(") == ") +
	SPLITDIMVAR + "? (" + NUMGROUPSVAR + ") : get_num_groups(" +
	std::string(strArg) + "))";

      TheRewriter.ReplaceText(SourceRange(call->getLocStart(),
					  call->getLocEnd()),
			      str);
    }

    // get_global_size(workDim) -> NEWVARNAME * get_local_size(workDim)
    if (!funcname.compare("get_global_size")) {
      if (!currentFunction->hasAttr<OpenCLKernelAttr>())
	functionsSet.insert(currentFunction);
      else
	functionsSet2.insert(currentFunction);

      // Get work dim argument as a string
      Expr *arg = call->getArg(0);
      CharSourceRange argSourceRange =
	CharSourceRange::getTokenRange(arg->getLocStart(),
				       arg->getLocEnd());
      StringRef strArg = Lexer::getSourceText(argSourceRange, sm,
					      ci.getLangOpts());

      // Transformation

      // get_global_size(arg) -> ((arg) == SPLITDIMVAR ?
      // NUMGROUPSVAR : get_local_size(arg))
      std::string str = "((" + std::string(strArg) + std::string(") == ") +
	SPLITDIMVAR + "? (" + NUMGROUPSVAR + "* get_local_size(" +
	std::string(strArg) + ")) : get_global_size(" +
	std::string(strArg) + "))";

      TheRewriter.ReplaceText(SourceRange(call->getLocStart(),
					  call->getLocEnd()),
			      str);
    }

    return true;
  }

  // Add parameters splitdim and numgroups to each kernel.

  bool VisitFunctionDecl(FunctionDecl *f) {
    currentFunction = f;

    // Check if it is an OpenCL kernel
    if (!f->hasAttr<OpenCLKernelAttr>())
      return true;

    llvm::errs() << f->getNameInfo().getAsString() << " is a kernel.\n";

    // Add parameter
    TypeLoc TL = f->getTypeSourceInfo()->getTypeLoc();
    FunctionTypeLoc FTL = TL.getAs<FunctionTypeLoc>();

    if (f->param_size() == 0) { // kernel with no parameter
      std::string str(std::string("(const int ") + NUMGROUPSVAR +
		      ", const int " + SPLITDIMVAR + ")");
      TheRewriter.ReplaceText(FTL.getLocalSourceRange(), str);
    } else {
      std::string str(std::string(", const int ") + NUMGROUPSVAR + 
		      ", const int " + SPLITDIMVAR);
      TheRewriter.InsertText(FTL.getLocalRangeEnd(), str);
    }

    functionsSet2.insert(f);

    return true;
  }

private:
  Rewriter &TheRewriter;
  CompilerInstance &ci;
  SourceManager &sm;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R, CompilerInstance &ci,
		SourceManager &sm) : firstPass(R, ci, sm),
				     secondPass(R, ci, sm) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(DeclGroupRef DR) {

    // First visitor :
    // transform call to get_group_id, get_num_groups and get_global_size
    // store non kernel functions using one of these calls
    // add parameters numgroups and splitdim to kernels
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
      firstPass.TraverseDecl(*b);

    // Second visitor :
    // add parameters numgroups and splitdim to stored non kernel functions
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
      secondPass.TraverseDecl(*b);
    return true;
  }

private:
  MyASTVisitor firstPass;
  SecondVisitor secondPass;
};


int main(int argc, char *argv[]) {
  if (argc < 2) {
    llvm::errs() << "Usage: cltranform <options> <filename>\n";
    return 1;
  }

  // CompilerInstance will hold the instance of the Clang compiler for us,
  // managing the various objects needed to run the compiler.
  CompilerInstance TheCompInst;
  TheCompInst.createDiagnostics();

  CompilerInvocation *Invocation = new CompilerInvocation();
  CompilerInvocation::CreateFromArgs(*Invocation, argv + 1, argv + argc,
				     TheCompInst.getDiagnostics());
  TheCompInst.setInvocation(Invocation);

  LangOptions &lo = TheCompInst.getLangOpts();
  lo.OpenCL = 1;
  lo.C99 = 1;
  lo.OpenCLVersion = 120;

  // Initialize target info with the default triple for our platform.
  auto TO = std::make_shared<TargetOptions>();
  TO->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI =
      TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
  TheCompInst.setTarget(TI);

  TheCompInst.createFileManager();
  FileManager &FileMgr = TheCompInst.getFileManager();
  TheCompInst.createSourceManager(FileMgr);
  SourceManager &SourceMgr = TheCompInst.getSourceManager();
  TheCompInst.createPreprocessor(TU_Module);
  TheCompInst.createASTContext();

  // A Rewriter helps us manage the code rewriting task.
  Rewriter TheRewriter;
  TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

  // Set the main file handled by the source manager to the input file.
  const FileEntry *FileIn = FileMgr.getFile(argv[argc-1]);
  SourceMgr.setMainFileID(
      SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
  TheCompInst.getDiagnosticClient().BeginSourceFile(
      TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());

  // Create an AST consumer instance which is going to get called by
  // ParseAST.
  MyASTConsumer TheConsumer(TheRewriter, TheCompInst, SourceMgr);

  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
           TheCompInst.getASTContext());

  // At this point the rewriter's buffer should be full with the rewritten
  // file contents.
  const RewriteBuffer *RewriteBuf =
      TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());

  if (RewriteBuf)
    llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());
  else
    llvm::outs() << std::string(TheRewriter.getEditBuffer(SourceMgr.getMainFileID()).begin(),
				TheRewriter.getEditBuffer(SourceMgr.getMainFileID()).end());

  return 0;
}
