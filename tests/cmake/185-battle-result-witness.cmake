# Real game battle results reported to a private script package as one_more, reward, level_up and
# skill_learned events, from the private story0415 card through Castle 1F's fights. Explicitly
# selected only in the separately admitted witness lane; never a retail gate or reference.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_result_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_result_parts "${_p4_result_pair}")
    list(GET _p4_result_parts 0 _p4_result_target)
    list(LENGTH _p4_result_parts _p4_result_count)
    set(_p4_result_suffix "")
    if(_p4_result_count GREATER 1)
      list(GET _p4_result_parts 1 _p4_result_suffix)
    endif()
    if(NOT TARGET ${_p4_result_target})
      continue()
    endif()
    get_target_property(_p4_result_runner ${_p4_result_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_result_runner)
      set(_p4_result_runner "")
    endif()
    if(_p4_result_suffix STREQUAL "64w")
      set(_p4_result_runner "${P4_MOD_WINE}")
    endif()
    # The p4pc iteration's add_test already registers the 64-bit twin
    # (000-registration.cmake); this iteration only sets its properties.
    if(NOT TEST mod_battle_result_game${_p4_result_suffix})
      add_test(NAME mod_battle_result_game${_p4_result_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_battle_result_game_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-battle-result-witness${_p4_result_suffix}"
        --driver ${_p4_result_runner} "$<TARGET_FILE:${_p4_result_target}>")
    endif()
    set_tests_properties(mod_battle_result_game${_p4_result_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 2700 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_result_suffix STREQUAL "64w")
      set_property(TEST mod_battle_result_game${_p4_result_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_result_pair)
  unset(_p4_result_parts)
  unset(_p4_result_count)
  unset(_p4_result_target)
  unset(_p4_result_suffix)
  unset(_p4_result_runner)
endif()
