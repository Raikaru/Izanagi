# The production pad/nav registration must fence field input during teardown,
# while leaving title/card input usable before the collision scene exists.
add_executable(nav_probe_test nav_probe_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/nav_probe.c"
  "${CMAKE_SOURCE_DIR}/src/platform/pad/pad_sdl.c"
  "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c"
  "${CMAKE_SOURCE_DIR}/src/platform/pad/ps2_pad.c"
  "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c"
  ${P4_GUEST_TEST_SRC})
target_sources(nav_probe_test PRIVATE battle_sim_inactive.c)
target_compile_definitions(nav_probe_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(nav_probe_test m ${P4_SDL3_LIB})
p4_ps2_exe(nav_probe_test)
add_test(NAME nav_probe COMMAND nav_probe_test "${CMAKE_CURRENT_SOURCE_DIR}/nav_probe.walk")
