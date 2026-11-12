# Reuse the exact C-only package/VFS source cohort; only this target adds the
# real config/SDL frontend. No Lua SDK or game snapshot service is substituted.
get_target_property(_p4_catalog_core mod_files_test SOURCES)
list(REMOVE_ITEM _p4_catalog_core mod_files_test.c "${CMAKE_CURRENT_SOURCE_DIR}/mod_files_test.c")
add_executable(mod_configured_catalog_test mod_configured_catalog_test.c ${_p4_catalog_core}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods_config.c"
  "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c")
target_include_directories(mod_configured_catalog_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(mod_configured_catalog_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_configured_catalog_test m ${P4_SDL3_LIB})
get_target_property(_p4_catalog_runner mod_configured_catalog_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_catalog_runner)
  unset(_p4_catalog_runner)
endif()
add_test(NAME mod_configured_catalog COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/mod_configured_catalog_test.py"
  ${_p4_catalog_runner} "$<TARGET_FILE:mod_configured_catalog_test>" --driver-end)
set_tests_properties(mod_configured_catalog PROPERTIES LABELS "host" TIMEOUT 30
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
add_executable(mod_selector_test mod_selector_test.c ${_p4_catalog_core}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/os/restart.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods_config.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/packs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/packs_config.c"
  "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c")
target_include_directories(mod_selector_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(mod_selector_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_selector_test m ${P4_SDL3_LIB})
get_target_property(_p4_selector_runner mod_selector_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_selector_runner)
  unset(_p4_selector_runner)
endif()
add_test(NAME mod_selector COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mod_selector_test.py"
  ${_p4_selector_runner} "$<TARGET_FILE:mod_selector_test>" --driver-end)
set_tests_properties(mod_selector PROPERTIES LABELS "host" TIMEOUT 30
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_selector_runner)
unset(_p4_catalog_core)
unset(_p4_catalog_runner)
