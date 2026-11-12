# Real kernel ownership, alias identity and process-death release; no MC files.
add_executable(os_directory_lock_test os_directory_lock_test.c "${P4_OS_SRC}")
target_compile_definitions(os_directory_lock_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(os_directory_lock_test m)
get_target_property(_p4_directory_runner os_directory_lock_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_directory_runner)
  unset(_p4_directory_runner)
endif()
add_test(NAME os_directory_lock COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/os_directory_lock_test.py"
  ${_p4_directory_runner} "$<TARGET_FILE:os_directory_lock_test>" --driver-end)
set_tests_properties(os_directory_lock PROPERTIES LABELS "host" TIMEOUT 90
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_CONFIG=none")
unset(_p4_directory_runner)
