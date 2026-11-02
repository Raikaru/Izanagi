# Real game with privately generated added-enemy clones, explicitly selected
# only in the separately admitted witness lane. Never a retail gate or reference.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_enemy_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_enemy_parts "${_p4_enemy_pair}")
    list(GET _p4_enemy_parts 0 _p4_enemy_target)
    list(LENGTH _p4_enemy_parts _p4_enemy_count)
    set(_p4_enemy_suffix "")
    if(_p4_enemy_count GREATER 1)
      list(GET _p4_enemy_parts 1 _p4_enemy_suffix)
    endif()
    if(NOT TARGET ${_p4_enemy_target})
      continue()
    endif()
    get_target_property(_p4_enemy_runner ${_p4_enemy_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_enemy_runner)
      set(_p4_enemy_runner "")
    endif()
    if(_p4_enemy_suffix STREQUAL "64w")
      set(_p4_enemy_runner "${P4_MOD_WINE}")
    endif()
    add_test(NAME mod_enemy_game${_p4_enemy_suffix} COMMAND "${P4_PYTHON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_enemy_game_test.py"
      --out "${CMAKE_BINARY_DIR}/mod-enemy-witness${_p4_enemy_suffix}"
      --driver ${_p4_enemy_runner} "$<TARGET_FILE:${_p4_enemy_target}>")
    set_tests_properties(mod_enemy_game${_p4_enemy_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 5400 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_enemy_suffix STREQUAL "64w")
      set_property(TEST mod_enemy_game${_p4_enemy_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_enemy_pair)
  unset(_p4_enemy_parts)
  unset(_p4_enemy_count)
  unset(_p4_enemy_target)
  unset(_p4_enemy_suffix)
  unset(_p4_enemy_runner)
endif()
