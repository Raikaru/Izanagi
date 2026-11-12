# Reuse the actual production mirror and port objects, not a copied SFXZ body or
# scanner stand-in. The existing host factory selects the same full x64/Win64
# object closure and generated tables for its twins.
add_executable(cri_sfxz_crt_test cri_sfxz_crt_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(cri_sfxz_crt_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_options(cri_sfxz_crt_test PRIVATE -fno-builtin)
target_compile_definitions(cri_sfxz_crt_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(cri_sfxz_crt_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(cri_sfxz_crt_test)
add_test(NAME cri_sfxz_crt COMMAND cri_sfxz_crt_test)
set_tests_properties(cri_sfxz_crt PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)

# The allocator witness also calls the real mirrored checked wrappers. Its
# interpreter executes unchanged ELF instructions with an explicit EE kernel,
# while snapshots compare every arena/bin/header/accounting byte.
add_executable(ee_heap_retail_test ee_heap_retail_test.c "${P4_TABLES}"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(ee_heap_retail_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_compile_options(ee_heap_retail_test PRIVATE -fno-builtin)
target_compile_definitions(ee_heap_retail_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(ee_heap_retail_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(ee_heap_retail_test)
add_test(NAME ee_heap_retail COMMAND ee_heap_retail_test)
set_tests_properties(ee_heap_retail PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
