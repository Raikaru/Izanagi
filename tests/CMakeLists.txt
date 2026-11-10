# Numbered fragments share this directory and policy scope. Preserve declaration
# order: repeat an area in another fragment rather than moving dependent blocks.
file(GLOB _p4_test_modules CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/cmake/*.cmake")
list(SORT _p4_test_modules)
foreach(_p4_test_module IN LISTS _p4_test_modules)
  include("${_p4_test_module}" NO_POLICY_SCOPE)
endforeach()
unset(_p4_test_module)
unset(_p4_test_modules)

# Ordinary and synthetic tests forcibly disable both player settings selectors.
# The separately admitted example and bundled mod witnesses are real, explicitly
# enabled mod witnesses. No test reads the mods shipped beside the program (a
# build tree may have them); fixtures that need a bundled root set
# P4PC_BUNDLED_MOD_DIR themselves.
get_property(_p4_tests DIRECTORY PROPERTY TESTS)
foreach(_p4_test IN LISTS _p4_tests)
  if(NOT _p4_test MATCHES "^mod_(example|qol|physsplit)_game(64w|64)?$")
    set_property(TEST ${_p4_test} APPEND PROPERTY ENVIRONMENT "P4PC_MODS=0" "P4PC_PACKS=0")
  endif()
  set_property(TEST ${_p4_test} APPEND PROPERTY ENVIRONMENT "P4PC_BUNDLED_MOD_DIR=")
endforeach()
unset(_p4_test)
unset(_p4_tests)
