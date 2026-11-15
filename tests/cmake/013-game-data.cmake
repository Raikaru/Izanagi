# p4.data pure layer: row decode, name forms, keys and lookup messages on
# synthetic rows; no retail data or game process.
add_executable(game_data_test game_data_test.c "${CMAKE_SOURCE_DIR}/src/platform/mods/game_data.c")
target_compile_definitions(game_data_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_data COMMAND game_data_test)
