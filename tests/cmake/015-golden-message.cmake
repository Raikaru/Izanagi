set(P4_GOLDEN_MESSAGE_SOURCES
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_message.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_font.c")
add_executable(golden_message_test golden_message_test.c ${P4_GOLDEN_MESSAGE_SOURCES})
target_include_directories(golden_message_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME golden_message COMMAND golden_message_test)
set_tests_properties(golden_message PROPERTIES TIMEOUT 30)
add_executable(golden_message_cli golden_message_cli.c ${P4_GOLDEN_MESSAGE_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c")
target_include_directories(golden_message_cli PRIVATE "${CMAKE_SOURCE_DIR}/include")
p4_target_command(_p4_golden_message_driver golden_message_cli)
add_test(NAME golden_message_steam COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/golden_message_steam_test.py" ${_p4_golden_message_driver} --driver-end)
set_tests_properties(golden_message_steam PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
unset(_p4_golden_message_driver)
