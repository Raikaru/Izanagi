# Committed mod UI art stays inside its native family (docs/modding/ui_art.md):
# cell, ramp, atlas rectangles, coverage and stroke. Dependency-free; the
# recorded native ranges stand in for the player's disc.
add_test(NAME ui_art_check COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/ui_art/ui_art.py" check)
set_tests_properties(ui_art_check PROPERTIES LABELS "host" TIMEOUT 120)
