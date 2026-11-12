# The object list goes through a file: the tables step gets about 1000 paths, too many for one command line.
file(GENERATE OUTPUT "${P4_GEN}/objects.txt"
  CONTENT "$<JOIN:$<TARGET_OBJECTS:p4decomp>;$<TARGET_OBJECTS:p4port>;$<TARGET_OBJECTS:p4portee>,\n>\n")

set(P4_TABLES "${P4_GEN}/p4pc_tables.c")
add_custom_command(
  OUTPUT "${P4_TABLES}"
  COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/gen_port.py" tables
          --decomp "${P4_DECOMP}" --build "${P4_GEN}"
          --objects-file "${P4_GEN}/objects.txt"
          ${P4_RETAIL_ARGS} --out "${P4_TABLES}"
          ${P4_GEN_TOOL_ARGS} ${P4_HOST_LIB_ARGS} ${P4_MOD_CRT_PROJECT}
  # The objects themselves, not only the object list: a unit whose path is unchanged can still
  # gain or lose a definition (an override that replaces a decomp function), and then the
  # undefined-name set the tables are built from changes. gen_port.py writes the tables only when
  # their content changes and Ninja restats custom commands, so an unchanged result does not relink.
  DEPENDS p4decomp p4port p4portee $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>
    "${P4_CRT_i386_STAMP}"
    "${P4_GEN}/units.txt" "${P4_GEN}/objects.txt" "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
    "${CMAKE_SOURCE_DIR}/config/inactive_movie_decoder.json"
    "${CMAKE_SOURCE_DIR}/config/port_symbols.txt" "${CMAKE_SOURCE_DIR}/config/port_symbols_manual.txt"
    "${CMAKE_SOURCE_DIR}/config/port_owned.txt"
    "${CMAKE_SOURCE_DIR}/config/native_names.txt"
    "${CMAKE_SOURCE_DIR}/config/unbound_natives.txt" "${CMAKE_SOURCE_DIR}/include/p4pc/libc_rename.h"
    "${CMAKE_SOURCE_DIR}/config/call_traps.txt" "${CMAKE_SOURCE_DIR}/config/retail_fnptr_words.txt"
  VERBATIM
  COMMENT "Generating port tables")



