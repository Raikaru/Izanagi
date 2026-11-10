add_test(NAME ps2save_export COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/ps2save_export_test.py")
set_tests_properties(ps2save_export PROPERTIES LABELS "tools;port64" TIMEOUT 180)
