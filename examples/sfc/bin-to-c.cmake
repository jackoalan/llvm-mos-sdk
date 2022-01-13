get_filename_component(base_filename ${INPUT_FILE} NAME)
string(MAKE_C_IDENTIFIER ${base_filename} c_name)
file(READ ${INPUT_FILE} content HEX)

# Separate into individual bytes.
string(REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" SEPARATED_HEX ${content})

set(output_c)

set(counter 0)
foreach(hex IN LISTS SEPARATED_HEX)
  string(APPEND output_c "0x${hex},")
  math(EXPR counter "${counter}+1")
  if(counter GREATER 16)
    string(APPEND output_c "\n  ")
    set(counter 0)
  endif()
endforeach()

set(output_c "#include <stdint.h>
uint8_t ${c_name}_data[] = {
  ${output_c}
}\;
unsigned ${c_name}_size = sizeof(${c_name}_data)\;
")

file(WRITE ${OUTPUT_FILE} ${output_c})
