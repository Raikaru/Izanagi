# Differential check of the native overrides against the retail code (docs/override_diff.md): an R5900 interpreter runs
# the retail words in the loaded image, the compiled native runs on the same guest memory, and the return value, the
# callee log and every guest-memory write are compared. Links the port objects like movie_smoke. POSIX only (mprotect
# write tracking, signals, fork): the Windows build skips it. ctest ovdiff_subset runs tests/ovdiff_subset.txt (functions
# that match exactly), skipped (77) without P4PC_ELF.
if(NOT P4_WIN32)
  add_executable(ovdiff "${CMAKE_SOURCE_DIR}/tools/mipsdiff/ovdiff.c" "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" "${P4_TABLES}"
    $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
  target_include_directories(ovdiff PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
  target_compile_options(ovdiff PRIVATE -fno-builtin -Wno-unused)
  target_compile_definitions(ovdiff PRIVATE ${P4_PORT_DEFINES})
  target_link_libraries(ovdiff m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
  p4_ps2_exe(ovdiff)
  target_link_options(ovdiff PRIVATE LINKER:--wrap=func_0043f810 LINKER:--wrap=func_0043f9c8)
  add_test(NAME ovdiff_subset COMMAND ovdiff -n 40 env "@${CMAKE_SOURCE_DIR}/tests/ovdiff_subset.txt")
  # the retail double libm cores against the port's bindings (identical as float), and ADX_GetCoefficient against the retail asm (exact)
  add_test(NAME ovdiff_dblcores COMMAND ovdiff -n 100000 -Q env all)
  set_tests_properties(ovdiff_dblcores PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    ENVIRONMENT "P4_MIPSDIFF_DIR=${CMAKE_SOURCE_DIR}/tools/mipsdiff")
  set_tests_properties(ovdiff_subset PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    ENVIRONMENT "P4_MIPSDIFF_DIR=${CMAKE_SOURCE_DIR}/tools/mipsdiff")
endif()

