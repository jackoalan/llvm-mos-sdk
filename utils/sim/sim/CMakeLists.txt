add_executable(sim fake6502.c sim.c)
if(WIN32)
    target_compile_definitions(sim PUBLIC -D_CRT_SECURE_NO_WARNINGS)
endif()
