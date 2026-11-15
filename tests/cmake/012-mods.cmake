# No retail input and no player configuration: isolated synthetic manifests/files.
add_executable(mod_files_test mod_files_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_registry.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_memory.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_setting_store.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/lifecycle.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_artifacts.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/compendium.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/file_index.c"
  "${CMAKE_SOURCE_DIR}/third_party/yxml/yxml.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
target_include_directories(mod_files_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(mod_files_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_files_test m)
# CTest only applies the emulator to its outer command, not Python's child.
get_target_property(_p4_mod_files_runner mod_files_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_mod_files_runner)
  unset(_p4_mod_files_runner)
endif()
add_test(NAME mod_files COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mod_files_test.py"
  ${_p4_mod_files_runner} "$<TARGET_FILE:mod_files_test>" --driver-end)
unset(_p4_mod_files_runner)
# Mod lifecycle sessions and crash markers on the real OS file layer.
add_executable(mod_lifecycle_test mod_lifecycle_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/lifecycle.c" "${P4_OS_SRC}")
target_compile_definitions(mod_lifecycle_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_lifecycle_test m)
add_test(NAME mod_lifecycle COMMAND mod_lifecycle_test "${CMAKE_CURRENT_BINARY_DIR}/mod-lifecycle-data")
set_tests_properties(mod_lifecycle PROPERTIES LABELS "host" TIMEOUT 30)
# Mod artifact files (p4.artifacts) on the real OS file layer.
add_executable(mod_artifacts_test mod_artifacts_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_artifacts.c" "${P4_OS_SRC}")
target_compile_definitions(mod_artifacts_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_artifacts_test m)
add_test(NAME mod_artifacts COMMAND mod_artifacts_test "${CMAKE_CURRENT_BINARY_DIR}/mod-artifacts-data")
set_tests_properties(mod_artifacts PROPERTIES LABELS "host" TIMEOUT 60)
# Operation authorizers: closed operations, integer context, first denial wins.
add_executable(mod_authorize_test mod_authorize_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/authorize.c")
target_compile_definitions(mod_authorize_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME mod_authorize COMMAND mod_authorize_test)
set_tests_properties(mod_authorize PROPERTIES LABELS "host" TIMEOUT 30)
# Weapon fusion transaction over a fake game state (accessors stubbed in the test).
add_executable(mod_forge_test mod_forge_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/forge_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/authorize.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(mod_forge_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(mod_forge_test)
add_test(NAME mod_forge COMMAND mod_forge_test)
set_tests_properties(mod_forge PROPERTIES LABELS "host" TIMEOUT 30)
# Equipment exchange over a fake game state (accessors stubbed in the test).
add_executable(mod_equipment_test mod_equipment_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/equipment_native.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(mod_equipment_test PRIVATE ${P4_PORT_DEFINES})
target_include_directories(mod_equipment_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/mods")
p4_ps2_exe(mod_equipment_test)
add_test(NAME mod_equipment COMMAND mod_equipment_test)
set_tests_properties(mod_equipment PROPERTIES LABELS "host" TIMEOUT 30)
# Velvet Room command over stubbed Config/settings services (002e7ac0 seams).
add_executable(mod_velvet_test mod_velvet_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/velvet_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/authorize.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(mod_velvet_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(mod_velvet_test)
add_test(NAME mod_velvet COMMAND mod_velvet_test)
set_tests_properties(mod_velvet PROPERTIES LABELS "host" TIMEOUT 30)
