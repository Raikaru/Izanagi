# Real game with p4pc.golden's derived tables from the user's own Persona 4 Golden
# install (P4PC_GOLDEN), explicitly selected only in the separately admitted
# witness lane (P4PC_MOD_WITNESS_TESTS). Never a retail gate or reference.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_golden_pair "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_golden_parts "${_p4_golden_pair}")
    list(GET _p4_golden_parts 0 _p4_golden_target)
    list(GET _p4_golden_parts 1 _p4_golden_suffix)
    if(NOT TARGET ${_p4_golden_target})
      continue()
    endif()
    get_target_property(_p4_golden_runner ${_p4_golden_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_golden_runner)
      set(_p4_golden_runner "")
    endif()
    if(_p4_golden_suffix STREQUAL "64w")
      set(_p4_golden_runner "${P4_MOD_WINE}")
    endif()
    _add_test(NAME mod_golden_game${_p4_golden_suffix} COMMAND "${P4_PYTHON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_golden_game_test.py"
      --out "${CMAKE_BINARY_DIR}/mod-golden-witness${_p4_golden_suffix}"
      --driver ${_p4_golden_runner} "$<TARGET_FILE:${_p4_golden_target}>")
    set_tests_properties(mod_golden_game${_p4_golden_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 5400 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_golden_suffix STREQUAL "64w")
      set_property(TEST mod_golden_game${_p4_golden_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_golden_pair)
  unset(_p4_golden_parts)
  unset(_p4_golden_target)
  unset(_p4_golden_suffix)
  unset(_p4_golden_runner)
endif()
