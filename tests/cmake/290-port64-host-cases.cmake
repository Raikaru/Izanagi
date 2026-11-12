# Finish after every reference fixture and source-test policy, including late
# tools and lockstep cases. Keep the two build aggregates architecture-specific.
if(P4_PORT64_LINK AND P4_PORT64_OBJS)
  p4_host64_tests(64)
  if(TARGET p4pc64w)
    if(TEST port64_coro_win)
      set_property(TEST port64_coro_win APPEND PROPERTY LABELS port64win)
      set_tests_properties(port64_coro_win PROPERTIES FIXTURES_SETUP wine64_ready)
    endif()
    p4_host64_tests(64w)
    # The Wine budget of every Windows x64 test (cmake/wine-tests.cmake).
    add_test(NAME wine_timeouts64w COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/wine_timeout_test.py"
      "${CMAKE_BINARY_DIR}" "${P4_WINE_TIMEOUT_SCALE}" "${CMAKE_CTEST_COMMAND}")
    set_tests_properties(wine_timeouts64w PROPERTIES LABELS "host;port64win" TIMEOUT 60)
  endif()
  get_property(P4_HOST64_EXES GLOBAL PROPERTY P4_HOST64_EXES)
  list(REMOVE_DUPLICATES P4_HOST64_EXES)
  foreach(t IN LISTS P4_HOST64_EXES)
    add_dependencies(p4_x64 ${t})
  endforeach()
  get_property(P4_HOST64W_EXES GLOBAL PROPERTY P4_HOST64W_EXES)
  list(REMOVE_DUPLICATES P4_HOST64W_EXES)
  foreach(t IN LISTS P4_HOST64W_EXES)
    add_dependencies(p4_x64w ${t})
  endforeach()
endif()
# The Win32 build's tests run under Wine as well (the land's win32 shard), with the same per-process start
# cost as the Windows x64 build's: every test with its own TIMEOUT gets the Wine scale (cmake/wine-tests.cmake).
# 2026-11-03..06: package_registry (TIMEOUT 30, 23 process starts) timed out at 30 s under load in three Win32
# shards whose members were not at fault (tools/land_flakes.py report). Keep this after every registration.
if(P4_WIN32)
  include("${CMAKE_SOURCE_DIR}/cmake/wine-tests.cmake")
  get_property(_p4_win32_tests DIRECTORY PROPERTY TESTS)
  p4_wine_scale_timeouts(${_p4_win32_tests})
  unset(_p4_win32_tests)
endif()
