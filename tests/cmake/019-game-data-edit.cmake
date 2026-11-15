# p4.data edit layers (data/edits.xml) over real package boot and synthetic
# disc inputs: typed writes in the composed inputs, load order and conflicts,
# learnsets and refusals; session edits over the composed rows copied into
# guest memory behind their slots, with their save block round trip. No
# native loader or gameplay claim.
get_target_property(_p4_data_edit_sources game_table_derived_test SOURCES)
list(REMOVE_ITEM _p4_data_edit_sources game_table_derived_test.c
  "${CMAKE_CURRENT_SOURCE_DIR}/game_table_derived_test.c")
list(APPEND _p4_data_edit_sources game_data_edit_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_data.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_data_catalog.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_data_edit.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_data_edit_xml.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_data_session.c")
list(REMOVE_DUPLICATES _p4_data_edit_sources)
add_executable(game_data_edit_test ${_p4_data_edit_sources})
target_include_directories(game_data_edit_test PRIVATE
  "${CMAKE_SOURCE_DIR}/src/platform/files" "${CMAKE_SOURCE_DIR}/src/platform/memcard"
  "${CMAKE_SOURCE_DIR}/src/platform/mods" "${CMAKE_SOURCE_DIR}/third_party/yxml")
target_compile_definitions(game_data_edit_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_data_edit_test)
target_link_libraries(game_data_edit_test m)
get_target_property(_p4_data_edit_runner game_data_edit_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_data_edit_runner)
  unset(_p4_data_edit_runner)
endif()
add_test(NAME game_data_edit COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_data_edit_test.py"
  ${_p4_data_edit_runner} "$<TARGET_FILE:game_data_edit_test>" --driver-end)
set_tests_properties(game_data_edit PROPERTIES LABELS "host" TIMEOUT 120
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_data_edit_runner)
unset(_p4_data_edit_sources)

# The actual Challenge script, driven by synthetic semantic values and save
# callbacks; this is not an in-game or hardware witness.
if(TARGET luau_script_runner64)
  get_target_property(_p4_challenge_runner luau_script_runner64 CROSSCOMPILING_EMULATOR)
  if(NOT _p4_challenge_runner)
    set(_p4_challenge_runner "")
  endif()
  add_test(NAME mod_challenge_original_level
    COMMAND ${_p4_challenge_runner} "$<TARGET_FILE:luau_script_runner64>"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_challenge_original_fixture.luau"
      "${CMAKE_SOURCE_DIR}/mods/p4pc.challenge/main.luau"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_challenge_original_test.luau")
  set_tests_properties(mod_challenge_original_level PROPERTIES LABELS "host" TIMEOUT 120)
  unset(_p4_challenge_runner)
endif()
