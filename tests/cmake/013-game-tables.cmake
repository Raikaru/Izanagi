# Native immutable element/channel metadata; no retail assets or game process.
add_executable(game_tables_test game_tables_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c")
target_compile_definitions(game_tables_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_tables COMMAND game_tables_test)
add_test(NAME table_consumers COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/table_consumers_test.py")

# Side-ailment battle state over a real frozen registry and guest DatUnits.
add_executable(game_side_ailment_test game_side_ailment_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_side_ailment.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(game_side_ailment_test PRIVATE ${P4_PORT_DEFINES})
target_include_directories(game_side_ailment_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_link_libraries(game_side_ailment_test m)
p4_ps2_exe(game_side_ailment_test)
add_test(NAME game_side_ailment COMMAND game_side_ailment_test)
set_tests_properties(game_side_ailment PROPERTIES SKIP_RETURN_CODE 77)

add_executable(game_text_test game_text_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_text.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_text_native.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(game_text_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_text_test m)
p4_ps2_exe(game_text_test)
add_test(NAME game_text COMMAND game_text_test)
set_tests_properties(game_text PROPERTIES SKIP_RETURN_CODE 77)

add_executable(game_tables_prepare_test game_tables_prepare_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables_prepare.c")
target_compile_definitions(game_tables_prepare_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_tables_prepare COMMAND game_tables_prepare_test)
