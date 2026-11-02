add_test(NAME cmake_sources COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_sources_test.py")
set_tests_properties(cmake_sources PROPERTIES LABELS "tools;port64" TIMEOUT 180)
