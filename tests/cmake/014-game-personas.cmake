# Real Core/VFS inputs and CPU/guest phases; synthetic banks, not gameplay proof.
get_target_property(_p4_persona_input_sources mod_files_test SOURCES)
list(REMOVE_ITEM _p4_persona_input_sources mod_files_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
add_executable(game_personas_test game_personas_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_personas.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_text.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c"
  ${_p4_persona_input_sources} ${P4_GUEST_TEST_SRC})
target_include_directories(game_personas_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_definitions(game_personas_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_personas_test m)
p4_ps2_exe(game_personas_test)
get_target_property(_p4_persona_runner game_personas_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_persona_runner)
  unset(_p4_persona_runner)
endif()
add_test(NAME game_personas COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_personas_test.py"
  ${_p4_persona_runner} "$<TARGET_FILE:game_personas_test>" --driver-end)
set_tests_properties(game_personas PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 120
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0")
unset(_p4_persona_input_sources)
unset(_p4_persona_runner)
