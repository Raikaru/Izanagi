set(P4_MOD_COMPAT_TEST_SOURCES
  "${CMAKE_SOURCE_DIR}/src/platform/mods/compendium.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_memory.c"
  "${CMAKE_SOURCE_DIR}/third_party/yxml/yxml.c" "${P4_OS_SRC}")
add_executable(compendium_metadata_test compendium_metadata_test.c ${P4_MOD_COMPAT_TEST_SOURCES})
add_executable(package_files_test package_files_test.c ${P4_MOD_COMPAT_TEST_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/file_index.c")
add_executable(mod_package_read_test mod_package_read_test.c ${P4_MOD_COMPAT_TEST_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_registry.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/file_index.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c")
foreach(target compendium_metadata_test package_files_test mod_package_read_test)
  target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/mods"
    "${CMAKE_SOURCE_DIR}/third_party/yxml")
  target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
  target_compile_definitions(${target} PRIVATE ${P4_PORT_DEFINES})
  target_link_libraries(${target} m)
  get_target_property(_p4_mod_compat_runner ${target} CROSSCOMPILING_EMULATOR)
  if(NOT _p4_mod_compat_runner)
    unset(_p4_mod_compat_runner)
  endif()
  string(REGEX REPLACE "_test$" "" fixture "${target}")
  add_test(NAME ${fixture} COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/${target}.py"
    ${_p4_mod_compat_runner} "$<TARGET_FILE:${target}>" --driver-end)
endforeach()
set_tests_properties(compendium_metadata package_files mod_package_read PROPERTIES TIMEOUT 120)
unset(_p4_mod_compat_runner)
