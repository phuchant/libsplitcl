LLVM_CONFIG_PATH := ~/mytools/llvm-3.6/bin/llvm-config
CLANG_INCLUDE_PATH := /home/phuchant/mytools/llvm-3.6/include
CLANG_LIB_PATH := /home/phuchant/mytools/llvm-3.6/lib
$(info -----------------------------------------------)
$(info Using LLVM_CONFIG_PATH = $(LLVM_CONFIG_PATH))
$(info Using CLANG_INCLUDE_PATH = $(CLANG_INCLUDE_PATH))
$(info Using CLANG_LIB_PATH = $(CLANG_LIB_PATH))
$(info -----------------------------------------------)

CXX := g++-4.9
CXXFLAGS := -fno-rtti -O0 -g -std=c++11
PLUGIN_CXXFLAGS := -fpic

LLVM_CXXFLAGS := `$(LLVM_CONFIG_PATH) --cxxflags`
LLVM_LDFLAGS := `$(LLVM_CONFIG_PATH) --ldflags --libs --system-libs`
LLVM_LDFLAGS_NOLIBS := `$(LLVM_CONFIG_PATH) --ldflags`
PLUGIN_LDFLAGS := -shared

CLANG_LIBS := \
	-Wl,--start-group \
	-lclangAST \
	-lclangAnalysis \
	-lclangBasic \
	-lclangDriver \
	-lclangEdit \
	-lclangFrontend \
	-lclangLex \
	-lclangParse \
	-lclangSema \
	-lclangRewrite \
	-lclangSerialization \
	-Wl,--end-group

SRC := src
BIN := bin

all: make_bindir check_deps $(BIN)/cltransform

$(BIN)/cltransform: $(SRC)/ClTransform.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) -I$(CLANG_INCLUDE_PATH) $^ \
		-L$(CLANG_LIB_PATH) $(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

.PHONY: make_bindir check_deps clean

make_bindir:
	@test -d $(BIN) || mkdir $(BIN)

check_deps:
	@test -e $(LLVM_CONFIG_PATH) || echo Error: $(LLVM_CONFIG_PATH) \
	does not exist
	@test -d $(CLANG_INCLUDE_PATH) || echo Error: \
	$(CLANG_INCLUDE_PATH) does not exist
	@test -d $(CLANG_LIB_PATH) || echo Error: $(CLANG_LIB_PATH) \
	does not exist

clean:
	rm -rf $(BIN) *~ $(SRC)/*~ $(SRC)/*.o
