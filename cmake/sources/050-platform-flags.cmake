# Two targets: the Linux i386 build (clang -m32) and the 32-bit Windows build (llvm-mingw
# i686-w64-mingw32, selected by -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-win32.cmake). Everything
# that differs between them is in this block and src/platform/os (include/p4pc/os.h).
#
# _FILE_OFFSET_BITS must be set before any header (the -include below pulls in stdint.h,
# which fixes off_t), or CD offsets above 2 GiB truncate. Set it for every unit.
# Port-owned code is built without -malign-double. The decomp mirror gets it on p4decomp
# (below), since that is how the game lays out its structs. A port struct that mirrors a game
# struct with a 64-bit member uses P4_PS2_ALIGN64 (include/p4pc/align.h) and asserts offsets.
include("${CMAKE_CURRENT_LIST_DIR}/../reproducible.cmake")
p4_pe_reproducible_link_flags(P4_PE_REPRO_LINK_FLAGS)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(P4_WIN32 TRUE)
  # Directory policy reaches every executable, DLL and tool, not just players.
  add_link_options(${P4_PE_REPRO_LINK_FLAGS})
  enable_language(ASM)
  set(P4_TARGET_FLAGS -DP4_PC=1 -mfpmath=sse -msse2 -ffp-contract=off -D_FILE_OFFSET_BITS=64)
  set(P4_TARGET_LINK_FLAGS)
  set(P4_PORT_DEFINES _FILE_OFFSET_BITS=64)
  set(P4_OS_SRC "${CMAKE_SOURCE_DIR}/src/platform/os/os_win32.c")
  # The exe image owns the PS2 RAM range (src/platform/os/win32_ps2ram.S): image base 0xF0000 with
  # 64 KiB sections puts `.ps2ram`, forced to be the first section, at 0x00100000. No ASLR and no
  # base relocations, so the loader maps the exe exactly there or not at all. 8 MiB of stack, as
  # the Linux build has.
  file(WRITE "${CMAKE_BINARY_DIR}/ps2ram_layout.txt" ".ps2ram\n")
  set(P4_PS2RAM_LINK_FLAGS -Wl,--image-base=0xF0000 -Wl,--section-alignment=0x10000
    -Wl,--disable-dynamicbase -Wl,--disable-reloc-section -Wl,--stack=8388608
    "-Wl,--Xlink=/sectionlayout:${CMAKE_BINARY_DIR}/ps2ram_layout.txt")
  set(P4_PS2RAM_SRC "${CMAKE_SOURCE_DIR}/src/platform/os/win32_ps2ram.S")
  # pthread_mutex_* (src/movie/adx_dec.c): llvm-mingw's winpthreads linked statically (-Bstatic),
  # because the default would import libwinpthread-1.dll, which is one more DLL to ship.
  link_libraries(-Wl,-Bstatic winpthread -Wl,-Bdynamic)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
  # The NDK toolchain file (cmake/toolchain-android.cmake) sets the target triple and ANDROID_ABI. The game is a shared library, libmain.so,
  # that SDL's activity loads (src/platform/android/android_main.c); it is position independent, because Android refuses text
  # relocations (the Linux and Windows builds are not: they place PS2 RAM and the absolute data symbols at fixed addresses).
  set(P4_WIN32 FALSE)
  set(P4_ANDROID TRUE)
  # arm64-v8a and x86_64 are the 64-bit model (docs/port64.md): the rewritten mirror and the port compiled for the ABI, linked into the same libmain.so.
  # x86 is the i386 build. The Android build passes -DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 (and -DP4_PORT64_ARCH=aarch64 for arm64-v8a).
  if(ANDROID_ABI STREQUAL "arm64-v8a" OR ANDROID_ABI STREQUAL "x86_64")
    set(P4_ANDROID64 TRUE)
  endif()
  # NDK r27's clang 18 makes `return;` in a non-void function an error (-Wreturn-type); the decomp's flag files turn off the clang 20 spelling,
  # -Wno-return-mismatch, which this clang does not know.
  set(P4_TARGET_FLAGS -DP4_PC=1 -ffp-contract=off -D_FILE_OFFSET_BITS=64 -fPIC -Wno-error=return-type -Wno-return-type)
  if(ANDROID_ABI STREQUAL "x86")
    list(APPEND P4_TARGET_FLAGS -mfpmath=sse -msse2)
  endif()
  set(P4_TARGET_LINK_FLAGS)
  set(P4_PORT_DEFINES _FILE_OFFSET_BITS=64)
  set(P4_OS_SRC "${CMAKE_SOURCE_DIR}/src/platform/os/os_posix.c")
  set(P4_PS2RAM_LINK_FLAGS)
  set(P4_PS2RAM_SRC)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # macOS (docs/platform/macos.md): a 64-bit-only host (arm64 or x86_64, the machine's own), so a build directory builds only the 64-bit
  # model, like an AArch64 Linux one (P4_PORT64_ARCH, from the host). Compiled with the pinned clang of ci/macos/build_deps.sh against
  # the SDK CMake found; Mach-O code is position independent on both architectures. _FORTIFY_SOURCE=0: the SDK turns it on for optimised
  # builds, which would route the game's memcpy/strcpy through the checked __*_chk entry points instead of the reviewed byte seams.
  set(P4_WIN32 FALSE)
  set(P4_APPLE TRUE)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    set(P4_PORT64_ARCH "aarch64" CACHE STRING "architecture of the 64-bit build: x86_64 or aarch64 (cross; tests run under qemu-user)")
  else()
    set(P4_PORT64_ARCH "x86_64" CACHE STRING "architecture of the 64-bit build: x86_64 or aarch64 (cross; tests run under qemu-user)")
  endif()
  set(P4_TARGET_FLAGS -DP4_PC=1 -ffp-contract=off -D_FILE_OFFSET_BITS=64 -D_FORTIFY_SOURCE=0)
  # Mach-O has no undefined weak symbols: kernel_thread.c's weak kwlnTaskDumpAll, absent from the platform unit tests, stays a lookup at
  # load time that finds nothing (NULL), as it is on ELF; where the decomp defines it the link binds it as usual.
  set(P4_TARGET_LINK_FLAGS -Wl,-U,_kwlnTaskDumpAll)
  set(P4_PORT_DEFINES _FILE_OFFSET_BITS=64)
  set(P4_OS_SRC "${CMAKE_SOURCE_DIR}/src/platform/os/os_posix.c")
  set(P4_PS2RAM_LINK_FLAGS)
  set(P4_PS2RAM_SRC)
else()
  set(P4_WIN32 FALSE)
  set(P4_TARGET_FLAGS --target=i386-linux-gnu -m32 -DP4_PC=1 -mfpmath=sse -msse2 -ffp-contract=off
    -D_FILE_OFFSET_BITS=64)
  set(P4_TARGET_LINK_FLAGS --target=i386-linux-gnu -m32 -no-pie)
  # Host stat()/readdir() on a 32-bit build fail with EOVERFLOW on 64-bit-inode filesystems
  # (drvfs, ext4, xfs) unless the file and time APIs are 64-bit. Port-owned targets only: the
  # decomp mirror never calls host stat, so it keeps the default ABI.
  set(P4_PORT_DEFINES _FILE_OFFSET_BITS=64 _TIME_BITS=64)
  set(P4_OS_SRC "${CMAKE_SOURCE_DIR}/src/platform/os/os_posix.c")
  set(P4_PS2RAM_LINK_FLAGS)
  set(P4_PS2RAM_SRC)
endif()
# The renderer's GL API (src/render/p4gl_api.h): the OpenGL ES 3.0 subset (the default: Android natively, Mesa on Linux, ANGLE on Windows,
# macOS and iOS), or desktop OpenGL 3.3 core (-DP4PC_GLES=OFF). One source serves both; this sets the default and P4PC_GL_API=gl|gles
# overrides it for a run. docs/renderer_port.md section 10.
option(P4PC_GLES "Default to the OpenGL ES 3.0 renderer path instead of desktop OpenGL 3.3 core" ON)
if(P4PC_GLES)
  list(APPEND P4_PORT_DEFINES P4PC_GL_API_ES_DEFAULT=1)
endif()
# The renderer's context through the port's own EGL on a CAMetalLayer (src/render/p4gl_egl.c, ANGLE's Metal back end): what iOS needs, because SDL's
# UIKit video driver has no EGL. Off elsewhere; tools/ios_check.sh syntax-checks it for arm64 against the NDK's headers (no iOS SDK in CI).
option(P4PC_EGL_NATIVE "Create the GL ES context with the port's EGL on a CAMetalLayer (iOS)" OFF)
if(P4PC_EGL_NATIVE OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
  list(APPEND P4_PORT_DEFINES P4PC_GL_VIA_EGL=1)
endif()
if(P4_PORT64_ARCH STREQUAL "aarch64" AND NOT P4_ANDROID AND NOT P4_APPLE)
  # an AArch64 build directory builds only the 64-bit targets (P4_PORT64_ARCH below): the i386 flags (-m32, SSE) are not for them
  set(P4_TARGET_FLAGS -DP4_PC=1 -ffp-contract=off -D_FILE_OFFSET_BITS=64)
endif()
add_compile_options(${P4_TARGET_FLAGS})
# Optimisation: -O2 with line tables for every unit (the decomp units that name their own level, the
# RenderWare block at -O4, get -O3 after this, on their own command line). The decomp is m2c-typed C
# that punts pointers through integers and reads structs through byte pointers, so strict aliasing is off
# and signed overflow wraps (MWCC did both). Frame pointers stay, so the Windows crash log, gdb and the
# profilers can walk stacks. P4_OPT=O0 gives the unoptimised build for debugging.
set(P4_OPT "O2" CACHE STRING "optimisation level of every unit: O0, O1, O2, O3")
set(P4_DEBUG_INFO "g1" CACHE STRING "debug info of every unit: g (full), g1 (line tables), empty (none)")
add_compile_options(-${P4_OPT} -fno-strict-aliasing -fwrapv -fno-omit-frame-pointer)
if(P4_DEBUG_INFO)
  add_compile_options(-${P4_DEBUG_INFO})
endif()
include_directories("${CMAKE_SOURCE_DIR}/include")
# The forced include is C-only: the Windows build also assembles win32_ps2ram.S.
add_compile_options("$<$<COMPILE_LANGUAGE:C>:SHELL:-include p4pc/compat.h>")
if(NOT P4_ANDROID)
  add_compile_options(-fno-pic)
endif()
# P4_AUTO_INIT=pattern|zero: clang -ftrivial-auto-var-init for every unit, to find reads of uninitialised locals (a frame that differs
# between pattern and zero reads one; a CI run passes it as -DP4_AUTO_INIT).
set(P4_AUTO_INIT "" CACHE STRING "clang -ftrivial-auto-var-init mode (pattern or zero); empty: off")
set(P4_AUTO_INIT_ARGS)
if(P4_AUTO_INIT)
  add_compile_options(-ftrivial-auto-var-init=${P4_AUTO_INIT} -Wuninitialized -Wsometimes-uninitialized)
  set(P4_AUTO_INIT_ARGS "--auto-init=${P4_AUTO_INIT}")
endif()
if(P4_PORT64_ARCH STREQUAL "aarch64" AND NOT P4_ANDROID AND NOT P4_APPLE)
  set(P4_TARGET_LINK_FLAGS)   # (the i386 -m32 would turn an aarch64 link into a 32-bit ARM one)
endif()
add_link_options(${P4_TARGET_LINK_FLAGS})

