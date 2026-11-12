# The mirror (gen_port.py mirror) writes every file of the decomp's src/ and include/ into
# ${P4_GEN}/decomp. All of them are outputs of one custom command, so a changed input reruns it.
# Write-if-changed keeps unchanged outputs' mtimes, so ninja rebuilds only the units whose files
# changed. Headers are outputs too: a changed header must dirty the units that include it.
file(GLOB_RECURSE P4_DECOMP_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false
  "${P4_DECOMP}/src/*" "${P4_DECOMP}/include/*")
set(P4_MIRROR_FILES ${P4_UNIT_SRCS})
foreach(f IN LISTS P4_DECOMP_FILES)
  string(REPLACE "${P4_DECOMP}/" "${P4_GEN}/decomp/" m "${f}")
  list(APPEND P4_MIRROR_FILES "${m}")
endforeach()
list(REMOVE_DUPLICATES P4_MIRROR_FILES)
# The mirror deliberately removes these decoder units. Listing a removed file as
# an output makes Ninja rerun the entire mirror on every incremental build.
file(READ "${CMAKE_SOURCE_DIR}/config/inactive_movie_decoder.json" P4_INACTIVE_MOVIE_POLICY)
string(JSON P4_INACTIVE_MOVIE_COUNT LENGTH "${P4_INACTIVE_MOVIE_POLICY}" units)
if(P4_INACTIVE_MOVIE_COUNT GREATER 0)
  math(EXPR P4_INACTIVE_MOVIE_LAST "${P4_INACTIVE_MOVIE_COUNT} - 1")
  foreach(i RANGE ${P4_INACTIVE_MOVIE_LAST})
    string(JSON unit GET "${P4_INACTIVE_MOVIE_POLICY}" units ${i})
    list(REMOVE_ITEM P4_MIRROR_FILES "${P4_GEN}/decomp/${unit}")
  endforeach()
endif()
# A selected RenderWare clean subsystem: the mirror also writes src/rwclean (its units are in P4_UNIT_SRCS; its headers are
# outputs too, so a changed header dirties the clean units in the same build).
if(P4_RW_CLEAN_SUBSYSTEMS)
  foreach(f IN LISTS P4_RW_CLEAN_FILES)
    string(REPLACE "${CMAKE_SOURCE_DIR}/" "${P4_GEN}/decomp/" m "${f}")
    list(APPEND P4_MIRROR_FILES "${m}")
  endforeach()
  list(REMOVE_DUPLICATES P4_MIRROR_FILES)
endif()
set(P4_DECOMP_SYMBOL_FILES
  "${P4_DECOMP}/config/symbol_addrs.txt"
  "${P4_DECOMP}/config/symbol_data_addrs.txt"
  "${P4_DECOMP}/config/symbols_recovered.txt")
add_custom_command(
  OUTPUT ${P4_MIRROR_FILES} "${P4_MOD_BRIDGES}"
  COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/gen_port.py" mirror
          --decomp "${P4_DECOMP}" --build "${P4_GEN}" ${P4_MOD_MIRROR_ARGS}
  DEPENDS ${P4_DECOMP_FILES} ${P4_DECOMP_SYMBOL_FILES} ${P4_OVERRIDES}
    "${CMAKE_SOURCE_DIR}/config/units.txt" "${CMAKE_SOURCE_DIR}/config/excluded_units.txt"
    "${CMAKE_SOURCE_DIR}/config/inactive_movie_decoder.json"
    "${CMAKE_SOURCE_DIR}/config/rw_clean.json" "${P4_GEN}/rw_clean.json" ${P4_RW_CLEAN_FILES}
    "${CMAKE_SOURCE_DIR}/config/port_owned.txt" "${CMAKE_SOURCE_DIR}/config/port_symbols.txt" "${CMAKE_SOURCE_DIR}/config/port_symbols_manual.txt"
    "${CMAKE_SOURCE_DIR}/config/mirror_fixes.txt" "${CMAKE_SOURCE_DIR}/config/native_names.txt"
    "${CMAKE_SOURCE_DIR}/config/mirror_contracts.json" "${CMAKE_SOURCE_DIR}/config/unbound_natives.txt"
    "${CMAKE_SOURCE_DIR}/overrides/CLAIMS.md"
    "${CMAKE_SOURCE_DIR}/config/passthrough_calls.txt" "${CMAKE_SOURCE_DIR}/config/passthrough_calls_manual.txt"
    "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
    "${CMAKE_SOURCE_DIR}/tools/mod_codegen.py"
    "${CMAKE_SOURCE_DIR}/include/p4pc/libc_rename.h"
  VERBATIM
  COMMENT "Mirroring the decomp (gen_port mirror)")
# Named target for targets outside this file that include the mirror's headers directly.
add_custom_target(p4_mirror DEPENDS ${P4_MIRROR_FILES} "${P4_MOD_BRIDGES}")

