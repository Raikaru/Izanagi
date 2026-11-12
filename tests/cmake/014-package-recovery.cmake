# Actual core, startup-health and atomic settings writes; no dialog/backend mock.
get_target_property(_p4_recovery_core mod_files_test SOURCES)
list(REMOVE_ITEM _p4_recovery_core mod_files_test.c)
add_executable(mod_recovery_config_test mod_recovery_config_test.c ${_p4_recovery_core}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/failure_config.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/boot_health.c"
  "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c")
target_compile_definitions(mod_recovery_config_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_recovery_config_test m ${P4_SDL3_LIB})
get_target_property(_p4_recovery_runner mod_recovery_config_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_recovery_runner)
  unset(_p4_recovery_runner)
endif()
add_test(NAME mod_recovery_config COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mod_recovery_config_test.py"
  ${_p4_recovery_runner} "$<TARGET_FILE:mod_recovery_config_test>" --driver-end)
set_tests_properties(mod_recovery_config PROPERTIES LABELS "host" TIMEOUT 120
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_recovery_runner)
unset(_p4_recovery_core)
