# Root-only scalar guard/generation configuration. VM/binding sources are built
# in tools/mods, outside guest-pointer/EE-float C launchers and forced includes.
option(P4PC_MOD_SUPPORT "Compile opt-in typed mod entry guards" ON)
set(P4_MOD_BRIDGES "${P4_GEN}/mod_bridges.c")
set(P4_MOD_MIRROR_ARGS)
if(P4PC_MOD_SUPPORT)
  list(APPEND P4_MOD_MIRROR_ARGS --mod-hooks)
endif()
set(P4_MOD_LUAU_ACTIVE FALSE)
if(P4PC_MOD_SUPPORT AND P4PC_LUAU)
  set(P4_MOD_LUAU_ACTIVE TRUE)
endif()
set(P4_MOD_CORE_SOURCES
  "${CMAKE_SOURCE_DIR}/src/platform/mods/hooks.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/events.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/failure.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/numbers.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/memory_policy.c")

# Trusted factory bytes cannot be replaced by a package or a Lua filesystem API.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/platform/mods/layout_api.luau")
file(READ "${CMAKE_SOURCE_DIR}/src/platform/mods/layout_api.luau" _p4_layout)
string(REPLACE "\\" "\\\\" _p4_layout "${_p4_layout}")
string(REPLACE "\"" "\\\"" _p4_layout "${_p4_layout}")
string(REPLACE "\r" "" _p4_layout "${_p4_layout}")
string(REPLACE "\n" "\\n\"\n\"" _p4_layout "${_p4_layout}")
file(CONFIGURE OUTPUT "${P4_GEN}/mod_luau_api.h"
  CONTENT "static const char p4_mod_layout_api[] =\n\"${_p4_layout}\";\n" @ONLY)
unset(_p4_layout)

# One producer, shared by scalar-only and real-VM witnesses on every native ABI.
set(P4_MOD_TEST_DIR "${P4_GEN}/mod_hook_fixture")
add_custom_command(
  OUTPUT "${P4_MOD_TEST_DIR}/game.c" "${P4_MOD_TEST_DIR}/mod_bridges.c"
  COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tests/mod_codegen_fixture.py" "${P4_MOD_TEST_DIR}"
  DEPENDS "${CMAKE_SOURCE_DIR}/tests/mod_codegen_fixture.py"
    "${CMAKE_SOURCE_DIR}/tests/mod_hooks_game.c"
    "${CMAKE_SOURCE_DIR}/tools/mod_codegen.py" "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
    "${CMAKE_SOURCE_DIR}/include/p4pc/libc_rename.h"
  VERBATIM)
set(P4_MOD_TEST_GAME "${P4_MOD_TEST_DIR}/game.c" "${P4_MOD_TEST_DIR}/mod_bridges.c")
add_custom_target(p4_mod_hook_fixture DEPENDS ${P4_MOD_TEST_GAME})
