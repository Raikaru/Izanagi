# API capability registry: include/p4pc/p4.d/<key>.luau fragments parse and compose
# (tools/p4_capabilities.py) and agree with the native table's header rule.
add_test(NAME capabilities COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/p4_capabilities_test.py")
set_tests_properties(capabilities PROPERTIES LABELS "tools;port64" TIMEOUT 60)
