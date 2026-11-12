# Real selected package/native plans; no retail bytes or loaded-guest authority mock.
get_target_property(_p4_input_sources mod_files_test SOURCES)
list(REMOVE_ITEM _p4_input_sources mod_files_test.c "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c")
add_executable(game_table_inputs_test game_table_inputs_test.c vfs_source_iso.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c" ${_p4_input_sources})
target_include_directories(game_table_inputs_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_definitions(game_table_inputs_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_table_inputs_test m)
get_target_property(_p4_input_runner game_table_inputs_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_input_runner)
  unset(_p4_input_runner)
endif()
add_test(NAME game_table_inputs COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_table_inputs_test.py"
  ${_p4_input_runner} "$<TARGET_FILE:game_table_inputs_test>" --driver-end)
set_tests_properties(game_table_inputs PROPERTIES LABELS "host" TIMEOUT 240
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0")
unset(_p4_input_sources)
unset(_p4_input_runner)
