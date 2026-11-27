# Every registered test, in every fragment, has a memory-card save root of its own (tests/CMakeLists.txt gives one
# to the tests that did not set theirs). Listed last so the check names itself too.
add_test(NAME save_root_policy COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/save_root_policy_test.py"
  "${CMAKE_BINARY_DIR}" "${CMAKE_CTEST_COMMAND}")
set_tests_properties(save_root_policy PROPERTIES LABELS "tools;port64" TIMEOUT 60)

# The tests with a row in tests/gate_limits.txt carry their own P4PC_GATE_NAME (tests/CMakeLists.txt), so a plain ctest reads the row.
add_test(NAME gate_limit_names COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/gate_limit_names_test.py"
  "${CMAKE_BINARY_DIR}" "${CMAKE_CTEST_COMMAND}" "${CMAKE_CURRENT_SOURCE_DIR}/gate_limits.txt")
set_tests_properties(gate_limit_names PROPERTIES LABELS "tools;port64" TIMEOUT 60)
