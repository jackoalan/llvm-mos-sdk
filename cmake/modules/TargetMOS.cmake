# TargetMOS.cmake
# CMake pass for targeting MOS.

# Since this is the MOS build, and we've found our own compiler, don't run 
# the command line tests on CMake startup
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
include(Platform/llvm-mos)
include(BootstrapCompiler)
include(DetectCompiler)
detect_compiler(_result)
if (NOT _result)
    message(FATAL_ERROR "failed to discover LLVM-MOS compiler")
endif()
include(Platform/llvm-mos)
