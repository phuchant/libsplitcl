project(AnalysisPass)

# Generate the source files list
file (GLOB_RECURSE source_files src/*)

add_library(klanalysis MODULE ${source_files})

# Include header files
target_include_directories(klanalysis
  PRIVATE include ${LLVM_INCLUDE_DIRS})

target_compile_definitions(klanalysis PRIVATE ${LLVM_DEFINITIONS})
target_compile_options(klanalysis PRIVATE -fno-rtti)
target_link_libraries(klanalysis LibKernelExpr)
