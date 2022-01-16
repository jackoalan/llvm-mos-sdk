if(MOS)
  add_custom_command(OUTPUT spc-prg.bin
    COMMAND ${CMAKE_OBJCOPY} -O binary
    $<TARGET_FILE:spc700-hello-spc700> spc-prg.bin
    DEPENDS spc700-hello-spc700)

  add_custom_command(OUTPUT spc-prg.c
    COMMAND ${CMAKE_COMMAND}
    -DINPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/spc-prg.bin -DOUTPUT_FILE=spc-prg.c
    -P ${CMAKE_SOURCE_DIR}/cmake/bin-to-c.cmake
    DEPENDS spc-prg.bin ${CMAKE_SOURCE_DIR}/cmake/bin-to-c.cmake)

  file(GENERATE
    OUTPUT spc-elf.S
    CONTENT ".section .spc.elf
  .asciz \"$<TARGET_FILE:spc700-hello-spc700>\"
  ")
endif()

set(CMAKE_EXECUTABLE_SUFFIX .elf)
llvm_mos_sdk_add_example_executable(hello-sfc hello-sfc.cpp hello-sfc.S spc-elf.S spc-prg.c)
