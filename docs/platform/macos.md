# macOS

[Platform index](../platform.md) · [Playing](playing.md) · [Packaging](packaging.md) · [64-bit model](../port64.md) · [Renderer, ES path](../renderer_port.md#10-opengl-es-30-path-one-renderer-source-for-windows-linux-macos-android-and-ios)

macOS is built for both kinds of Mac as two packages, `p4pc-macos-arm64-<version>.zip` (Apple silicon) and
`p4pc-macos-x86_64-<version>.zip` (Intel), each built natively on its own machine. The oldest supported system is macOS 12,
that of the ANGLE build the packages carry. Nobody on the project has a Mac: GitHub's hosted macOS runners are the only build
and test machines, and the list at the end says what that leaves unverified.

## What is built

Only the [64-bit model](../port64.md). A Mac runs no 32-bit processes, and a 64-bit Mach-O process cannot map below 4 GB
(`__PAGEZERO`), so the i386 placement of PS2 memory at its retail addresses is impossible; the guest block is reserved anywhere
(`p4_os_reserve_aligned`, [port64.md 2.1](../port64.md)). A macOS build directory is therefore like an AArch64 Linux one:
CMake configures the i386 targets but nothing builds them, and the target `p4_x64` builds the player (`p4pc64`), the viewer
and every test labelled `port64`. `P4_PORT64_ARCH` comes from the host (`aarch64` on Apple silicon, `x86_64` on Intel).

```sh
git clone --recurse-submodules https://github.com/Raikaru/Izanagi.git && cd Izanagi
sh ci/macos/build_deps.sh build/macos-deps            # clang, SDL3, FFmpeg, ANGLE (pinned, checksummed)
sh ci/macos/configure.sh build/macos-deps build/macos # cmake -G Ninja (needs cmake and ninja)
cmake --build build/macos --target p4_x64             # or --target p4pc64: the player only
ctest --test-dir build/macos -L '^port64$'            # the tests that need no disc pass; game tests skip
sh tools/package_macos.sh --deps build/macos-deps --build build/macos   # dist/p4pc-macos-<arch>-<version>.zip
```

Requirements: the Xcode command line tools (SDK, `ld`, `codesign`), CMake 3.20 or newer, Ninja and Python 3.

## Toolchain and libraries (`ci/macos/build_deps.sh`)

- **Compiler:** clang 23.1.3 from the macOS universal build of the pinned llvm-mingw release (`20261006`, SHA-256 checked),
  the same LLVM the Linux and Windows builds use. The 64-bit mirror's rewriter census and its layout oracle are per compiler
  major ([port64.md 2.17](../port64.md)), so one compiler for every platform keeps one set of facts. Only `clang`, `llvm-nm`,
  `llvm-objdump`, their two LLVM dylibs and the builtin headers are extracted. Linking uses Apple's `ld` (`-fuse-ld=ld`, set by
  `ci/macos/configure.sh`), the SDK is the command line tools' (`xcrun --show-sdk-path`).
- **SDL3 3.2.10** and the **LGPL-only FFmpeg 7.1.1 subset**: the same tarballs, checksums and configure options as
  `ci/win32/build_deps.sh`, built as dylibs whose install names are `@rpath/...`, with the architecture's assembly like the other
  builds (on Intel through nasm 2.16.03, built from its pinned source): the renderer's movie colour model reproduces swscale's SIMD
  YUV conversion (`src/render/p4gl_yuv_shader.h`), which `movie_colour_cpu64` checks exhaustively.
- **ANGLE** (`libEGL.dylib`, `libGLESv2.dylib`): Chromium's build, unmodified, from Electron 43.8.6's macOS zip for the
  architecture (pinned version and SHA-256). See below for why. The zip's SwiftShader is not taken: its ICD manifest is not code, so
  a signed bundle cannot carry it in `Frameworks` (`codesign --verify --strict` rejects it), and every Mac that runs macOS 12 has Metal.
- **Luau and miniz** are vendored (`third_party/`) and built by CMake like everywhere else; the C++ runtime is the system's
  `libc++` (nothing is linked statically).

## Mach-O in the build tools

- C names carry a leading underscore. `gen_port.py tables` and `crt_guard.py` read the objects with `--macho` (the C name of a
  symbol, Mach-O relocation types `ARM64_RELOC_*` / `X86_64_RELOC_*`, the host names of the dylibs and of the SDK's
  `libSystem.tbd` with the libraries it re-exports). They list relocations with `llvm-objdump --macho -r`, every entry as stored:
  the generic `-r` listing hides some x86_64 `X86_64_RELOC_UNSIGNED` entries of a data section (827 of the tables object's
  function-table pointers with LLVM 21), which made the CRT guard miss references on Intel.
- An asm label names a symbol exactly as written: `__asm__("func_0044b610")` is the symbol `func_0044b610`, while the C function
  is `_func_0044b610`. The mirror and the overrides spell labels for ELF, so the rewriter's Mach-O pass (`gptr_rewrite.macho_text`,
  on the preprocessed text before the AST is read) gives every declaration's label the prefix; port code that names a retail
  function by label uses `P4_ASM_LABEL("func_0044b610")` (`include/p4pc/os.h`). The tables step fails on any object that still
  references an unprefixed symbol. The same pass drops ELF section attributes (the decomp's one `.sdata` hint on an extern
  declaration), which a Mach-O target rejects.
- Clang's AST gives an ordinary declaration's symbol with the underscore (`mangledName` `_RwMatrixCreate`); the rewriter
  (`Rewriter.linked_name`) takes the C-level name from it, and writes the labels of its own ABI declarations with the target's
  prefix (`gptr_rewrite.symbol_prefix`).
- The i386 layout oracle of the rewrite compiles each unit for `i686-apple-macos10.13` against the same SDK (its i386 headers are
  the x86_64 ones), through the same Mach-O pass; only record layouts are taken from it. `tools/port64/unit64.py` derives this and
  the symbol prefix from the Apple target triple. Darwin's i386 `size_t`, `intptr_t` and `uintptr_t` are `long`-based, so
  `include/p4pc/compat.h` gives those units the host types instead of the decomp's `type.h` ones (the same 4 bytes), as it does
  for every 64-bit unit.
- `-D_FORTIFY_SOURCE=0`: the SDK turns fortification on for optimised builds, which would route the game's `memcpy`/`strcpy`
  through the checked `__*_chk` entry points instead of the reviewed byte seams (`config/host_symbols_allow.txt`). `-fno-blocks`
  and `-fno-stack-protector`: clang enables Apple's blocks (whose SDK declarations the rewriter has no rule for) and stack
  protection (which the other builds do not have: its guard would be a host data reference from every decomp function) by
  default. LLVM lowers `memset(p, 0, n)` to Darwin's `bzero` (`__bzero` on x86_64), reviewed in the same allowlist.
- Mach-O has no undefined weak symbol: the one weak reference of the platform layer (`kwlnTaskDumpAll` in `kernel_thread.c`,
  absent from the unit tests) is linked with `-Wl,-U` (a load-time lookup that stays NULL), and section garbage collection is
  ld64's `-dead_strip`. GNU ld's `--wrap` has no ld64 counterpart, so the few host tests that intercept calls with it (`ovdiff`, the
  CRI file-system and SFD callback adapters) are not built on macOS; Linux and Windows run them.
- **Variadic calls on Apple arm64.** Apple's arm64 ABI passes a variadic function's variable arguments on the stack and its fixed
  ones in registers (AAPCS64 elsewhere, x86-64 and i386 put both in the same places). A mirror call through a declaration that
  disagrees with the definition about `...` (an m2c `int f();` for a printf-like definition, a `...` prototype for a fixed one)
  would put the arguments where the callee does not read them, so on that target the rewriter sends it through the definition's
  prototype (`gptr_rewrite.variadic_on_stack`, census rule `abi-variadic-adapted`; `tests/variadic_call_rewrite_test.py` runs both
  directions on the arm64 runner). An override twin compiled without the ABI table declares the prototype in its prelude
  (`tests/lockstep_restore_witness_prelude.h`).
- The rewriter census has its own baselines, `config/port64macos_aarch64_census.txt` and `config/port64macos_x86_64_census.txt`,
  for the same clang major as Linux x86_64's. Intel's counts equal Linux x86_64's but for `skipped:macro-or-foreign` (the SDK's
  macros). Apple arm64's also differ where the ABI does: `abi-variadic-adapted` (above), and `addr-of-local-array` where a unit uses
  a `va_list`, which is a pointer there (as on i386) and an array that decays on x86_64, plus two in every unit from header code
  outside the mirror. A `long double` is a plain double there (`include/p4pc/eefloat.h` takes its integer path, as on Linux AArch64).

## Platform layer

`src/platform/os/os_posix.c` serves macOS: the executable's image window is its Mach-O segments (`_dyld_get_image_header`), the
coroutine switch is spelled for Mach-O on both architectures, a fatal signal writes the Darwin register state to `p4pc-crash.log`,
the module list comes from dyld instead of `/proc`, and a restart re-executes `_NSGetExecutablePath`. Threads, audio, input,
dialogs and paths are SDL3's. Headless runs (`p4_os_headless_env`) use a hidden window, as on Windows: SDL's offscreen driver
makes GL contexts only on EGL devices, which ANGLE's Metal back end does not offer.

Settings, saves and logs are in `~/Library/Application Support/p4pc/Persona4/` (`SDL_GetPrefPath("p4pc", "Persona4")`):
`p4pc.ini`, `save/`, `p4pc.log`, `p4pc-crash.log` and the `SLUS_217.82` read from the disc. A no-argument launch (Finder) is the
player path: the disc image is asked for once with the system file dialog ([playing.md](playing.md)).

## Renderer: OpenGL ES 3.0 on ANGLE's Metal back end

The renderer's default API is OpenGL ES 3.0 ([renderer_port.md 10](../renderer_port.md)). SDL3's Cocoa video driver creates an
ES context through EGL (`Cocoa_GLES_*`), and the EGL is ANGLE's. `src/render/p4gl_api.h` points SDL at the bundled libraries by
path (`SDL_HINT_EGL_LIBRARY`, `SDL_HINT_OPENGL_LIBRARY`: `Contents/Frameworks` of the app, else beside the executable, which is
where the build tree copies them), because SDL's own lookup is `dlopen("libEGL.dylib")`, which never searches an app bundle.
ANGLE's back end is Metal unless the run sets `ANGLE_DEFAULT_PLATFORM` or `P4PC_ANGLE` (`gl` asks for ANGLE on Apple's OpenGL,
which got no EGL display on the CI runners). When no ES context can be made, the renderer falls back to Apple's own desktop OpenGL
(4.1 core, deprecated by Apple but present), as it does on any machine without ES; `P4PC_GL_API=gl` asks for it directly. SDL's
Cocoa driver sets up the EGL display when it creates the window, so a Mac where ANGLE has no display (no Metal device: the Intel
runners) fails there, before the context: `p4gl_api_create_window` then makes the window again for desktop OpenGL and the rest of
the run takes it as `P4PC_GL_API=gl` would (an ES context attempt on that window leaves SDL without Apple's OpenGL entry points), and
the desktop attributes drop the ANGLE library hint so SDL loads Apple's OpenGL. On the runners that is the Apple Software Renderer,
and the renderer's `#version 330 core` shader header compiles and draws there.

**Why Chromium's ANGLE from Electron.** ANGLE publishes no binaries; its supported build is a Chromium-style checkout (depot_tools,
`gclient sync` of several GB of dependencies and Chromium's own clang) and `gn`/`ninja`, an hour or more per architecture on a
3-core runner, plus the upkeep of a pin that is a Chromium commit position. Electron's releases carry exactly Chromium's ANGLE
build for macOS, arm64 and x86_64, with published SHA-256 sums, so `build_deps.sh` pins a release and checks its sum like every
other input. Measured on the 43.8.6 zips: `libEGL.dylib` and `libGLESv2.dylib` are self-contained (system frameworks only),
built for macOS 12 (`LC_BUILD_VERSION minos 12.0`), and contain ANGLE's Metal, OpenGL (CGL) and Vulkan back ends. Electron 43 is
the last release line that ships them as separate dylibs (44 links ANGLE into its framework), so a later update means another
ANGLE source, not a newer Electron. The other prebuilt considered, the `EdgeFirstAI/angle-package` xcframework
([renderer_port.md 10.4](../renderer_port.md)), is a single small project's build. Apple's OpenGL alone was rejected as the
primary path: it is deprecated and stays only as the fallback.

## Packages (`tools/package_macos.sh`)

```
p4pc-macos-<arch>-<version>/
  Persona 4 PC.app/Contents/
    Info.plist                     org.p4pc.game, LSMinimumSystemVersion 12.0, NSHighResolutionCapable
    MacOS/p4pc                     p4pc64; run path @executable_path/../Frameworks only
    Frameworks/                    libSDL3.0.dylib, libavcodec/libavutil/libswscale, libEGL, libGLESv2
  README.txt  THIRD_PARTY_NOTICES.txt  VERSION.txt  licenses/  source/  examples/mods/
```

Before it zips (with `ditto`, which keeps the bundle intact), the packager checks that every Mach-O in the bundle loads only
`/usr/lib`, `/System/Library` and the bundle's own libraries, that the executable's only run path is the bundle's `Frameworks`,
and that the shipped FFmpeg dylibs are LGPL-only (`tests/ffmpeg_license_test.c` against them, `P4PC_REQUIRE_LGPL=1`). Every
library and then the bundle are signed ad hoc (`codesign --force --sign -`) and `codesign --verify --deep --strict` must pass.
There is no Developer ID, so the app is not notarized: on first start Gatekeeper refuses it. Players right-click the app and
choose Open (on macOS 15 and newer: open it once, then System Settings > Privacy & Security > Open Anyway), or remove the
download's quarantine flag with `xattr -dr com.apple.quarantine "Persona 4 PC.app"`. The README in the package says this.

## CI

The `macos` job of `.github/workflows/build.yml` runs on `macos-latest` (Apple silicon) and `macos-15-intel`: the dependencies
(cached by the hash of `build_deps.sh`), configure with the census check on, `cmake --build --target p4_x64`, `ctest -L '^port64$'`
(game tests skip without a disc), the GL context smoke (`tests/gles_context_smoke.c`: the port's own attributes, window and
context fallbacks, window-system calls, GL loader and shader header, a triangle read back from an RGBA8 framebuffer), then the
package (artifact `p4pc-macos-<arch>`, attached to `v*` releases). On Apple silicon the smoke must get OpenGL ES on ANGLE's Metal
back end (`P4PC_REQUIRE_GLES=1`, `P4PC_REQUIRE_GL_RENDERER=Metal`); the Intel runners have no Metal device, ANGLE gets no EGL
display there, and the smoke must draw through the game's fallback to Apple's desktop OpenGL instead. Both also draw with
`P4PC_GL_API=gl`. The logs and PNGs are the artifact `macos-<arch>-gl-smoke`.

## Not verified without a real Mac

GitHub's runners are virtual machines without a display, a GPU of their own, controllers or audio devices. Not covered:

1. **A real GPU.** The arm64 runners' Metal device is Apple's paravirtual GPU, the Intel runners have none (ANGLE's Metal back end
   never ran on Intel), and their desktop OpenGL is the Apple Software Renderer; frames were never compared on M-series or AMD/Intel
   Mac GPUs ([renderer_port.md 10.6](../renderer_port.md)). The scene and game gates need the disc and are not run on the runners.
2. **Retina scaling and the window**: a `backingScaleFactor` of 2, the present blit into a high-density window, fullscreen
   (separate Space), window resizing and multiple displays.
3. **Controllers** (GameController/IOKit through SDL) and the keyboard on a Mac keyboard layout; the README's keys are the
   other platforms'.
4. **Audio output** (Core Audio through SDL): the tests use SDL's dummy driver.
5. **The player path end to end**: Finder start, the disc dialog, a full boot and play from a real disc image, saves, and
   performance and pacing at 60 Hz on real hardware (including ProMotion displays).
6. **Gatekeeper and quarantine** on a downloaded zip: the runners build the zip but never download it.
7. **Intel Macs with AMD or Intel GPUs**, and macOS 12/13 (the runners are macOS 15 and newer).
8. **ANGLE's `gl` back end** (`P4PC_ANGLE=gl` gets no EGL display on the runners), and Apple's desktop OpenGL beyond the smoke.
