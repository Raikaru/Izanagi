# The REQUIRED expanded-skills save block over the real mod-save core, source
# ledger and a skill view materialized from real synthetic input spans; no
# gameplay claim.
get_target_property(_p4_skill_save_sources game_table_sources_test SOURCES)
list(REMOVE_ITEM _p4_skill_save_sources game_table_sources_test.c "${CMAKE_CURRENT_SOURCE_DIR}/game_table_sources_test.c")
add_executable(game_skills_save_test game_skills_save_test.c ${_p4_skill_save_sources}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_skills_save.c")
target_include_directories(game_skills_save_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard" "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_definitions(game_skills_save_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_skills_save_test)
target_link_libraries(game_skills_save_test m)
get_target_property(_p4_skill_save_runner game_skills_save_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_skill_save_runner)
  unset(_p4_skill_save_runner)
endif()
add_test(NAME game_skills_save COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_skills_test.py"
  ${_p4_skill_save_runner} "$<TARGET_FILE:game_skills_save_test>" --driver-end)
set_tests_properties(game_skills_save PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 120
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_skill_save_sources)
unset(_p4_skill_save_runner)
