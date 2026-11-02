# Windows reference build

[Platform index](../platform.md) · [64-bit default and Windows x64](../port64.md)

This page covers the **i386 nightly reference** (`p4pc.exe`), not the x64 player package.
The shared host API is `include/p4pc/os.h`, implemented by `os_posix.c` and `os_win32.c`.
Port code/tests use that seam rather than direct platform calls.

## Host services and fixed addresses

The reference ABI uses ucontext on Linux and `CreateFiberEx` fibers on Windows, with
host-owned stacks of at least 256 KiB. Time uses QueryPerformanceCounter and a
high-resolution waitable timer. File/image loading uses stdio and shared local ELF
structures. OS mkdir/setenv/usleep wrappers hide platform differences.

Windows disables error dialogs and installs a vectored exception handler that logs the
exception, PC, registers and backtrace, then exits 139. The main stack is 8 MiB.
Fibers share [guest float state](numeric_determinism.md), not default per-fiber float state.

Reference guest RAM must occupy `[0x00100000, 0x02000000)`. Reserving it at runtime is
unsafe because startup heaps/stacks/NLS mappings can already occupy it. Instead,
`src/platform/os/win32_ps2ram.S` supplies a zero-filled writable/executable `0x1f00000`
section. The image links at `0xf0000`, section alignment `0x10000`, without ASLR/base
relocations. lld `/sectionlayout` places `.ps2ram` first at `0x00100000`.
`p4_os_map_fixed` verifies placement with VirtualQuery and executable permissions.
`p4_ps2_exe()` applies this only to exes/tests using fixed guest memory; unrelated tools
need not carry 31 MiB. The EE register page uses VirtualAlloc.

## Toolchain and symbols

`ci/Dockerfile` pins llvm-mingw 20261006 (clang/lld 23.1.3, UCRT).
`ci/win32/build_deps.sh <prefix> [llvm-mingw-dir]` builds pinned SHA-checked SDL3 3.2.10
and LGPL shared FFmpeg 7.1.1: MPEG-2 decoder/parser, ADX decoder, swscale and avutil.
Changing `FFMPEG_OPTS` invalidates its dependency build stamp. Decomp pointer-type
warnings remain warnings through `-Wno-error=incompatible-pointer-types`, not suppression
of the diagnostics or reliance on -O0.

i686 COFF cdecl names have a leading underscore. `gen_port.py tables --coff --nm llvm-nm
--objdump llvm-objdump` reads COFF calls (`IMAGE_REL_I386_REL32`) and host imports,
uses the UCRT/MinGW/compiler-rt/SDL/FFmpeg host-symbol sets, and emits COFF absolute data
and jump thunks. Explicit asm-named symbols may be resolved to their prefixed form by
lld's MinGW driver. This is a symbol-format distinction, not a different game ABI.

## Building and running

Cross-build from Linux/WSL/the CI image:

```sh
sh ci/win32/build_deps.sh /opt/p4win32 /opt/llvm-mingw
P4PC_LLVM_MINGW=/opt/llvm-mingw cmake -S . -B build-win32 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-win32.cmake -DP4_WIN32_PREFIX=/opt/p4win32 \
  -DP4_DECOMP=decomp -DP4_RETAIL=/path/to/SLUS_217.82
cmake --build build-win32
```

Ship/run the executable beside `SDL3.dll`, `avcodec-61.dll`, `avutil-59.dll` and
`swscale-8.dll`. They also copy into `build-win32/tests/`. Host CTests run under Wine
with `-DCMAKE_CROSSCOMPILING_EMULATOR=wine`; GL-labeled tests require a real context
and are not qualified by container Wine. Land builds x64; nightlies retain this ABI.

For automated native Windows runs use the local lease/scene/boot runner with
`P4PC_HIDDEN_WINDOW=1`, `SDL_AUDIODRIVER=dummy` and `P4PC_INPUT=script`.
A hidden WGL window provides a context without desktop UI. SDL's offscreen EGL driver
is not the Windows headless solution, and its dummy video driver has no GL.
See [testing](testing.md#headless-execution) and [contribution rules](../../CONTRIBUTING.md).
