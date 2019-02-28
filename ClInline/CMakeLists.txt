project(ClInline)

# Specify bin path
set(EXECUTABLE_OUTPUT_PATH bin/)

add_executable(clinline src/ClInline.cpp)

target_include_directories(clinline PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_compile_definitions(clinline PRIVATE ${LLVM_DEFINITIONS} ${CLANG_DEFINITIONS})
target_compile_options(clinline PRIVATE -fno-rtti)

# Link against clang tooling libraries
target_link_libraries(clinline PRIVATE clangTooling)
