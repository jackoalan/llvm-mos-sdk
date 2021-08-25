# TargetMOS.cmake
# CMake pass for targeting MOS.

include(BootstrapCompiler)
include(DetectCompiler)
detect_compiler(_result)
if (NOT _result)
    message(FATAL_ERROR "need to find own compiler")
endif()
