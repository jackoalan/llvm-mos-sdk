llvm_mos_sdk_add_object_file(crt0 crt0.S)
llvm_mos_sdk_add_crt(sim_exit.c)
llvm_mos_sdk_add_c(
  assert.c
  putchar.c
  stdlib.c
)
llvm_mos_sdk_add_m()
