# Host OS shim (include/p4pc/os.h): os_posix.c or os_win32.c.
target_sources(p4port PRIVATE "${P4_OS_SRC}" src/platform/os/restart.c)
if(P4_ANDROID)
  target_sources(p4port PRIVATE src/platform/os/coro_asm.c)
endif()

