# Never registered by default retail/PS2 comparison gates. These deliberate
# synthetic mod executions are isolated from reference-producing harnesses.
option(P4PC_MOD_WITNESS_TESTS "Register isolated MOD_WITNESS recovery tests (not retail gates)" OFF)
if(P4PC_MOD_WITNESS_TESTS)
  if(ANDROID OR NOT TARGET mod_luau_test)
    message(FATAL_ERROR "MOD_WITNESS tests require the desktop Luau game binding and native VM fixture")
  endif()
  foreach(_p4_recovery_suffix "" 64 64w)
    if(NOT TARGET mod_luau_test${_p4_recovery_suffix})
      continue()
    endif()
    set(_p4_recovery_sdl "${P4_SDL3_LIB}")
    if(_p4_recovery_suffix STREQUAL "64")
      set(_p4_recovery_sdl "${P4_SDL3_LIB64}")
    elseif(_p4_recovery_suffix STREQUAL "64w")
      set(_p4_recovery_sdl "${P4W_SDL3}")
    endif()
    if(NOT _p4_recovery_sdl OR _p4_recovery_sdl MATCHES "-NOTFOUND$")
      message(FATAL_ERROR "MOD_WITNESS profile${_p4_recovery_suffix} needs its native SDL link profile, not a compile-only ABI")
    endif()
    # Reuse the production VM/kernel/provider fixture. Do not mock the native
    # error backend, atomic INI writer or the fresh-image OS restart boundary.
    target_sources(mod_luau_test${_p4_recovery_suffix} PRIVATE
      "${CMAKE_SOURCE_DIR}/src/platform/mods/failure_native.c"
      "${CMAKE_SOURCE_DIR}/src/platform/mods/failure_config.c"
      "${CMAKE_SOURCE_DIR}/src/platform/mods/mods_config.c"
      "${CMAKE_SOURCE_DIR}/src/platform/mods/boot_health.c"
      "${CMAKE_SOURCE_DIR}/src/platform/mods/package_boot.c"
      "${CMAKE_SOURCE_DIR}/src/crt.c"
      "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c"
      "${CMAKE_SOURCE_DIR}/src/platform/os/restart.c")
    target_compile_definitions(mod_luau_test${_p4_recovery_suffix} PRIVATE P4_MOD_NATIVE_RECOVERY=1)
    target_link_libraries(mod_luau_test${_p4_recovery_suffix} PRIVATE ${_p4_recovery_sdl})
    # Actual CRT/boot modules supply the observer and public recovery bridge.
    # Unused game-entry/completion sections are not pretend fixture providers.
    target_compile_options(mod_luau_test${_p4_recovery_suffix} PRIVATE -ffunction-sections -fdata-sections)
    get_target_property(_p4_recovery_runner mod_luau_test${_p4_recovery_suffix} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_recovery_runner)
      set(_p4_recovery_runner "")
    endif()
    if(_p4_recovery_suffix STREQUAL "64w")
      set(_p4_recovery_runner "${P4_MOD_WINE}")
    endif()
    add_test(NAME mod_loader_recovery${_p4_recovery_suffix} COMMAND "${P4_PYTHON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_loader_recovery_test.py"
      ${_p4_recovery_runner} "$<TARGET_FILE:mod_luau_test${_p4_recovery_suffix}>" --driver-end)
    set_tests_properties(mod_loader_recovery${_p4_recovery_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 180
      ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script;SDL_AUDIODRIVER=dummy")
    if(_p4_recovery_suffix STREQUAL "64")
      set_property(TEST mod_loader_recovery${_p4_recovery_suffix} APPEND PROPERTY LABELS port64)
    elseif(_p4_recovery_suffix STREQUAL "64w")
      set_tests_properties(mod_loader_recovery${_p4_recovery_suffix} PROPERTIES FIXTURES_REQUIRED wine64_ready)
      set_property(TEST mod_loader_recovery${_p4_recovery_suffix} APPEND PROPERTY LABELS port64win)
      set_property(TEST mod_loader_recovery${_p4_recovery_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  # Real game + mod example, explicitly selected only in a separately admitted
  # witness lane. It writes a private copied card and never produces references.
  foreach(_p4_example_pair "p4pc|" "p4pc64|64" "p4pc64w|64w")
    string(REPLACE "|" ";" _p4_example_parts "${_p4_example_pair}")
    list(GET _p4_example_parts 0 _p4_example_target)
    list(LENGTH _p4_example_parts _p4_example_count)
    set(_p4_example_suffix "")
    if(_p4_example_count GREATER 1)
      list(GET _p4_example_parts 1 _p4_example_suffix)
    endif()
    if(NOT TARGET ${_p4_example_target})
      continue()
    endif()
    get_target_property(_p4_example_runner ${_p4_example_target} CROSSCOMPILING_EMULATOR)
    if(NOT _p4_example_runner)
      set(_p4_example_runner "")
    endif()
    if(_p4_example_suffix STREQUAL "64w")
      set(_p4_example_runner "${P4_MOD_WINE}")
    endif()
    _add_test(NAME mod_example_game${_p4_example_suffix} COMMAND "${P4_PYTHON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/mod_example_game_test.py"
      --out "${CMAKE_BINARY_DIR}/mod-example-witness${_p4_example_suffix}"
      --driver ${_p4_example_runner} "$<TARGET_FILE:${_p4_example_target}>")
    set_tests_properties(mod_example_game${_p4_example_suffix} PROPERTIES
      LABELS "mod-witness" TIMEOUT 4800 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=1;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_MOD_WITNESS=1")
    if(_p4_example_suffix STREQUAL "64w")
      set_property(TEST mod_example_game${_p4_example_suffix} APPEND PROPERTY ENVIRONMENT
        WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    endif()
  endforeach()
  unset(_p4_example_pair)
  unset(_p4_example_parts)
  unset(_p4_example_count)
  unset(_p4_example_target)
  unset(_p4_example_suffix)
  unset(_p4_example_runner)
  unset(_p4_recovery_suffix)
  unset(_p4_recovery_sdl)
  unset(_p4_recovery_runner)
endif()
