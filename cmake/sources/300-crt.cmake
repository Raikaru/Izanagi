# Every compiled profile is guarded, even the compile-only 64-bit mirrors.
# Native adapters remain in the inventory. Host exceptions are reviewed by
# exact symbol/caller pair; targeted AST checks reject guest-result escapes.
set(P4_CRT_NATIVE "$<FILTER:$<TARGET_OBJECTS:p4port>,EXCLUDE,/(src/platform|third_party)/>"
                  "$<FILTER:$<TARGET_OBJECTS:p4portee>,EXCLUDE,/(src/platform|third_party)/>")
set(P4_CRT_FLAGS)
if(P4_WIN32)
  list(APPEND P4_CRT_FLAGS --coff32)
endif()
p4_crt_guard(i386 "$<TARGET_OBJECTS:p4decomp>;${P4_CRT_NATIVE}"
  "$<TARGET_OBJECTS:p4port>;$<TARGET_OBJECTS:p4portee>" ${P4_CRT_FLAGS} ${P4_HOST_LIB_ARGS} ${P4_MOD_CRT_PROJECT})
if(P4_MOD_CRT_DEP)
  add_custom_command(OUTPUT "${P4_CRT_i386_STAMP}" APPEND DEPENDS ${P4_MOD_CRT_DEP})
endif()
if(P4_PORT64_OBJS)
  p4_crt_guard(mirror64 "${P4_PORT64_OBJS}" "" ${P4_HOST_LIB_ARGS} ${P4_MOD_CRT_PROJECT64})
  if(P4_MOD_CRT_DEP64)
    add_custom_command(OUTPUT "${P4_CRT_mirror64_STAMP}" APPEND DEPENDS ${P4_MOD_CRT_DEP64})
  endif()
  add_dependencies(p4decomp64 p4crt_mirror64)
endif()
if(P4_PORT64W_OBJS)
  p4_crt_guard(mirror64w "${P4_PORT64W_OBJS}" "" ${P4_HOST_LIB_ARGS} ${P4_MOD_CRT_PROJECT64w})
  if(P4_MOD_CRT_DEP64w)
    add_custom_command(OUTPUT "${P4_CRT_mirror64w_STAMP}" APPEND DEPENDS ${P4_MOD_CRT_DEP64w})
  endif()
  add_dependencies(p4decomp64w p4crt_mirror64w)
endif()
