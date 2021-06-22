# Detect details about the llvm-mos compiler suite.
function(detect_compiler ret)
    if (NOT EXISTS ${LLVM_MOS_SDK_LLVM_CONFIG})
        # Find the llvm-config program, either as part of this SDK or as part of 
        # a sibling install.
        find_program(
            LLVM_MOS_SDK_LLVM_CONFIG
            NAMES
                llvm-config
                llvm-config.exe
            PATH_SUFFIXES
                .
                bin
                build/bin
                llvm-mos/bin
                llvm-mos/build/bin
                llvm-mos/dist/bin
            HINTS
                ${LLVM_MOS}
                ${CMAKE_SOURCE_DIR}
                ${CMAKE_SOURCE_DIR}/../llvm-mos
        )
    endif()

    if (NOT EXISTS ${LLVM_MOS_SDK_LLVM_CONFIG})
        STRING(CONCAT desc
            "The llvm-config program could not be found.  This SDK uses llvm-config "
            "to find the location of the compiler.  Please download and install the "
            "llvm-mos distribution from https://github.com/llvm-mos/llvm-mos, and "
            "place it in your directory hierarchy as a sibling of the llvm-mos-sdk "
            "directory; or, download the llvm-mos toolchain and set the LLVM_MOS "
            "environment variable to the root of the toolchain."
        )
        MESSAGE(${desc})
        set(${ret} FALSE PARENT_SCOPE)
        return()
    else()
        MESSAGE("-- Found llvm-config at: ${LLVM_MOS_SDK_LLVM_CONFIG}")
    endif()

    if (NOT EXISTS ${LLVM_MOS_BIN_DIR})
    execute_process(
        COMMAND ${LLVM_MOS_SDK_LLVM_CONFIG} --bindir
        OUTPUT_VARIABLE LLVM_MOS_BIN_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        TIMEOUT 10
    )
    endif()

    if (NOT EXISTS ${LLVM_MOS_BIN_DIR})
        message("The LLVM_MOS binary directory reported by llvm-config at ${LLVM_MOS_BIN_DIR} does not exist.")
        set(${ret} FALSE PARENT_SCOPE)
        return()
    endif()

    MESSAGE("-- LLVM-MOS binary directory is ${LLVM_MOS_BIN_DIR}")

    execute_process(
        COMMAND ${LLVM_MOS_SDK_LLVM_CONFIG} --targets-built
        OUTPUT_VARIABLE _llvm_mos_targets
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        TIMEOUT 10
    )
    string(TOUPPER ${_llvm_mos_targets} _llvm_mos_targets)

    if (NOT ${_llvm_mos_targets} MATCHES "MOS")
        STRING(CONCAT desc
            "A working LLVM tools binary directory was found, but llvm-config reports that LLVM does not support the MOS target. Download a precompiled version of the LLVM-MOS tool chain from https://github.com/llvm-mos/llvm-mos, or compile a build of the tool chain yourself per those instructions, and set the LLVM_MOS environment variable to the root directory of those tools."
        )
        MESSAGE(${desc})
        set(${ret} FALSE PARENT_SCOPE)
        return()
    else()
        MESSAGE("-- llvm-config reports that MOS is a supported target")
    endif()

    find_program(
        _llvm_mos_c_compiler
        NAMES
            clang
            clang.exe
        PATHS
            ${LLVM_MOS_BIN_DIR}
        NO_DEFAULT_PATH
    )

    if(EXISTS ${_llvm_mos_c_compiler})
        if (NOT EXISTS ${CMAKE_C_COMPILER})
            # At this point CMAKE_C_COMPILER is not set and we have a good candidate.
            message("-- Setting \$\{CMAKE_C_COMPILER\} to ${_llvm_mos_c_compiler}")
            set(CMAKE_C_COMPILER ${_llvm_mos_c_compiler} 
                CACHE FILEPATH "Path to the LLVM-MOS C compiler")
        else()
            message("A candidate LLVM-MOS C compiler was found, but you've already set \$\{CMAKE_C_COMPILER\}, so we are disregarding the one at ${_llvm_mos_c_compiler}")
        endif()
    endif()

    find_program(
        _llvm_mos_cxx_compiler
        NAMES
            clang++
            clang++.exe
        PATHS
            ${LLVM_MOS_BIN_DIR}
        NO_DEFAULT_PATH
    )

    if(EXISTS ${_llvm_mos_cxx_compiler})
        if (NOT EXISTS ${CMAKE_CXX_COMPILER})
            # At this point CMAKE_C_COMPILER is not set and we have a good candidate.
            message(
                "-- Setting \$\{CMAKE_CXX_COMPILER\} to ${_llvm_mos_cxx_compiler}")
            set(CMAKE_CXX_COMPILER ${_llvm_mos_cxx_compiler} 
                CACHE FILEPATH "Path to the LLVM-MOS C++ compiler")
        else()
            message(
                "A candidate LLVM-MOS C++ compiler was found, but you've already set \$\{CMAKE_CXX_COMPILER\}, so we are disregarding the one at ${_llvm_mos_c_compiler}")
        endif()
    endif()

    set(${ret} TRUE PARENT_SCOPE)
    return()

endfunction()

