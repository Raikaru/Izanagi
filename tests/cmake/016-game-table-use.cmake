# Real package/input/ledger/save semantic use; only the nonreturning recovery
# frontends are intercepted. This is not native UI/VM/battle qualification.
get_target_property(_p4_input_use_sources game_table_sources_test SOURCES)
list(REMOVE_ITEM _p4_input_use_sources game_table_sources_test.c
  "${CMAKE_CURRENT_SOURCE_DIR}/game_table_sources_test.c")
add_executable(game_table_inputs_use_test game_table_inputs_use_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs_use.c" ${_p4_input_use_sources})
target_include_directories(game_table_inputs_use_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/mods" "${CMAKE_SOURCE_DIR}/src/platform/memcard")
target_compile_definitions(game_table_inputs_use_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_table_inputs_use_test)
target_link_libraries(game_table_inputs_use_test m)
get_target_property(_p4_input_use_runner game_table_inputs_use_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_input_use_runner)
  unset(_p4_input_use_runner)
endif()
add_test(NAME game_table_input_use COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_table_inputs_test.py"
  ${_p4_input_use_runner} "$<TARGET_FILE:game_table_inputs_use_test>" --driver-end)
set_tests_properties(game_table_input_use PROPERTIES LABELS "host" TIMEOUT 240
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0")
unset(_p4_input_use_sources)
unset(_p4_input_use_runner)
