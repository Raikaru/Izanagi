# Real selected source plans and CPU seal; deliberately unavailable native stage.
get_target_property(_p4_table_boot_sources game_table_xml_test SOURCES)
list(REMOVE_ITEM _p4_table_boot_sources game_table_xml_test.c
  "${CMAKE_CURRENT_SOURCE_DIR}/game_table_xml_test.c")
list(APPEND _p4_table_boot_sources game_table_boot_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_admission.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_manifest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_boot.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_delta.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_setting_store.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_setting_gate.c")
# Early CPU seal must not allocate or free guest memory in this fixture.
list(REMOVE_ITEM _p4_table_boot_sources guest_test.c "${CMAKE_CURRENT_SOURCE_DIR}/guest_test.c"
  "${CMAKE_SOURCE_DIR}/tests/guest_test.c")
list(REMOVE_DUPLICATES _p4_table_boot_sources)
add_executable(game_table_boot_test ${_p4_table_boot_sources})
target_include_directories(game_table_boot_test PRIVATE
  "${CMAKE_SOURCE_DIR}/src/platform/files" "${CMAKE_SOURCE_DIR}/src/platform/memcard"
  "${CMAKE_SOURCE_DIR}/src/platform/mods" "${CMAKE_SOURCE_DIR}/third_party/yxml")
target_compile_definitions(game_table_boot_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_table_boot_test)
target_link_libraries(game_table_boot_test m)
get_target_property(_p4_table_boot_runner game_table_boot_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_table_boot_runner)
  unset(_p4_table_boot_runner)
endif()
add_test(NAME game_table_boot COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_table_boot_test.py"
  ${_p4_table_boot_runner} "$<TARGET_FILE:game_table_boot_test>" --driver-end)
set_tests_properties(game_table_boot PROPERTIES LABELS "host" TIMEOUT 60
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_table_boot_runner)
unset(_p4_table_boot_sources)
