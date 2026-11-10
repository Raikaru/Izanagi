# Host adapters stay outside the guest-pointer/EE-float C launchers. The shared
# consumer registry carries exact adapter usage requirements to cloned players.
add_subdirectory(tools/mods)
list(APPEND P4_HOST_LIB_ARGS ${P4_MOD_HOST_ARGS})
target_compile_definitions(p4port PRIVATE P4_MOD_GAME_BINDINGS=1)
if(P4PC_MOD_SUPPORT)
  target_compile_definitions(p4port PRIVATE P4_MOD_GUARDS=1)
endif()
if(P4_MOD_LUAU_ACTIVE)
  target_compile_definitions(p4port PRIVATE P4_MOD_LUAU=1)
endif()
