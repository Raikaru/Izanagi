# Element-UI model only: real registry and resource token service with a mocked
# raster backend. Native draws, the boot provider and captures are separate gates.
add_executable(game_elements_ui_test game_elements_ui_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_ui.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_resources.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(game_elements_ui_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_elements_ui_test m)
p4_ps2_exe(game_elements_ui_test)
add_test(NAME game_elements_ui COMMAND game_elements_ui_test)
set_tests_properties(game_elements_ui PROPERTIES SKIP_RETURN_CODE 77 LABELS host)
