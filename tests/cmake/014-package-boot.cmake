# Real registry/contained file adapters only; no retail bytes or fake GPU backend.
get_target_property(_p4_boot_sources mod_files_test SOURCES)
list(REMOVE_ITEM _p4_boot_sources mod_files_test.c)
add_executable(package_boot_test package_boot_test.c ${_p4_boot_sources})
target_include_directories(package_boot_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_definitions(package_boot_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(package_boot_test m)
get_target_property(_p4_boot_runner package_boot_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_boot_runner)
  unset(_p4_boot_runner)
endif()
add_test(NAME package_boot COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/package_boot_test.py"
  ${_p4_boot_runner} "$<TARGET_FILE:package_boot_test>" --driver-end)
set_tests_properties(package_boot PROPERTIES LABELS "host" TIMEOUT 30
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0")
unset(_p4_boot_sources)
unset(_p4_boot_runner)
