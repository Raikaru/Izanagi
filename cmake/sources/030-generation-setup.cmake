set(P4_GEN "${CMAKE_BINARY_DIR}/gen")
file(MAKE_DIRECTORY "${P4_GEN}")

file(GLOB_RECURSE P4_OVERRIDES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/overrides/*")

# Configure-time inputs: the unit list and each unit's flags (gen_port.py units). CMake reruns
# when one of these changes; `cmake --build` does that by itself, and an unchanged tree skips
# configure entirely.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
  "${CMAKE_SOURCE_DIR}/config/units.txt"
  "${CMAKE_SOURCE_DIR}/config/excluded_units.txt"
  "${CMAKE_SOURCE_DIR}/config/inactive_movie_decoder.json"
  "${P4_DECOMP}/compile_flags.txt"
  "${P4_DECOMP}/config/compiler_units.txt"
  "${P4_DECOMP}/config/version_flags.txt")

execute_process(
  COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/gen_port.py" units
          --decomp "${P4_DECOMP}" --build "${P4_GEN}"
  RESULT_VARIABLE p4_rc
  OUTPUT_VARIABLE p4_out
  ERROR_VARIABLE p4_err)
message(STATUS "${p4_out}")
if(NOT p4_rc EQUAL 0)
  message(FATAL_ERROR "gen_port units failed:\n${p4_err}")
endif()

# Each unit line is "<mirror path>|<flags>"; flags are tab-separated (paths may contain spaces).
file(STRINGS "${P4_GEN}/units.txt" P4_UNIT_LINES)
set(P4_UNIT_SRCS)
foreach(line IN LISTS P4_UNIT_LINES)
  string(FIND "${line}" "|" bar)
  string(SUBSTRING "${line}" 0 ${bar} unit)
  math(EXPR rest "${bar} + 1")
  string(SUBSTRING "${line}" ${rest} -1 flags)
  string(REPLACE "\t" ";" flags "${flags}")
  set_source_files_properties("${unit}" PROPERTIES COMPILE_OPTIONS "${flags}")
  list(APPEND P4_UNIT_SRCS "${unit}")
endforeach()

