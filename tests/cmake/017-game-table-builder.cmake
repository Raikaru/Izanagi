# CPU schema and metadata validation; no native loader or gameplay authority.
add_executable(game_table_manifest_test game_table_manifest_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_manifest.c")
target_compile_definitions(game_table_manifest_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_table_manifest COMMAND game_table_manifest_test)
set_tests_properties(game_table_manifest PROPERTIES LABELS "host" TIMEOUT 30)
get_target_property(_p4_xml_sources game_table_sources_test SOURCES)
list(REMOVE_ITEM _p4_xml_sources game_table_sources_test.c "${CMAKE_CURRENT_SOURCE_DIR}/game_table_sources_test.c")
add_executable(game_table_xml_test game_table_xml_test.c ${_p4_xml_sources}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_xml.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_builder.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_derived.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs_use.c")
target_include_directories(game_table_xml_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard" "${CMAKE_SOURCE_DIR}/src/platform/mods"
  "${CMAKE_SOURCE_DIR}/third_party/yxml")
target_compile_definitions(game_table_xml_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_table_xml_test)
target_link_libraries(game_table_xml_test m)
add_test(NAME game_table_xml COMMAND game_table_xml_test)
set_tests_properties(game_table_xml PROPERTIES LABELS "host" TIMEOUT 60
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
add_executable(game_table_builder_test game_table_builder_test.c ${_p4_xml_sources}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items_ui.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_xml.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_builder.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_derived.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs_use.c")
target_include_directories(game_table_builder_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard" "${CMAKE_SOURCE_DIR}/src/platform/mods"
  "${CMAKE_SOURCE_DIR}/third_party/yxml")
target_compile_definitions(game_table_builder_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_table_builder_test)
target_link_libraries(game_table_builder_test m)
get_target_property(_p4_builder_runner game_table_builder_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_builder_runner)
  unset(_p4_builder_runner)
endif()
add_test(NAME game_table_builder COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_elements_inputs_test.py"
  ${_p4_builder_runner} "$<TARGET_FILE:game_table_builder_test>" --driver-end)
set_tests_properties(game_table_builder PROPERTIES LABELS "host" TIMEOUT 60
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
add_test(NAME fusion_arts_rows COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/fusion_arts_rows_test.py"
  ${_p4_builder_runner} "$<TARGET_FILE:game_table_builder_test>" --driver-end)
set_tests_properties(fusion_arts_rows PROPERTIES LABELS "host" TIMEOUT 240
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_builder_runner)
unset(_p4_xml_sources)
