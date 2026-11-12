# The bundled seeded randomizer (mods/p4pc.randomizer) in the real game with the
# shipped package and the witness probe, explicitly selected only in the
# separately admitted witness lane, plus its many-seed corpus on the user's own
# disc banks, one randomized run played from New Game into Yukiko's Castle
# through its first boss (mod_randomizer_dungeon), and the run adopted on
# reconstructed second- and third-dungeon anchors (mod_randomizer_anchor, a
# disclosed test-only reconstruction). Never a retail gate or reference.
if(P4PC_MOD_WITNESS_TESTS AND TARGET luau_script_runner64)
  get_target_property(_p4_rando_runner_emulator luau_script_runner64 CROSSCOMPILING_EMULATOR)
  if(NOT _p4_rando_runner_emulator)
    set(_p4_rando_runner_emulator "")
  endif()
  if(NOT TEST mod_randomizer_corpus)
    add_test(NAME mod_randomizer_corpus COMMAND "${P4_PYTHON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_randomizer_engine_test.py" --corpus-only --seeds 1000
      -- ${_p4_rando_runner_emulator} "$<TARGET_FILE:luau_script_runner64>")
    set_tests_properties(mod_randomizer_corpus PROPERTIES LABELS "mod-witness" TIMEOUT 3600 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
  endif()
  foreach(_p4_rando_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_rando_parts "${_p4_rando_pair}")
    list(GET _p4_rando_parts 0 _p4_rando_target)
    list(LENGTH _p4_rando_parts _p4_rando_count)
    set(_p4_rando_suffix "")
    if(_p4_rando_count GREATER 1)
      list(GET _p4_rando_parts 1 _p4_rando_suffix)
    endif()
    if(NOT TARGET ${_p4_rando_target})
      continue()
    endif()
    get_target_property(_p4_rando_driver ${_p4_rando_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_rando_driver)
      set(_p4_rando_driver "")
    endif()
    if(_p4_rando_suffix STREQUAL "64w")
      set(_p4_rando_driver "${P4_MOD_WINE}")
    endif()
    if(NOT TEST mod_randomizer_game${_p4_rando_suffix})
      add_test(NAME mod_randomizer_game${_p4_rando_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_randomizer_game_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-randomizer-witness${_p4_rando_suffix}"
        --runner ${_p4_rando_runner_emulator} "$<TARGET_FILE:luau_script_runner64>"
        --driver ${_p4_rando_driver} "$<TARGET_FILE:${_p4_rando_target}>")
    endif()
    if(NOT TEST mod_randomizer_dungeon${_p4_rando_suffix})
      add_test(NAME mod_randomizer_dungeon${_p4_rando_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_randomizer_dungeon_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-randomizer-dungeon${_p4_rando_suffix}"
        --runner ${_p4_rando_runner_emulator} "$<TARGET_FILE:luau_script_runner64>"
        --driver ${_p4_rando_driver} "$<TARGET_FILE:${_p4_rando_target}>")
    endif()
    if(NOT TEST mod_randomizer_anchor${_p4_rando_suffix})
      add_test(NAME mod_randomizer_anchor${_p4_rando_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_randomizer_anchor_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-randomizer-anchor${_p4_rando_suffix}"
        --runner ${_p4_rando_runner_emulator} "$<TARGET_FILE:luau_script_runner64>"
        --driver ${_p4_rando_driver} "$<TARGET_FILE:${_p4_rando_target}>")
    endif()
    foreach(_p4_rando_test game dungeon anchor)
      set_tests_properties(mod_randomizer_${_p4_rando_test}${_p4_rando_suffix} PROPERTIES
        LABELS "mod-witness" TIMEOUT 7200 SKIP_RETURN_CODE 77
        ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
      if(_p4_rando_suffix STREQUAL "64w")
        set_property(TEST mod_randomizer_${_p4_rando_test}${_p4_rando_suffix} APPEND PROPERTY ENVIRONMENT
          WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
      endif()
    endforeach()
  endforeach()
  unset(_p4_rando_test)
  unset(_p4_rando_pair)
  unset(_p4_rando_parts)
  unset(_p4_rando_count)
  unset(_p4_rando_target)
  unset(_p4_rando_suffix)
  unset(_p4_rando_driver)
  unset(_p4_rando_runner_emulator)
endif()
