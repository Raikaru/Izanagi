# PS2 save conversion (tools/ps2save.py: .psu to and from the card folder layout).
find_program(P4_PYTHON NAMES python3 python)
if(P4_PYTHON AND NOT P4_WIN32)
  add_test(NAME ps2save_psu COMMAND ${P4_PYTHON} "${CMAKE_SOURCE_DIR}/tools/ps2save.py" selftest)
  # 64-bit seam gate (docs/port64.md 6.4): no port prototype that disagrees with its native's definition, no code address
  # taken as data. An empty baseline: a finding fails the land check.
  add_test(NAME seam_abi COMMAND ${P4_PYTHON} "${CMAKE_SOURCE_DIR}/tools/port64/seam_abi.py" --check)
endif()

