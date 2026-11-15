# Real game giving and taking items and yen through p4.give/p4.take from a private
# script package after New Game, read back through the game's own accessors; with
# the private story0415 card also Persona skills/EXP and the give.skill choice panel.
# Explicitly selected only in the separately admitted witness lane; never a retail
# gate or reference.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_actions_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_actions_parts "${_p4_actions_pair}")
    list(GET _p4_actions_parts 0 _p4_actions_target)
    list(LENGTH _p4_actions_parts _p4_actions_count)
    set(_p4_actions_suffix "")
    if(_p4_actions_count GREATER 1)
      list(GET _p4_actions_parts 1 _p4_actions_suffix)
    endif()
    if(NOT TARGET ${_p4_actions_target})
      continue()
    endif()
    get_target_property(_p4_actions_runner ${_p4_actions_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_actions_runner)
      set(_p4_actions_runner "")
    endif()
    if(_p4_actions_suffix STREQUAL "64w")
      set(_p4_actions_runner "${P4_MOD_WINE}")
    endif()
    # The p4pc iteration's add_test already registers the 64-bit twin
    # (000-registration.cmake); this iteration only sets its properties.
    if(NOT TEST mod_actions_game${_p4_actions_suffix})
      add_test(NAME mod_actions_game${_p4_actions_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_actions_game_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-actions-witness${_p4_actions_suffix}"
        --driver ${_p4_actions_runner} "$<TARGET_FILE:${_p4_actions_target}>")
    endif()
    set_tests_properties(mod_actions_game${_p4_actions_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 2700 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_actions_suffix STREQUAL "64w")
      set_property(TEST mod_actions_game${_p4_actions_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_actions_pair)
  unset(_p4_actions_parts)
  unset(_p4_actions_count)
  unset(_p4_actions_target)
  unset(_p4_actions_suffix)
  unset(_p4_actions_runner)
endif()
