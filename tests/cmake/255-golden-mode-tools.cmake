# Golden mode milestone-1 measurement tools (tools/golden_mode) on synthetic fixtures: no Golden or retail bytes,
# no game. This fragment follows the plain-test label sweep, so label these registrations explicitly.
foreach(_p4_golden_mode_tool IN ITEMS formats inventory provenance sources)
  add_test(NAME golden_mode_${_p4_golden_mode_tool}
    COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_mode_${_p4_golden_mode_tool}_test.py")
  set_tests_properties(golden_mode_${_p4_golden_mode_tool} PROPERTIES LABELS "tools;port64" TIMEOUT 120)
endforeach()
unset(_p4_golden_mode_tool)
