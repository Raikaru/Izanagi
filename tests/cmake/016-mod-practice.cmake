# The API 1.4 game services and the bundled practice mod in the real game,
# explicitly selected only in the separately admitted witness lane
# (P4PC_MOD_WITNESS_TESTS, tests/cmake/014-mod-recovery.cmake). Private copied
# card, no references.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_practice_pair "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_practice_parts "${_p4_practice_pair}")
    list(GET _p4_practice_parts 0 _p4_practice_target)
    list(GET _p4_practice_parts 1 _p4_practice_suffix)
    if(NOT TARGET ${_p4_practice_target})
      continue()
    endif()
    get_target_property(_p4_practice_runner ${_p4_practice_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_practice_runner)
      set(_p4_practice_runner "")
    endif()
    if(_p4_practice_suffix STREQUAL "64w")
      set(_p4_practice_runner "${P4_MOD_WINE}")
    endif()
    _add_test(NAME mod_practice_game${_p4_practice_suffix} COMMAND "${P4_PYTHON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_practice_game_test.py"
      --out "${CMAKE_BINARY_DIR}/mod-practice-witness${_p4_practice_suffix}"
      --driver ${_p4_practice_runner} "$<TARGET_FILE:${_p4_practice_target}>")
    set_tests_properties(mod_practice_game${_p4_practice_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 7200 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_practice_suffix STREQUAL "64w")
      set_property(TEST mod_practice_game${_p4_practice_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_practice_pair)
  unset(_p4_practice_parts)
  unset(_p4_practice_target)
  unset(_p4_practice_suffix)
  unset(_p4_practice_runner)
endif()
