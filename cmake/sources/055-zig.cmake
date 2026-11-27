# The battle simulator (src/platform/battle_sim/*.zig, 187-battle-sim.cmake) is Zig, compiled by the pinned Zig of cmake/zig/pin.cmake
# as a CMake language of its own (cmake/zig/): a .zig source is an ordinary source of its target, so its object is in
# $<TARGET_OBJECTS:...> for the tables (gen_port), the CRT audits and every link, and the 64-bit profiles that copy p4port's SOURCES
# (350-port64-player.cmake, 360-port64-win.cmake) compile it for their own target. Each object is compiled for the target its C units
# are (the last --target and -m32/-m64 of its compile options; otherwise the C compiler's own target), with the same definitions and
# include directories, so `zig translate-c` of its binding header (src/platform/battle_sim/bindings/<root>.h, the root's c_bindings
# module) sees the profile's headers and pointer width (include/p4pc/guest.h: P4_GUEST64 on 64-bit targets).
# No Zig: configuring fails. There is no C fallback.
include("${CMAKE_SOURCE_DIR}/cmake/zig/pin.cmake")
set(P4_ZIG "" CACHE FILEPATH "Zig ${P4_ZIG_VERSION} executable (empty: \$ENV{P4_ZIG}, then zig on PATH; ci/zig/fetch_zig.sh installs it)")
set(P4_ZIG_OPTIMIZE "auto" CACHE STRING
  "Zig build mode of the battle simulator: auto (debug for -DP4_OPT=O0, safe otherwise), debug, safe, fast, small")
set_property(CACHE P4_ZIG_OPTIMIZE PROPERTY STRINGS auto debug safe fast small)
if(NOT P4_ZIG_OPTIMIZE MATCHES "^(auto|debug|safe|fast|small)$")
  message(FATAL_ERROR "P4_ZIG_OPTIMIZE must be auto, debug, safe, fast or small (Zig ${P4_ZIG_VERSION} -O names), not "
    "'${P4_ZIG_OPTIMIZE}'")
endif()
if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows|Android|Darwin)$")
  message(FATAL_ERROR "the Zig battle simulator has no target mapping for ${CMAKE_SYSTEM_NAME} (cmake/zig/zig_object.py: Linux glibc, "
    "mingw-w64, Android, macOS)")
endif()

# The target of a unit whose options name none: the C compiler's own (the NDK and the macOS configure name it; llvm-mingw's and the
# host clang's report theirs).
if(CMAKE_C_COMPILER_TARGET)
  set(P4_ZIG_DEFAULT_TARGET "${CMAKE_C_COMPILER_TARGET}")
else()
  execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-target-triple
    OUTPUT_VARIABLE P4_ZIG_DEFAULT_TARGET OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _p4_zig_rc)
  if(NOT _p4_zig_rc EQUAL 0 OR NOT P4_ZIG_DEFAULT_TARGET)
    message(FATAL_ERROR "${CMAKE_C_COMPILER} -print-target-triple failed: the Zig objects need the C compiler's target")
  endif()
  unset(_p4_zig_rc)
endif()
# Android: Zig ships no bionic; the objects use the NDK sysroot's headers through a libc installation file.
set(P4_ZIG_ANDROID_SYSROOT)
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  if(CMAKE_SYSROOT)
    set(P4_ZIG_ANDROID_SYSROOT "${CMAKE_SYSROOT}")
  elseif(ANDROID_TOOLCHAIN_ROOT)
    set(P4_ZIG_ANDROID_SYSROOT "${ANDROID_TOOLCHAIN_ROOT}/sysroot")
  endif()
  if(NOT P4_ZIG_ANDROID_SYSROOT OR NOT EXISTS "${P4_ZIG_ANDROID_SYSROOT}/usr/include/stdio.h")
    message(FATAL_ERROR "the Android Zig objects need the NDK sysroot (CMAKE_SYSROOT); found '${P4_ZIG_ANDROID_SYSROOT}'")
  endif()
endif()

# The compiler, resolved and checked at every configure: -DP4_ZIG, then P4_ZIG in the environment (the CI images set it), then the
# macOS dependency prefix's version-named install (ci/macos/build_deps.sh: <prefix>/zig-<version>), then PATH. Overriding it needs no
# image rebuild. The search result is not kept from an earlier configure, so a build directory first configured with an older pinned
# Zig finds the current one instead of failing on the cached old path.
if(P4_ZIG)
  set(P4_ZIG_EXECUTABLE "${P4_ZIG}")
elseif(DEFINED ENV{P4_ZIG} AND NOT "$ENV{P4_ZIG}" STREQUAL "")
  set(P4_ZIG_EXECUTABLE "$ENV{P4_ZIG}")
else()
  set(_p4_zig_hints)
  if(P4_MACOS_PREFIX)
    list(APPEND _p4_zig_hints "${P4_MACOS_PREFIX}/zig-${P4_ZIG_VERSION}")
  endif()
  unset(P4_ZIG_FOUND CACHE)
  find_program(P4_ZIG_FOUND NAMES zig HINTS ${_p4_zig_hints} NO_CMAKE_FIND_ROOT_PATH)
  set(P4_ZIG_EXECUTABLE "${P4_ZIG_FOUND}")
  unset(_p4_zig_hints)
endif()
if(NOT P4_ZIG_EXECUTABLE OR NOT EXISTS "${P4_ZIG_EXECUTABLE}")
  message(FATAL_ERROR "Zig ${P4_ZIG_VERSION} not found ('${P4_ZIG_EXECUTABLE}'): the battle simulator is Zig and has no C fallback. "
    "Install it with `sh ci/zig/fetch_zig.sh <dir>` (checksum-pinned, cmake/zig/pin.cmake) and pass -DP4_ZIG=<dir>/zig.")
endif()
execute_process(COMMAND "${P4_ZIG_EXECUTABLE}" version
  OUTPUT_VARIABLE _p4_zig_version OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _p4_zig_rc)
if(NOT _p4_zig_rc EQUAL 0 OR NOT _p4_zig_version STREQUAL P4_ZIG_VERSION)
  message(FATAL_ERROR "${P4_ZIG_EXECUTABLE} is Zig '${_p4_zig_version}', not the pinned ${P4_ZIG_VERSION} (cmake/zig/pin.cmake). "
    "Install the pinned release with `sh ci/zig/fetch_zig.sh <dir>` and pass -DP4_ZIG=<dir>/zig.")
endif()
unset(_p4_zig_version)
unset(_p4_zig_rc)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/zig")
enable_language(Zig)
message(STATUS "Zig ${P4_ZIG_VERSION}: ${P4_ZIG_EXECUTABLE} (default target ${P4_ZIG_DEFAULT_TARGET}, mode ${P4_ZIG_OPTIMIZE})")
