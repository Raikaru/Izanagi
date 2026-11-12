# Owned metadata/boot fixture; raster backend, file lookup and queue are mocked.
# No actual battle/menu, save persistence or renderer qualification here.
add_executable(game_items_test game_items_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items_ui.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items_equipment.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items_battle.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items_list.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items_shop.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables_prepare.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_text.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_resources.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(game_items_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_items_test m)
p4_ps2_exe(game_items_test)
add_test(NAME game_items COMMAND game_items_test)
set_tests_properties(game_items PROPERTIES SKIP_RETURN_CODE 77)