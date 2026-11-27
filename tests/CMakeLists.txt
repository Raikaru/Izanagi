# Numbered fragments share this directory and policy scope. Preserve declaration
# order: repeat an area in another fragment rather than moving dependent blocks.
file(GLOB _p4_test_modules CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/cmake/*.cmake")
list(SORT _p4_test_modules)
foreach(_p4_test_module IN LISTS _p4_test_modules)
  include("${_p4_test_module}" NO_POLICY_SCOPE)
endforeach()
unset(_p4_test_module)
unset(_p4_test_modules)
# Isolated table fixtures do not load player configuration. Fixtures exercising
# Golden mode provide their own selector; the remaining table cohorts use PS2.
get_property(_p4_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_p4_target IN LISTS _p4_targets)
  if(NOT _p4_target MATCHES "^(game_personas_test|game_skills_test|game_data_edit_test|golden_mode_pipeline_test|golden_mode_image_test)(64w|64)?$")
    get_target_property(_p4_sources ${_p4_target} SOURCES)
    if("${_p4_sources}" MATCHES "/game_(personas|personas_native|skills|skills_native|table_derived)\\.c")
      target_sources(${_p4_target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/game_mode_ps2_stub.c")
    endif()
  endif()
endforeach()
unset(_p4_targets)
unset(_p4_target)
unset(_p4_sources)


# Ordinary and synthetic tests forcibly disable both player settings selectors.
# The separately admitted game witnesses explicitly enable mods. No test reads
# the mods shipped beside the program (a
# build tree may have them); fixtures that need a bundled root set
# P4PC_BUNDLED_MOD_DIR themselves.
get_property(_p4_tests DIRECTORY PROPERTY TESTS)
foreach(_p4_test IN LISTS _p4_tests)
  if(NOT _p4_test MATCHES "^mod_(example|qol|physsplit|golden|calendar|randomizer|fusion_arts)_game(64w|64)?$")
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
unset(_p4_environment)

# A gate with a row in tests/gate_limits.txt is judged by that row's limit on a matching renderer only if the game
# knows the gate's name (src/render/p4gl_gl.c p4gl_gate_limit reads P4PC_GATE_NAME). The game runners that judge a gate by
# its renderer set it themselves; a plain ctest did not, so the NVIDIA rows were never read there (a cloud L4 failed
# systems_shop_daidara64 at 6.0188% against the llvmpipe 6.0 while the row says 6.07). Tests with a row get their own name;
# tests/gate_limit_names_test.py checks it. Only gates with a row are touched: a gate keeps its name-free environment
# unless a renderer row says it needs one.
set(_p4_limits "${CMAKE_CURRENT_SOURCE_DIR}/gate_limits.txt")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_p4_limits}")
file(STRINGS "${_p4_limits}" _p4_limit_rows REGEX "^[A-Za-z0-9_.-]+[ 	]+[^ 	#]+[ 	]+[0-9.]+")
foreach(_p4_row IN LISTS _p4_limit_rows)
  string(REGEX MATCH "^[A-Za-z0-9_.-]+" _p4_gate "${_p4_row}")
  if(TEST ${_p4_gate})
    get_test_property(${_p4_gate} ENVIRONMENT _p4_environment)
    if(NOT "${_p4_environment}" MATCHES "(^|;)P4PC_GATE_NAME=")
      set_property(TEST ${_p4_gate} APPEND PROPERTY ENVIRONMENT "P4PC_GATE_NAME=${_p4_gate}")
    endif()
  endif()
endforeach()
unset(_p4_limits)
unset(_p4_limit_rows)
unset(_p4_row)
unset(_p4_gate)
unset(_p4_environment)
unset(_p4_test)
unset(_p4_tests)
