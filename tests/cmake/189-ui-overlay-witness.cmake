# Queue ownership fixture: a retained pack pointer and resource pin stand in
# for the independently qualified native sprite queue and raster backend.
add_executable(ui_overlay_lifetime_test ui_overlay_lifetime_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/config/ui_overlay_native.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(ui_overlay_lifetime_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(ui_overlay_lifetime_test m)
p4_ps2_exe(ui_overlay_lifetime_test)
foreach(_p4_case flush failed reregister cycles)
  add_test(NAME ui_overlay_lifetime_${_p4_case} COMMAND ui_overlay_lifetime_test ${_p4_case})
  set_tests_properties(ui_overlay_lifetime_${_p4_case} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()
unset(_p4_case)

# Real queue, synthetic sprite and pinned RW resource, on the no-device backend.
add_executable(ui_overlay_queue_test ui_overlay_queue_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(ui_overlay_queue_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/render")
target_compile_options(ui_overlay_queue_test PRIVATE -fno-builtin)
target_compile_definitions(ui_overlay_queue_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(ui_overlay_queue_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(ui_overlay_queue_test)
add_test(NAME ui_overlay_queue COMMAND ui_overlay_queue_test)
set_tests_properties(ui_overlay_queue PROPERTIES SKIP_RETURN_CODE 77 LABELS host TIMEOUT 120)

# Real game with the authored witness.ui-overlay fixture (p4.ui.sprite and
# p4.ui.overlay), explicitly selected only in the separately admitted witness
# lane. Never a retail gate or reference.
if(P4PC_MOD_WITNESS_TESTS)
  foreach(_p4_overlay_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_overlay_parts "${_p4_overlay_pair}")
    list(GET _p4_overlay_parts 0 _p4_overlay_target)
    list(LENGTH _p4_overlay_parts _p4_overlay_count)
    set(_p4_overlay_suffix "")
    if(_p4_overlay_count GREATER 1)
      list(GET _p4_overlay_parts 1 _p4_overlay_suffix)
    endif()
    if(NOT TARGET ${_p4_overlay_target})
      continue()
    endif()
    get_target_property(_p4_overlay_runner ${_p4_overlay_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_overlay_runner)
      set(_p4_overlay_runner "")
    endif()
    if(_p4_overlay_suffix STREQUAL "64w")
      set(_p4_overlay_runner "${P4_MOD_WINE}")
    endif()
    # The p4pc iteration's add_test already registers the 64-bit twin
    # (000-registration.cmake); this iteration only sets its properties.
    if(NOT TEST mod_overlay_game${_p4_overlay_suffix})
      add_test(NAME mod_overlay_game${_p4_overlay_suffix} COMMAND "${P4_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mod_overlay_game_test.py"
        --out "${CMAKE_BINARY_DIR}/mod-overlay-witness${_p4_overlay_suffix}"
        --driver ${_p4_overlay_runner} "$<TARGET_FILE:${_p4_overlay_target}>")
    endif()
    set_tests_properties(mod_overlay_game${_p4_overlay_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 2400 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_overlay_suffix STREQUAL "64w")
      set_property(TEST mod_overlay_game${_p4_overlay_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_overlay_pair)
  unset(_p4_overlay_parts)
  unset(_p4_overlay_count)
  unset(_p4_overlay_target)
  unset(_p4_overlay_suffix)
  unset(_p4_overlay_runner)
endif()
