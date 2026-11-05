# Native providers plus the real event/RAM dump fidelity refusal; no retail inputs.
add_executable(package_registry_test package_registry_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_registry.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_memory.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_setting_store.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/compendium.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/file_index.c"
  "${CMAKE_SOURCE_DIR}/third_party/yxml/yxml.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" ${P4_GUEST_TEST_SRC}
  "${CMAKE_SOURCE_DIR}/src/platform/state_dump.c" state_dump_test_stubs.c)
target_include_directories(package_registry_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_definitions(package_registry_test PRIVATE ${P4_PORT_DEFINES} P4_MOD_GAME_BINDINGS=1)
target_link_libraries(package_registry_test m)
get_target_property(_p4_package_runner package_registry_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_package_runner)
  unset(_p4_package_runner)
endif()
add_test(NAME package_registry COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/package_registry_test.py"
  ${_p4_package_runner} "$<TARGET_FILE:package_registry_test>" --driver-end)
set_tests_properties(package_registry PROPERTIES LABELS "host" TIMEOUT 30
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0")
unset(_p4_package_runner)
