# Test the same host-only retained buffer module used by the runtime. No ELF
# section-GC assumption: the i386 target must also link on Windows/COFF.
add_executable(lockstep_draw_test lockstep_draw_test.c ../src/platform/lockstep_draw.c)
add_test(NAME lockstep_draw_buffer COMMAND lockstep_draw_test)
