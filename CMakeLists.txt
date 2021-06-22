cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# add_subdirectory(common/ldscripts)
include(DetectCompiler)

detect_compiler(_result)
if (NOT _result)
    message(FATAL_ERROR "need to find own compiler")
endif()

project(llvm-mos-sdk LANGUAGES C CXX ASM)


