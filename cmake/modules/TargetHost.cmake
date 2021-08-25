# TargetHost.cmake
# Build programs that must be run on this host

include(ExternalProject)
add_subdirectory(platform/sim/sim)

# Run the superbuild-style rebuild of this project, with the different compiler
# suite
message(INFO "Project dir is ${PROJECT_SOURCE_DIR}")
ExternalProject_Add(llvm_mos_sdk_mos
    SOURCE_DIR ${PROJECT_SOURCE_DIR}
    CMAKE_ARGS -DLLVM_MOS_COMPILE_TARGET_MOS=On ${CMAKE_ARGS}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mos
    BUILD_COMMAND ${CMAKE_COMMAND} --build .
    USES_TERMINAL_CONFIGURE Yes 
    USES_TERMINAL_BUILD Yes 
    INSTALL_COMMAND ""
    )