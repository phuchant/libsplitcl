project(libsplit)

# Generate the source files list
file (GLOB_RECURSE source_files src/*)

add_library(libsplit SHARED ${source_files})

# Include header files
target_include_directories(libsplit PRIVATE src)
target_link_libraries(libsplit LibKernelExpr pthread OpenCL gsl glpk)

target_compile_definitions(libsplit PRIVATE
  CLANGPATH="${LLVM_TOOLS_BINARY_DIR}/clang"
  CLTRANSFORMPATH="$<TARGET_FILE:cltransform>"
  CLINLINEPATH="$<TARGET_FILE:clinline>"
  LLVM_LIB_DIR="${LLVM_LIBRARY_DIR}"
  KERNELANALYSIS_SO_PATH="$<TARGET_FILE:klanalysis>"
  CLANGVERSION="${LLVM_VERSION}"
  CLANGMAJOR=${LLVM_VERSION_MAJOR}
  OPTPATH="${LLVM_TOOLS_BINARY_DIR}/opt")
