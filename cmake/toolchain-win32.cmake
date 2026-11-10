# Cross toolchain for the 32-bit Windows build (i686-w64-mingw32, UCRT) with llvm-mingw.
#
#   cmake -S . -B build-win32 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-win32.cmake \
#         -DP4_WIN32_PREFIX=/opt/p4win32 -DP4_DECOMP=... -DP4_RETAIL=...
#
# P4PC_LLVM_MINGW (environment) is the llvm-mingw directory; /opt/llvm-mingw is the default, and
# a toolchain already on PATH is used when that directory does not exist. P4_WIN32_PREFIX holds
# SDL3 and FFmpeg built by ci/win32/build_deps.sh. See docs/platform/windows.md.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

if(DEFINED ENV{P4PC_LLVM_MINGW})
  set(_p4_llvm "$ENV{P4PC_LLVM_MINGW}")
else()
  set(_p4_llvm "/opt/llvm-mingw")
endif()
if(EXISTS "${_p4_llvm}/bin")
  set(_p4_bin "${_p4_llvm}/bin/")
else()
  set(_p4_bin "")
endif()

set(_p4_triple i686-w64-mingw32)
set(CMAKE_C_COMPILER "${_p4_bin}${_p4_triple}-clang")
set(CMAKE_ASM_COMPILER "${_p4_bin}${_p4_triple}-clang")
set(CMAKE_RC_COMPILER "${_p4_bin}${_p4_triple}-windres")
set(CMAKE_AR "${_p4_bin}llvm-ar")
set(CMAKE_RANLIB "${_p4_bin}llvm-ranlib")
# gen_port.py reads the objects with these (COFF names and relocations).
set(P4_NM "${_p4_bin}llvm-nm" CACHE FILEPATH "nm for the target objects")
set(P4_OBJDUMP "${_p4_bin}llvm-objdump" CACHE FILEPATH "objdump for the target objects")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
