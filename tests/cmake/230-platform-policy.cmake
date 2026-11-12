# No test reads or writes a player's settings file (src/platform/config/p4cfg.h): the defaults are what the gates measure.
get_property(P4_ALL_TESTS DIRECTORY PROPERTY TESTS)
foreach(t IN LISTS P4_ALL_TESTS)
  set_property(TEST ${t} APPEND PROPERTY ENVIRONMENT "P4PC_CONFIG=none")
  get_property(P4_TEST_ENV TEST ${t} PROPERTY ENVIRONMENT)
  # Only the explicit virtual-controller fixture may use another isolated mode.
  if(NOT "${P4_TEST_ENV}" MATCHES "(^|;)P4PC_INPUT=virtual(;|$)")
    set_property(TEST ${t} APPEND PROPERTY ENVIRONMENT "P4PC_INPUT=script")
  endif()
endforeach()
# AArch64 under qemu-user (P4_PORT64_ARCH=aarch64) runs the game 5 to 30 times slower than the host: the time limits of the gates scale with it.
if(P4_PORT64_ARCH STREQUAL "aarch64")
  foreach(t IN LISTS P4_ALL_TESTS)
    get_property(P4_T_TIMEOUT TEST ${t} PROPERTY TIMEOUT)
    if(P4_T_TIMEOUT)
      math(EXPR P4_T_TIMEOUT "${P4_T_TIMEOUT} * 4")
      set_property(TEST ${t} PROPERTY TIMEOUT ${P4_T_TIMEOUT})
      # Save-play (also used by event scenes) has an inner watchdog; scale it with CTest's budget.
      set_property(TEST ${t} APPEND PROPERTY ENVIRONMENT "P4PC_GATE_TIMEOUT=${P4_T_TIMEOUT}")
    endif()
  endforeach()
endif()

