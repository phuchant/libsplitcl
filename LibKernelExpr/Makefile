CXX=g++-4.9
SRC_DIR := $(PWD)/src
LIB_DIR := $(PWD)/lib
COMMON_FLAGS=-Wall -Wextra -g -std=c++11
CPPFLAGS=-I$(PWD)/include
CXXFLAGS=$(COMMON_FLAGS) -O3 -fPIC

LDFLAGS	= -ldl -shared -fPIC
OBJ	= $(SRC_DIR)/ArgumentAnalysis.o \
	$(SRC_DIR)/IndexExpr/IndexExpr.o \
	$(SRC_DIR)/IndexExpr/IndexExprArg.o \
	$(SRC_DIR)/IndexExpr/IndexExprBinop.o \
	$(SRC_DIR)/IndexExpr/IndexExprConst.o \
	$(SRC_DIR)/IndexExpr/IndexExprInterval.o \
	$(SRC_DIR)/IndexExpr/IndexExprMax.o \
	$(SRC_DIR)/IndexExpr/IndexExprOCL.o \
	$(SRC_DIR)/IndexExpr/IndexExprUnknown.o \
	$(SRC_DIR)/IndexExpr/IndexExprValue.o \
	$(SRC_DIR)/Interval.o\
	$(SRC_DIR)/KernelAnalysis.o\
	$(SRC_DIR)/ListInterval.o\
	$(SRC_DIR)/NDRange.o \
	$(SRC_DIR)/ArgumentAnalysis.o \
	$(SRC_DIR)/GuardExpr.o \
	$(SRC_DIR)/WorkItemExpr.o

TARGET	= $(LIB_DIR)/libkernelexpr.so

all: make_lib_dir $(TARGET)

%.o : $(SRC_DIR)/%.cpp $(SRC_DIR)/IndexExpr/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(TARGET): $(OBJ)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^

.PHONY: clean make_lib_dir

make_lib_dir:
	@ test -d lib || mkdir lib

clean:
	rm -f $(OBJ) $(TARGET) *~ src/*~ src/IndexExpr/*~ include/*~ \
	include/IndexExpr/*~
