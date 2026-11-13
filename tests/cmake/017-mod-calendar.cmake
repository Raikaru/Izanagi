# p4.calendar.pass (calendar.pass@1) in the real game, explicitly selected only in the
# separately admitted witness lane (P4PC_MOD_WITNESS_TESTS). Private copied card,
# no references.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_calendar_pair "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_calendar_parts "${_p4_calendar_pair}")
    list(GET _p4_calendar_parts 0 _p4_calendar_target)
    list(GET _p4_calendar_parts 1 _p4_calendar_suffix)
    if(NOT TARGET ${_p4_calendar_target})
      continue()
    endif()
    get_target_property(_p4_calendar_runner ${_p4_calendar_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_calendar_runner)
      set(_p4_calendar_runner "")
    endif()
    if(_p4_calendar_suffix STREQUAL "64w")
      set(_p4_calendar_runner "${P4_MOD_WINE}")
    endif()
    _add_test(NAME mod_calendar_game${_p4_calendar_suffix} COMMAND "${P4_PYTHON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_calendar_game_test.py"
      --out "${CMAKE_BINARY_DIR}/mod-calendar-witness${_p4_calendar_suffix}"
      --driver ${_p4_calendar_runner} "$<TARGET_FILE:${_p4_calendar_target}>")
    set_tests_properties(mod_calendar_game${_p4_calendar_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 2700 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_calendar_suffix STREQUAL "64w")
      set_property(TEST mod_calendar_game${_p4_calendar_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_calendar_pair)
  unset(_p4_calendar_parts)
  unset(_p4_calendar_target)
  unset(_p4_calendar_suffix)
  unset(_p4_calendar_runner)
endif()
