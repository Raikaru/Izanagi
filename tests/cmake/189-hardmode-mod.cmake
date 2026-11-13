# The bundled hard mode (mods/p4pc.hardmode) in the real game with the shipped
# package, explicitly selected only in the separately admitted witness lane.
# Never a retail gate or reference.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_hard_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_hard_parts "${_p4_hard_pair}")
    list(GET _p4_hard_parts 0 _p4_hard_target)
    list(LENGTH _p4_hard_parts _p4_hard_count)
    set(_p4_hard_suffix "")
    if(_p4_hard_count GREATER 1)
      list(GET _p4_hard_parts 1 _p4_hard_suffix)
    endif()
    if(NOT TARGET ${_p4_hard_target})
      continue()
    endif()
    get_target_property(_p4_hard_runner ${_p4_hard_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_hard_runner)
      set(_p4_hard_runner "")
    endif()
    if(_p4_hard_suffix STREQUAL "64w")
      set(_p4_hard_runner "${P4_MOD_WINE}")
    endif()
    if(NOT TEST mod_hardmode_game${_p4_hard_suffix})
      add_test(NAME mod_hardmode_game${_p4_hard_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_hardmode_game_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-hardmode-witness${_p4_hard_suffix}"
        --driver ${_p4_hard_runner} "$<TARGET_FILE:${_p4_hard_target}>")
    endif()
    set_tests_properties(mod_hardmode_game${_p4_hard_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 7200 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_hard_suffix STREQUAL "64w")
      set_property(TEST mod_hardmode_game${_p4_hard_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_hard_pair)
  unset(_p4_hard_parts)
  unset(_p4_hard_count)
  unset(_p4_hard_target)
  unset(_p4_hard_suffix)
  unset(_p4_hard_runner)
endif()
