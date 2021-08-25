# llvm-mos.cmake
#
# When CMake is configured, CMake tends to set compilation and linker strings
# based on targeting the local host.  On Windows especially, though not 
# uniquely Windows, this tends to add in a bunch of linker flags and default
# system libraries that are not compatible with LLVM-MOS.
#
# CMake is not well set up to target multiple targets in a single compile. 
# However, LLVM-MOS run a few programs on the local host, such as the 6502 
# simulator, even though it also compiles and runs code for a MOS target.
# We work around this conflict by setting sensible compilation and link command
# lines in this file.  Then, we include this file exclusively in any CMakeFile
# that targets MOS.

include_guard(GLOBAL)

set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_LINKER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES> -o <TARGET>")
set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_LINKER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES> -o <TARGET>")

set(LLVM_MOS_OBJCOPY_FLAGS --output-target binary
    CACHE STRING "Flags for stripping executables with llvm-objcopy")
