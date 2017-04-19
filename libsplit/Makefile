CXX=g++-4.9

CLANGPATH=~/mytools/llvm-3.2/bin/clang
OPTPATH=~/mytools/llvm-3.2/bin/opt

CLTRANSFORMPATH=$(PWD)/../ClTransform/bin/cltransform
CLINLINEPATH=$(PWD)/../ClInline/bin/clinline

SPIRTRANSFORMPATH=$(PWD)/../SpirTransform/lib/spirtransform.so

KERNELANALYSIS_SO_PATH=$(PWD)/../AnalysisPass/lib/klanalysis.so

KERNELEXPR_INCLUDE_PATH=$(PWD)/../LibKernelExpr/include
KERNELEXPR_LIB_PATH = $(PWD)/../LibKernelExpr/lib

MERGEKERNELS_TYPE=char
MERGEKERNELS_PATH=$(PWD)

SKGR_INCLUDE_PATH=/home/phuchant/mytools/akgr-1.0/include
# SKGR_LIB_PATH=/home/phuchant/mytools/akgr-1.0/lib

MKGR_INCLUDE_PATH=/home/phuchant/mytools/mkgr-1.2/include
MKGR_LIB_PATH=/home/phuchant/mytools/mkgr-1.2/lib

GLPK_LIB_PATH=/home/counilh/install_soft/glpk-4.55_install/lib/

SPIRHEADER= $(PWD)/include/opencl_spir.h

CXXFLAGS = -std=c++11 -shared -fPIC -Wall -Wextra -I$(KERNELEXPR_INCLUDE_PATH) \
	-Iinclude -DCLANGPATH=\"$(CLANGPATH)\" -DOPTPATH=\"$(OPTPATH)\" \
	-I$(SKGR_INCLUDE_PATH) -I$(MKGR_INCLUDE_PATH) \
	-DCLTRANSFORMPATH=\"$(CLTRANSFORMPATH)\" \
	-DCLINLINEPATH=\"$(CLINLINEPATH)\" \
	-DSPIRTRANSFORMPATH=\"$(SPIRTRANSFORMPATH)\" \
	-DKERNELANALYSIS_SO_PATH=\"$(KERNELANALYSIS_SO_PATH)\" \
	-DMERGEKERNELS_PATH=\"$(MERGEKERNELS_PATH)\" \
	-DMERGEKERNELS_TYPE=$(MERGEKERNELS_TYPE) \
	-DSPIRHEADER=\"$(SPIRHEADER)\" \
	-DCL_USE_DEPRECATED_OPENCL_1_1_APIS \
	-fopenmp \
	-O3 -gdwarf-3 #-DTIMESTATS #-DSTATS #-DPRINTPERF

LDFLAGS  = -ldl -Wl,-rpath,$(KERNELEXPR_LIB_PATH) -L$(KERNELEXPR_LIB_PATH) \
	-lkernelexpr -lm -lgsl \
	-fopenmp \
	-L$(GLPK_LIB_PATH) -lglpk \
	-Wl,--whole-archive $(MKGR_LIB_PATH)/libmkgr.a -Wl,--no-whole-archive
	# -Wl,--whole-archive $(SKGR_LIB_PATH)/libakgr.a -Wl,--no-whole-archive \

SRC	 =  \
	src/Algebra.cpp \
	src/Buffer.cpp \
	src/BufferManagerSimple.cpp \
	src/BufferManagerOptim.cpp \
	src/CommandQueue.cpp \
	src/Context.cpp \
	src/ContextHandle.cpp \
	src/Debug.cpp \
	src/DeviceInfo.cpp \
	src/Event.cpp \
	src/Globals.cpp \
	src/Kernel.cpp \
	src/KernelHandle.cpp \
	src/Logger.cpp \
	src/MemoryHandle.cpp \
	src/MemoryHandleOptim.cpp \
	src/OpenCLFunctions.cpp \
	src/Options.cpp \
	src/Platform.cpp \
	src/Program.cpp \
	src/ProgramHandle.cpp \
	src/Sampler.cpp \
	src/Scheduler.cpp \
	src/SchedulerAutoAbs.cpp \
	src/SchedulerAKGR.cpp \
	src/SchedulerMKGR.cpp \
	src/SchedulerBadBroyden.cpp \
	src/SchedulerBroyden.cpp \
	src/SchedulerFixPoint.cpp \
	src/SchedulerEnv.cpp \
	src/Utils.cpp

OBJ	 = $(SRC:.cpp=.o)

TARGET	 = lib/libhookocl.so

all: make_lib_dir check_deps $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ)

.PHONY: make_lib_dir check_deps clean

make_lib_dir:
	@test -d lib || mkdir lib

check_deps:
	@test -e $(CLANGPATH) || echo Error: $(CLANGPATH) does not exist
	@test -e $(OPTPATH) || echo Error: $(OPTPATH) does not exist
	@test -e $(CLTRANSFORMPATH) || echo Error: $(CLTRANSFORMPATH) \
	does not exist
	@test -e $(KERNELANALYSIS_SO_PATH) || echo Error: \
	$(KERNELANALYSIS_SO_PATH) does not exist
	@test -d $(KERNELEXPR_INCLUDE_PATH) || echo Error: \
	$(KERNELEXPR_INCLUDE_PATH) does not exist
	@test -d $(KERNELEXPR_LIB_PATH) || echo Error: $(KERNELEXPR_LIB_PATH) \
	does not exist

clean:
	rm -f $(OBJ) $(TARGET) *~ src/*~ include/*~
