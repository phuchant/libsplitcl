CXX=g++-4.9

include ../Makefile.inc

SRC_DIR := src
LDFLAGS += $(shell $(LLVM_CONFIG) --ldflags) $(shell $(LLVM_CONFIG) --libs) $(shell $(LLVM_CONFIG) --system-libs) -Wl,-rpath,$(KERNELEXPR_LIB_PATH) \
	-L$(KERNELEXPR_LIB_PATH) -lkernelexpr
COMMON_FLAGS=-Wall -Wextra -gdwarf-3
CXXFLAGS+=$(COMMON_FLAGS) $(shell $(LLVM_CONFIG) --cxxflags) -std=c++11
CPPFLAGS+=$(shell $(LLVM_CONFIG) --cppflags) -Iinclude \
	-I$(KERNELEXPR_INCLUDE_PATH) #-DDEBUG

ifeq ($shell uname),Darwin)
LOADABLE_MODULE_OPTIONS=-bundle -undefined dynamic_lookup
else
LOADABLE_MODULE_OPTIONS=-shared -Wl,-O0
endif

KERNELANALYSISPASS=lib/klanalysis.so
KERNELANALYSISPASS_OBJECTS= \
	$(SRC_DIR)/AnalysisPass.o \
	$(SRC_DIR)/ConditionBuilder.o \
	$(SRC_DIR)/IndexExprBuilder.o \
	$(SRC_DIR)/Utils.o


default: make_lib_dir check_deps $(KERNELANALYSISPASS)


%.o : $(SRC_DIR)/%.cpp
	@echo Compiling $*.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(KERNELANALYSISPASS) : $(KERNELANALYSISPASS_OBJECTS)
	@echo Linking $@
	$(CXX) -o $@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^
.PHONY: make_lib_dir check_deps clean

make_lib_dir:
	@test -d lib || mkdir lib

check_deps:
	@test -e $(LLVM_CONFIG) || echo Error: $(LLVM_CONFIG) does not exist
	@test -d $(KERNELEXPR_LIB_PATH) || echo Error: $(KERNELEXPR_LIB_PATH) does \
	not exist
	@test -d $(KERNELEXPR_INCLUDE_PATH) || echo Error: \
	$(KERNELEXPR_INCLUDE_PATH) does not exist

clean::
	@rm -f $(KERNELANALYSISPASS) $(KERNELANALYSISPASS_OBJECTS) *~ include/*~ src/*~
