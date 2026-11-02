# No retail input and no player configuration: isolated synthetic manifests/files.
add_executable(mod_files_test mod_files_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_registry.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_memory.c"
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
