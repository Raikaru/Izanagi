# Real shared package/VFS core plus the independent configured pack frontend.
get_target_property(_p4_pack_core mod_files_test SOURCES)
list(REMOVE_ITEM _p4_pack_core mod_files_test.c "${CMAKE_CURRENT_SOURCE_DIR}/mod_files_test.c")
add_executable(pack_configured_catalog_test pack_configured_catalog_test.c ${_p4_pack_core}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods_config.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/packs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/packs_config.c"
  "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c")
target_include_directories(pack_configured_catalog_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(pack_configured_catalog_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(pack_configured_catalog_test m ${P4_SDL3_LIB})
get_target_property(_p4_pack_runner pack_configured_catalog_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_pack_runner)
  unset(_p4_pack_runner)
endif()
add_test(NAME pack_configured_catalog COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/pack_configured_catalog_test.py"
  ${_p4_pack_runner} "$<TARGET_FILE:pack_configured_catalog_test>" --driver-end)
set_tests_properties(pack_configured_catalog PROPERTIES LABELS "host" TIMEOUT 90
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_pack_core)
unset(_p4_pack_runner)
