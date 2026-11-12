set(P4_GEN "${CMAKE_BINARY_DIR}/gen")
file(MAKE_DIRECTORY "${P4_GEN}")

file(GLOB_RECURSE P4_OVERRIDES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/overrides/*")

# The RenderWare clean layer switch (docs/rw_clean.md, config/rw_clean.json): the subsystems whose clean sources (src/rwclean) are compiled instead
# of the RenderWare units they replace. Empty (the default) builds the decomp's RenderWare exactly as before. A lane check selects subsystems with
# P4PC_CHECK_ENV="P4PC_RW_CLEAN=<ids>": the environment value, when set, wins over the cache at every configure.
set(P4_RW_CLEAN "" CACHE STRING "RenderWare clean subsystems to compile (config/rw_clean.json), comma-separated; empty: none")
if(DEFINED ENV{P4PC_RW_CLEAN})
  set(P4_RW_CLEAN "$ENV{P4PC_RW_CLEAN}" CACHE STRING "RenderWare clean subsystems to compile (config/rw_clean.json), comma-separated; empty: none" FORCE)
endif()
string(REPLACE ";" "," P4_RW_CLEAN_ARG "${P4_RW_CLEAN}")
file(GLOB_RECURSE P4_RW_CLEAN_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/rwclean/*.c" "${CMAKE_SOURCE_DIR}/src/rwclean/*.h")

# Configure-time inputs: the unit list and each unit's flags (gen_port.py units). CMake reruns
# when one of these changes; `cmake --build` does that by itself, and an unchanged tree skips
# configure entirely.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
  "${CMAKE_SOURCE_DIR}/config/units.txt"
  "${CMAKE_SOURCE_DIR}/config/excluded_units.txt"
  "${CMAKE_SOURCE_DIR}/config/inactive_movie_decoder.json"
  "${CMAKE_SOURCE_DIR}/config/rw_clean.json"
  "${P4_DECOMP}/compile_flags.txt"
  "${P4_DECOMP}/config/compiler_units.txt"
  "${P4_DECOMP}/config/version_flags.txt")

execute_process(
  COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/gen_port.py" units
          --decomp "${P4_DECOMP}" --build "${P4_GEN}" --rw-clean "${P4_RW_CLEAN_ARG}"
  RESULT_VARIABLE p4_rc
  OUTPUT_VARIABLE p4_out
  ERROR_VARIABLE p4_err)
message(STATUS "${p4_out}")
if(NOT p4_rc EQUAL 0)
  message(FATAL_ERROR "gen_port units failed:\n${p4_err}")
endif()
# P4_RW_CLEAN_SUBSYSTEMS and P4_RW_CLEAN_PORT_DROP (the port transcriptions a selected subsystem retires).
include("${P4_GEN}/rw_clean.cmake")

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

