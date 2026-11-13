# Real game reading the p4.data catalog from a private script package, checked
# against an independent decode of the same disc. Explicitly selected only in
# the separately admitted witness lane; never a retail gate or reference.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_data_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_data_parts "${_p4_data_pair}")
    list(GET _p4_data_parts 0 _p4_data_target)
    list(LENGTH _p4_data_parts _p4_data_count)
    set(_p4_data_suffix "")
    if(_p4_data_count GREATER 1)
      list(GET _p4_data_parts 1 _p4_data_suffix)
    endif()
    if(NOT TARGET ${_p4_data_target})
      continue()
    endif()
    get_target_property(_p4_data_runner ${_p4_data_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_data_runner)
      set(_p4_data_runner "")
    endif()
    if(_p4_data_suffix STREQUAL "64w")
      set(_p4_data_runner "${P4_MOD_WINE}")
    endif()
    # The p4pc iteration's add_test already registers the 64-bit twin
    # (000-registration.cmake); this iteration only sets its properties.
    # mod_data_game reads the catalog; mod_data_edit_game edits it first.
    foreach(_p4_data_test mod_data_game mod_data_edit_game)
      if(NOT TEST ${_p4_data_test}${_p4_data_suffix})
        add_test(NAME ${_p4_data_test}${_p4_data_suffix} COMMAND "${P4_PYTHON}"
          "${CMAKE_CURRENT_SOURCE_DIR}/${_p4_data_test}_test.py"
          --out "${CMAKE_BINARY_DIR}/${_p4_data_test}-witness${_p4_data_suffix}"
          --driver ${_p4_data_runner} "$<TARGET_FILE:${_p4_data_target}>")
      endif()
      set_tests_properties(${_p4_data_test}${_p4_data_suffix} PROPERTIES
        LABELS "mod-witness" TIMEOUT 1800 SKIP_RETURN_CODE 77
        ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
      if(_p4_data_suffix STREQUAL "64w")
        set_property(TEST ${_p4_data_test}${_p4_data_suffix} APPEND PROPERTY ENVIRONMENT
          WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
      endif()
    endforeach()
  endforeach()
  unset(_p4_data_pair)
  unset(_p4_data_test)
  unset(_p4_data_parts)
  unset(_p4_data_count)
  unset(_p4_data_target)
  unset(_p4_data_suffix)
  unset(_p4_data_runner)
endif()
