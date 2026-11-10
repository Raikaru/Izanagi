# Exact source origins and expanded item sidecar; synthetic files, no gameplay claim.
get_target_property(_p4_table_sources_core mod_save_test SOURCES)
list(REMOVE_ITEM _p4_table_sources_core mod_save_test.c "${CMAKE_CURRENT_SOURCE_DIR}/mod_save_test.c")
set(_p4_table_sources_sources game_table_sources_test.c ${_p4_table_sources_core}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_sources.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_refs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables_prepare.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_skills.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_enemies.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_personas.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items_save.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_text.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c" ${P4_GUEST_TEST_SRC})
list(REMOVE_DUPLICATES _p4_table_sources_sources)
add_executable(game_table_sources_test ${_p4_table_sources_sources})
target_include_directories(game_table_sources_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard")
target_compile_definitions(game_table_sources_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_table_sources_test)
target_link_libraries(game_table_sources_test m)
get_target_property(_p4_table_sources_runner game_table_sources_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_table_sources_runner)
  unset(_p4_table_sources_runner)
endif()
add_test(NAME game_table_sources COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_table_sources_test.py"
  ${_p4_table_sources_runner} "$<TARGET_FILE:game_table_sources_test>" --driver-end)
set_tests_properties(game_table_sources PROPERTIES LABELS "host" TIMEOUT 60
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_table_sources_core)
unset(_p4_table_sources_runner)
unset(_p4_table_sources_sources)
