/* The C declarations of live.zig (its `c_bindings` module): cmake/zig/zig_object.py runs `zig translate-c` on this header
   with the object's own target, libc, definitions and include directories. Angle includes, as @cInclude was. */
/* The fork worker's POSIX surface: Linux other than Android only (live.zig battle_fork_enabled, C P4_BATTLE_FORK);
   elsewhere an empty translation unit. */
#if defined(__linux__) && !defined(__ANDROID__)
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
