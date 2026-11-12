# Real Core/VFS inputs and CPU/guest phases; synthetic banks, not gameplay proof.
get_target_property(_p4_enemy_input_sources mod_files_test SOURCES)
list(REMOVE_ITEM _p4_enemy_input_sources mod_files_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
add_executable(game_enemies_test game_enemies_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_enemies.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_text.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c"
  ${_p4_enemy_input_sources} ${P4_GUEST_TEST_SRC})
target_include_directories(game_enemies_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_definitions(game_enemies_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_enemies_test m)
p4_ps2_exe(game_enemies_test)
get_target_property(_p4_enemy_runner game_enemies_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_enemy_runner)
  unset(_p4_enemy_runner)
endif()
add_test(NAME game_enemies COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_enemies_test.py"
  ${_p4_enemy_runner} "$<TARGET_FILE:game_enemies_test>" --driver-end)
set_tests_properties(game_enemies PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 120
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0")
unset(_p4_enemy_input_sources)
unset(_p4_enemy_runner)
