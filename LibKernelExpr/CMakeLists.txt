project(LibKernelExpr)

# Generate the source files list
file(GLOB_RECURSE source_files src/*)
file(GLOB_RECURSE HEADERS include/*.h)

# Create shared library
add_library(LibKernelExpr SHARED ${source_files} ${HEADER})

# Include header files
target_include_directories(LibKernelExpr PUBLIC include)

# -fPIC
set_property(TARGET LibKernelExpr PROPERTY POSITION_INDEPENDENT_CODE ON)
