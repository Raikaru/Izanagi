# The bundled Physical Split mod (mods/p4pc.physsplit): its committed package
# must be exactly what tools/physsplit.py generates from the authored data.
add_test(NAME physsplit_package COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/physsplit.py" check)
set_tests_properties(physsplit_package PROPERTIES LABELS "host" TIMEOUT 120)

# Real game with the shipped package, explicitly selected only in the separately
# admitted witness lane. Never a retail gate or reference.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_split_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_split_parts "${_p4_split_pair}")
    list(GET _p4_split_parts 0 _p4_split_target)
    list(LENGTH _p4_split_parts _p4_split_count)
    set(_p4_split_suffix "")
    if(_p4_split_count GREATER 1)
      list(GET _p4_split_parts 1 _p4_split_suffix)
    endif()
    if(NOT TARGET ${_p4_split_target})
      continue()
    endif()
    get_target_property(_p4_split_runner ${_p4_split_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_split_runner)
      set(_p4_split_runner "")
    endif()
    if(_p4_split_suffix STREQUAL "64w")
      set(_p4_split_runner "${P4_MOD_WINE}")
    endif()
    if(NOT TEST mod_physsplit_game${_p4_split_suffix})
      add_test(NAME mod_physsplit_game${_p4_split_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_physsplit_game_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-physsplit-witness${_p4_split_suffix}"
        --driver ${_p4_split_runner} "$<TARGET_FILE:${_p4_split_target}>")
    endif()
    set_tests_properties(mod_physsplit_game${_p4_split_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 10800 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_split_suffix STREQUAL "64w")
      set_property(TEST mod_physsplit_game${_p4_split_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_split_pair)
  unset(_p4_split_parts)
  unset(_p4_split_count)
  unset(_p4_split_target)
  unset(_p4_split_suffix)
  unset(_p4_split_runner)
endif()
