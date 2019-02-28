project(ClTransform)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)

add_executable(cltransform src/ClTransform.cpp)

target_include_directories(cltransform PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_compile_definitions(cltransform PRIVATE ${LLVM_DEFINITIONS} ${CLANG_DEFINITIONS})
target_compile_options(cltransform PRIVATE -fno-rtti)

# Link against clang tooling libraries
target_link_libraries(cltransform clangTooling)
