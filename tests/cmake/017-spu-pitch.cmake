# Pitch vectors always run. The optional unchanged LIBSD.IRX oracle runs its
# low linked addresses in the real guest block, without code/data relocation.
add_executable(spu_pitch_test spu_pitch_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/sound/spu_pitch.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c"
  "${CMAKE_SOURCE_DIR}/src/platform/guest.c"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c"
  "${P4_OS_SRC}")
target_include_directories(spu_pitch_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/sound")
target_compile_definitions(spu_pitch_test PRIVATE P4_MIPSDIFF_RAM_LO=0x1000u)
target_link_libraries(spu_pitch_test m)
p4_ps2_exe(spu_pitch_test)
add_test(NAME spu_pitch COMMAND spu_pitch_test)
add_test(NAME spu_pitch_retail COMMAND spu_pitch_test --require-retail)
set_tests_properties(spu_pitch_retail PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
add_test(NAME spu_pitch_coefficients COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/spu_pitch_table.py"
  --out "${CMAKE_SOURCE_DIR}/src/platform/sound/spu_pitch_table.h" --check)
