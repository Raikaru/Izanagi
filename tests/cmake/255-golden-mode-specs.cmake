# Golden mode milestone-3 format specs (docs/golden_mode/specs) and the layout measurement tool
# (tools/golden_mode/layouts.py), on synthetic fixtures and the specs' own synthetic vectors: no Golden or
# retail bytes, no game. Labelled explicitly: this fragment follows the plain-test label sweep.
foreach(_p4_golden_mode_check IN ITEMS layouts specs)
  add_test(NAME golden_mode_${_p4_golden_mode_check}
    COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_mode_${_p4_golden_mode_check}_test.py")
  set_tests_properties(golden_mode_${_p4_golden_mode_check} PROPERTIES LABELS "tools;port64" TIMEOUT 120)
endforeach()
unset(_p4_golden_mode_check)
