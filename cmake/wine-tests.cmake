include_guard(GLOBAL)
# Tests of the Windows x64 build run under Wine, where every process start costs
# roughly 0.3 to 1 s against a few milliseconds natively. A driver that starts its
# exe once per case pays that per case: package_registry (23 starts) takes 1.1 s
# natively, 6 to 14 s under Wine, and over 30 s on a loaded host, with no process
# stuck. Its native TIMEOUT is therefore not a Wine budget; the 64w twin of a test
# gets the native TIMEOUT times this scale. Tests without their own TIMEOUT keep the
# CTest default the suite passes.
set(P4_WINE_TIMEOUT_SCALE 4 CACHE STRING "TIMEOUT multiplier for tests of the Windows x64 build run under Wine")

function(p4_wine_scale_timeouts)
  foreach(test IN LISTS ARGN)
    get_test_property(${test} TIMEOUT seconds)
    if(seconds)
      math(EXPR seconds "${seconds} * ${P4_WINE_TIMEOUT_SCALE}")
      set_tests_properties(${test} PROPERTIES TIMEOUT ${seconds})
    endif()
  endforeach()
endfunction()
