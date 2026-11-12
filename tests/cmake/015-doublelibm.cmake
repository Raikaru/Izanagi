# Actual ELF instructions are the oracle, including the complete soft-double
# callee closure. This fixture never hooks arithmetic or substitutes host libm.
add_executable(ee_dlibm_test ee_dlibm_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/ee_dlibm.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_dfloat.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ps2_fpbit.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c"
  "${CMAKE_SOURCE_DIR}/src/platform/guest.c"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c"
  "${P4_OS_SRC}")
target_include_directories(ee_dlibm_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform")
target_link_libraries(ee_dlibm_test m)
p4_ps2_exe(ee_dlibm_test)
add_test(NAME ee_dlibm_retail COMMAND ee_dlibm_test)
set_tests_properties(ee_dlibm_retail PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
