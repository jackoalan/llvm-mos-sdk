cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(LLVM_MOS_BOOTSTRAP_COMPILER On)

include(BootstrapCompiler)
# include(DetectCompiler)

# detect_compiler(_result)
# if (NOT _result)
#    message(FATAL_ERROR "need to find own compiler")
# endif()

project(llvm-mos-sdk 
    LANGUAGES C CXX ASM
    DESCRIPTION "Software development kit targeting common 8-bit computers with the MOS 65xx processor"
    HOMEPAGE_URL "https://github.com/llvm-mos/llvm-mos-sdk")

include(CMakePrintSystemInformation)

add_subdirectory(platform/sim/sim)


