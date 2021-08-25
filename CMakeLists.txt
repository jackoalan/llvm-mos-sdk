cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_VERBOSE_MAKEFILE ON)

option(LLVM_MOS_BOOTSTRAP_COMPILER "Should I download a working LLVM-MOS compiler for use with this SDK?" On)
option(LLVM_MOS_COMPILE_TARGET_MOS "Is this iteration of CMake intended to target programs for the local host, not 65xx machines?" OFF)

if(LLVM_MOS_COMPILE_TARGET_MOS)
    # Change compiler settings before asserting the project
    include(TargetMOS)
    project(llvm-mos-sdk-mos 
        LANGUAGES C CXX ASM
        DESCRIPTION "Software development kit targeting common 8-bit computers with the MOS 65xx processor"
        HOMEPAGE_URL "https://github.com/llvm-mos/llvm-mos-sdk")
else()
    project(llvm-mos-sdk-host
        LANGUAGES C CXX
        DESCRIPTION "Software development kit targeting common 8-bit computers with the MOS 65xx processor"
        HOMEPAGE_URL "https://github.com/llvm-mos/llvm-mos-sdk")
    include(TargetHost)
endif()

include(CMakePrintSystemInformation)




