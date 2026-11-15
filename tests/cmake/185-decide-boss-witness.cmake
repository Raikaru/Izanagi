# p4.battle.decide on a private boss scenario in the native battle simulator, with and without
# the bundled hard mode (tests/mod_battle_decide_sim_test.py). Selected only in the separately
# admitted witness lane; never a retail gate or reference. battle_sim forbids P4PC_MOD_WITNESS in
# its own environment and declares the mod cohort per batch, so none is set here.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_decide_pair "p4pc|" "p4pc64|64")
    string(REPLACE "|" ";" _p4_decide_parts "${_p4_decide_pair}")
    list(GET _p4_decide_parts 0 _p4_decide_target)
    list(LENGTH _p4_decide_parts _p4_decide_count)
    set(_p4_decide_suffix "")
    if(_p4_decide_count GREATER 1)
      list(GET _p4_decide_parts 1 _p4_decide_suffix)
    endif()
    if(NOT TARGET ${_p4_decide_target})
      continue()
    endif()
    # Register explicitly: the generic add_test wrapper clones argv unchanged,
    # including --out, so its twin would overwrite the 32-bit artifacts.
    if(NOT TEST mod_battle_decide_sim${_p4_decide_suffix})
      _add_test(NAME mod_battle_decide_sim${_p4_decide_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_battle_decide_sim_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-battle-decide-witness${_p4_decide_suffix}"
        --worker "$<TARGET_FILE:${_p4_decide_target}>")
    endif()
    set_tests_properties(mod_battle_decide_sim${_p4_decide_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 3600 SKIP_RETURN_CODE 77 RESOURCE_LOCK mod-battle-decide-witness)
  endforeach()
  unset(_p4_decide_pair)
  unset(_p4_decide_parts)
  unset(_p4_decide_count)
  unset(_p4_decide_target)
  unset(_p4_decide_suffix)
endif()
