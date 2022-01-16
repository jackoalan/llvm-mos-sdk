if(NOT MOS)
  add_executable(brr-enc brr-enc.c)
else()
  add_custom_command(OUTPUT jarrea.brr
    COMMAND ${LLVM_MOS_BRR_ENC_COMMAND}
    ${CMAKE_CURRENT_SOURCE_DIR}/jarrea.wav ${CMAKE_CURRENT_BINARY_DIR}/jarrea.brr
    DEPENDS jarrea.wav)

  add_custom_command(OUTPUT jarrea.c
    COMMAND ${CMAKE_COMMAND}
    -DINPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/jarrea.brr -DOUTPUT_FILE=jarrea.c
    -P ${CMAKE_SOURCE_DIR}/cmake/bin-to-c.cmake
    DEPENDS jarrea.brr ${CMAKE_SOURCE_DIR}/cmake/bin-to-c.cmake)

  set(CMAKE_EXECUTABLE_SUFFIX .elf)
endif()

llvm_mos_sdk_add_example_executable(hello-spc700 hello-spc700.cpp jarrea.c)
