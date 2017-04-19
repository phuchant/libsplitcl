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
#include "clang/AST/Decl.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

#define NUMGROUPSVAR "__libsplit_num_groups_"
#define SPLITDIMVAR  "__libsplit_split_dim_"

using namespace clang;
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R, CompilerInstance &ci, SourceManager &sm)
    : TheRewriter(R), ci(ci), sm(sm) {}

  bool VisitFunctionDecl(FunctionDecl *f) {
    // Skip kernels
    if (f->hasAttr<OpenCLKernelAttr>())
      return true;

    // Handle case where return type is a macro
    SourceLocation loc = f->getSourceRange().getBegin();
    if (loc.isMacroID()) {
      std::pair<SourceLocation, SourceLocation> expansionRange =
	sm.getImmediateExpansionRange(loc);
      loc = expansionRange.first;
    }

    if (!f->isInlineSpecified())
      TheRewriter.InsertTextBefore(loc,
				   "__attribute__((always_inline)) inline ");
    else
      TheRewriter.InsertTextBefore(loc,
				   "__attribute__((always_inline)) ");

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
		SourceManager &sm) : Visitor(R, ci, sm) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
      // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);
    return true;
  }

private:
  MyASTVisitor Visitor;
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    llvm::errs() << "Usage: rewritersample <options> <filename>\n";
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
  lo.OpenCL = true;
  lo.C99 = true;

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
