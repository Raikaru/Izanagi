# Golden mode milestone-2 image typing (tools/golden_mode/typing_pass.py) on a synthetic MIPS image,
# declarations and haystacks: no Golden or retail bytes, no game. Labelled explicitly: this fragment
# follows the plain-test label sweep.
add_test(NAME golden_mode_typing COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_mode_typing_test.py")
set_tests_properties(golden_mode_typing PROPERTIES LABELS "tools;port64" TIMEOUT 120)
