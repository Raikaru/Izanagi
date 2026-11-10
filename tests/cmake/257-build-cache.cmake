# Build-output caching never reuses test verdicts or relaxes generation guards.
add_test(NAME build_cache COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/build_cache_test.py")
set_tests_properties(build_cache PROPERTIES LABELS "tools;port64" TIMEOUT 180)
