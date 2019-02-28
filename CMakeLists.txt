cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0054 NEW)

# c++11
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(MyProject)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(LLVM "3.9.1" EXACT REQUIRED  CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(Clang REQUIRED)
find_package(Clang REQUIRED clangTooling libClang)

find_package(GLPK REQUIRED)

find_package(GSL REQUIRED)

find_package(OpenCL REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_compile_options(-Wall -Wextra)

add_subdirectory(ClInline)
add_subdirectory(ClTransform)
add_subdirectory(LibKernelExpr)
add_subdirectory(AnalysisPass)
add_subdirectory(libsplit)

