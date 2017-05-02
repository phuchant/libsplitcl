include Makefile.inc

CXX := g++-4.9
CXXFLAGS := -fno-rtti -O0 -g
PLUGIN_CXXFLAGS := -fpic

LLVM_CXXFLAGS := `$(LLVM_CONFIG) --cxxflags`
LLVM_LDFLAGS := `$(LLVM_CONFIG) --ldflags --libs --system-libs`
LLVM_LDFLAGS_NOLIBS := `$(LLVM_CONFIG) --ldflags`
PLUGIN_LDFLAGS := -shared

CLANG_LIBS := \
	-Wl,--start-group \
	-lclangAST \
	-lclangASTMatchers \
	-lclangAnalysis \
	-lclangBasic \
	-lclangDriver \
	-lclangEdit \
	-lclangFrontend \
	-lclangFrontendTool \
	-lclangLex \
	-lclangParse \
	-lclangSema \
	-lclangEdit \
	-lclangRewrite \
	-lclangRewriteFrontend \
	-lclangStaticAnalyzerFrontend \
	-lclangStaticAnalyzerCheckers \
	-lclangStaticAnalyzerCore \
	-lclangSerialization \
	-lclangToolingCore \
	-lclangTooling \
	-lclangFormat \
	-Wl,--end-group

BIN := bin
SRC := src

.PHONY: all
all: make_builddir \
	$(BIN)/clinline

.PHONY: make_builddir
make_builddir:
	@test -d $(BIN) || mkdir $(BIN)

$(BIN)/clinline: $(SRC)/ClInline.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@


.PHONY: clean
clean:
	rm -rf $(BIN) *~ $(SRC)/*~ $(SRC)/*.o

