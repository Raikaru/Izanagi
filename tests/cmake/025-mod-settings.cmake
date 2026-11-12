# Real settings registry and C hub; no GL, VM replacement or retail assets.
add_executable(mod_settings_test mod_settings_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(mod_settings_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(mod_settings_test PRIVATE ${P4_PORT_DEFINES})
target_compile_options(mod_settings_test PRIVATE -fno-builtin)
target_link_libraries(mod_settings_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(mod_settings_test)
add_test(NAME mod_settings COMMAND mod_settings_test)
set_tests_properties(mod_settings PROPERTIES TIMEOUT 30)
