# Every registered test, in every fragment, has a memory-card save root of its own (tests/CMakeLists.txt gives one
# to the tests that did not set theirs). Listed last so the check names itself too.
add_test(NAME save_root_policy COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/save_root_policy_test.py"
  "${CMAKE_BINARY_DIR}" "${CMAKE_CTEST_COMMAND}")
set_tests_properties(save_root_policy PROPERTIES LABELS "tools;port64" TIMEOUT 60)
