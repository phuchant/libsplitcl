include Makefile.inc

CXX=g++-4.9
CXXFLAGS = -std=c++11 -shared -fPIC -Wall -Wextra -O3 -gdwarf-3 -Isrc/ \
	-I$(KERNELEXPR_INCLUDE_PATH) \
	-DCLANGPATH=\"$(CLANGPATH)\" -DOPTPATH=\"$(OPTPATH)\" \
	-I$(MKGR_INCLUDE_PATH) \
	-DCLTRANSFORMPATH=\"$(CLTRANSFORMPATH)\" \
	-DCLINLINEPATH=\"$(CLINLINEPATH)\" \
	-DLLVM_LIB_DIR=\"$(LLVMLIBPATH)\" \
	-DKERNELANALYSIS_SO_PATH=\"$(KERNELANALYSIS_SO_PATH)\"

LDFLAGS  = -ldl \
	-Wl,-rpath,$(KERNELEXPR_LIB_PATH) -L$(KERNELEXPR_LIB_PATH) -lkernelexpr \
	-lm -lpthread \
	-L$(GLPK_LIB_PATH) -lglpk \
	-Wl,--whole-archive $(MKGR_LIB_PATH)/libmkgr.a -Wl,--no-whole-archive \
	-lgsl

ifeq ($(USE_HWLOC),1)
	CXXFLAGS += -DUSE_HWLOC=1
	LDFLAGS += -lhwloc
endif

ifeq ($(CUDA),1)
	CXXFLAGS += -DCUDA=1
	LDFLAGS += -lcudart
endif

SRC	 = src/Dispatch/OpenCLFunctions.cpp \
	src/Dispatch/Platform.cpp \
	src/Dispatch/CommandQueue.cpp \
	src/Dispatch/Device.cpp \
	src/Dispatch/Context.cpp \
	src/Dispatch/MemoryObject.cpp \
	src/Dispatch/Sampler.cpp \
	src/Dispatch/Program.cpp \
	src/Dispatch/Kernel.cpp \
	src/Dispatch/Event.cpp \
	src/Dispatch/Enqueue.cpp \
	src/Handle/ContextHandle.cpp \
	src/Handle/KernelHandle.cpp \
	src/Handle/MemoryHandle.cpp \
	src/Handle/ProgramHandle.cpp \
	src/Queue/Command.cpp \
	src/Queue/DeviceQueue.cpp \
	src/Queue/DeviceLFQueue.cpp \
	src/Queue/DevicePthreadQueue.cpp \
	src/Queue/LockFreeQueue.cpp \
	src/Scheduler/Scheduler.cpp \
	src/Scheduler/SchedulerBadBroyden.cpp \
	src/Scheduler/SchedulerBroyden.cpp \
	src/Scheduler/SchedulerEnv.cpp \
	src/Scheduler/SchedulerFixedPoint.cpp \
	src/Scheduler/SchedulerMKGR.cpp \
	src/Utils/Algebra.cpp \
	src/Utils/Debug.cpp \
	src/Utils/Logger.cpp \
	src/Utils/Utils.cpp \
	src/BufferManager.cpp \
	src/Driver.cpp \
	src/Init.cpp \
	src/Options.cpp \
	src/Globals.cpp

OBJ	 = $(SRC:.cpp=.o)

TARGET	 = lib/libhookocl.so

all: make_lib_dir check_deps $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ)

.PHONY: make_lib_dir check_deps clean

make_lib_dir:
	@test -d lib || mkdir lib


clean:
	rm -f $(OBJ) $(TARGET) *~ src/*~ src/Dispatch/*~ src/Handle/*~ src/Queue/*~ src/Scheduler/*~
