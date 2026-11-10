# [table-derived] producers over real package boot, synthetic disc inputs, the
# admission rewrite and the skill-pair relocation at battle-table completion in
# guest memory; no native loader or gameplay claim.
get_target_property(_p4_table_derived_sources game_table_boot_test SOURCES)
list(REMOVE_ITEM _p4_table_derived_sources game_table_boot_test.c)
list(APPEND _p4_table_derived_sources game_table_derived_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_derived_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_extents.c")
list(REMOVE_DUPLICATES _p4_table_derived_sources)
add_executable(game_table_derived_test ${_p4_table_derived_sources})
target_include_directories(game_table_derived_test PRIVATE
  "${CMAKE_SOURCE_DIR}/src/platform/files" "${CMAKE_SOURCE_DIR}/src/platform/memcard"
  "${CMAKE_SOURCE_DIR}/src/platform/mods" "${CMAKE_SOURCE_DIR}/third_party/yxml")
target_compile_definitions(game_table_derived_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_table_derived_test)
target_link_libraries(game_table_derived_test m)
get_target_property(_p4_table_derived_runner game_table_derived_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_table_derived_runner)
  unset(_p4_table_derived_runner)
endif()
add_test(NAME game_table_derived COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_table_derived_test.py"
  ${_p4_table_derived_runner} "$<TARGET_FILE:game_table_derived_test>" --driver-end)
set_tests_properties(game_table_derived PROPERTIES LABELS "host" TIMEOUT 120
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_table_derived_runner)
unset(_p4_table_derived_sources)
