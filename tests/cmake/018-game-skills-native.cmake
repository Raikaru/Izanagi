# Actual skill staging/rollback and real synthetic input spans; no gameplay claim.
get_target_property(_p4_skill_native_sources game_table_sources_test SOURCES)
list(REMOVE_ITEM _p4_skill_native_sources game_table_sources_test.c "${CMAKE_CURRENT_SOURCE_DIR}/game_table_sources_test.c")
add_executable(game_skills_native_test game_skills_native_test.c ${_p4_skill_native_sources}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs_use.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_skills_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_enemies_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_personas_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables_packed.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_admission.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_extents.c")
target_include_directories(game_skills_native_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard" "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_definitions(game_skills_native_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_skills_native_test)
target_link_libraries(game_skills_native_test m)
get_target_property(_p4_skill_native_runner game_skills_native_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_skill_native_runner)
  unset(_p4_skill_native_runner)
endif()
add_test(NAME game_skills_native COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_elements_inputs_test.py"
  ${_p4_skill_native_runner} "$<TARGET_FILE:game_skills_native_test>" --driver-end)
set_tests_properties(game_skills_native PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 60
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_skill_native_sources)
unset(_p4_skill_native_runner)
