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
# The separately admitted game witnesses explicitly enable mods. No test reads
# the mods shipped beside the program (a
# build tree may have them); fixtures that need a bundled root set
# P4PC_BUNDLED_MOD_DIR themselves.
get_property(_p4_tests DIRECTORY PROPERTY TESTS)
foreach(_p4_test IN LISTS _p4_tests)
  if(NOT _p4_test MATCHES "^mod_(example|qol|physsplit|golden|calendar)_game(64w|64)?$")
    set_property(TEST ${_p4_test} APPEND PROPERTY ENVIRONMENT "P4PC_MODS=0" "P4PC_PACKS=0")
  endif()
  set_property(TEST ${_p4_test} APPEND PROPERTY ENVIRONMENT "P4PC_BUNDLED_MOD_DIR=")
  # Fragment 200's rule for the tests registered after it: a test without a memory-card root of its own gets
  # one. Two games sharing the default ./p4pc_save contend for the card's directory lock, and a game whose card
  # was refused never leaves the boot card check (field_mad64 timed out that way, 2026-11-10).
  get_test_property(${_p4_test} ENVIRONMENT _p4_environment)
  if(NOT "${_p4_environment}" MATCHES "(^|;)P4PC_SAVE_DIR=")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/save/${_p4_test}")
    set_property(TEST ${_p4_test} APPEND PROPERTY ENVIRONMENT "P4PC_SAVE_DIR=${CMAKE_BINARY_DIR}/save/${_p4_test}")
  endif()
endforeach()
unset(_p4_test)
unset(_p4_environment)
unset(_p4_tests)
