# Third-party code

| Component | Where | Origin | Terms |
|---|---|---|---|
| sky2hook (parts) | ported into `src/render/` where noted | https://github.com/tge-was-taken/sky2hook by tge-was-taken | Reuse permitted by the author: "In regards to licensing sky2hook, feel free to reuse parts of it." (https://github.com/Raikaru/Persona4-Decompilation/issues/2#issuecomment-6234440298, 2026-10-17). The repository has no LICENSE file; this permission is the grant. |
| yxml | `third_party/yxml/` | https://dev.yorhel.nl/yxml, pinned revision `66507906673bc6159d5d620414479954c9c21c24`; exact upstream archive mirrored at https://fuchsia.googlesource.com/third_party/yxml/+archive/66507906673bc6159d5d620414479954c9c21c24.tar.gz | MIT, original copyright/permission in both source files and `third_party/licenses/yxml-MIT.txt`. Streaming XML parser; bounded caller-owned stack, no external-entity fetching. Portability patch: `yxml.c` includes its colocated header with quotes. Used only for Mod Compendium metadata. |
| FFmpeg libavcodec, libavutil, libswscale | not vendored; the system i386 packages (`libavcodec-dev:i386`, `libavutil-dev:i386`, `libswscale-dev:i386`, in `ci/Dockerfile`) | https://ffmpeg.org | LGPL 2.1 or later for the libraries. Linked dynamically only, so a user can replace the shared libraries. Used for MPEG video picture decode and the YUV to RGBA conversion in `src/movie/`. Debian builds FFmpeg with `--enable-gpl`, which makes its binaries GPL as built. Only the built-in `mpeg1video`/`mpeg2video` decoder and swscale are used, so a release must ship or require an LGPL-only FFmpeg build (`./configure --disable-gpl --disable-nonfree`). |
| FFmpeg 7.1.1 (Windows build) | not vendored; built from the pinned release tarball by `ci/win32/build_deps.sh` as DLLs | https://ffmpeg.org | LGPL 2.1 or later, built `--disable-gpl --disable-nonfree` with only the MPEG-2 video decoder and parser, the ADX audio decoder, libswscale and libavutil, so the Windows binaries are LGPL as built. Shared (DLL) only. |
| SDL3 3.2.10 (Windows build) | not vendored; built from the pinned release tarball by `ci/win32/build_deps.sh` as `SDL3.dll` | https://libsdl.org | zlib license |
| llvm-mingw 20261006 | the Windows cross toolchain, not shipped (`ci/Dockerfile`) | https://github.com/mstorsjo/llvm-mingw | Apache-2.0 with LLVM exceptions; the mingw-w64 CRT and headers it links are permissively licensed. |
| miniz 3.0.2 low-level codecs/checksums | `third_party/miniz/` | https://github.com/richgel999/miniz/tree/3.0.2 | MIT; see `third_party/miniz/LICENSE` and source copyright notices. Bounded `tinfl` decodes CBS data with Adler-32 verification; streaming `tdefl` writes level-1 capture PNGs without whole-image/row staging. No ZIP component or zlib-style API/aliases is used. Local changes: `miniz_tinfl.c` includes its low-level header directly; `miniz.h` guards the unused ZIP include; `miniz_export.h` configures static, low-level-only linkage. |
| LodePNG 20261006 | `third_party/lodepng/` | https://github.com/lvandeve/lodepng, pinned commit `e584a8490db1b76b81bf298c11c09d3b55d9c270` | zlib; original notice in both source files and `third_party/licenses/LodePNG-zlib.txt`. The upstream `.cpp` file is renamed `.c` as permitted by its header; otherwise vendor bytes are unchanged. PNG image decode uses bounded host allocation, with disk I/O, encoding and ancillary metadata disabled. |
| JPEG XL `cjxl` / `djxl` 0.11.1 | capture-maintenance tools only, not vendored or linked into the port; install the portable binaries yourself (P4PC_JXL_BIN_DIR or PATH) | https://github.com/libjxl/libjxl/releases/tag/v0.11.1 ; `jxl-linux-x86_64-static-v0.11.1.tar.gz`, SHA256 `7ba87d09f220568a7e84c2a62e9fa8be608443930dec10b2799271d4cf032293` | JPEG XL: BSD-3-Clause (`LICENSE.jpeg-xl` in the package). No renderer-library or player-runtime dependency. |
| Okumura LZARI | adapted arithmetic model and decoder in `src/platform/memcard/import_max.c`, synthetic encoder in `tests/import_max_test.c` | Haruhiko Okumura, `LZARI.C` (1989), as preserved at https://github.com/bucanero/psv-save-converter/blob/master/src/lzari.c | Public domain; original notice: "Use, distribute, and modify this program freely. Please send me your improved versions." MAX framing and CRC reference: Ross Ridge's public-domain https://github.com/ps2dev/mymc/blob/master/ps2save.py. |
| Luau 0.744 | `third_party/luau/` | https://github.com/luau-lang/luau/releases/tag/0.744 ; exact commit `32c28089305f488f901aaf1dbf3fd1cb65fa3d11` | MIT (`LICENSE.txt`), including inherited Lua MIT notices (`lua_LICENSE.txt`). VM, Compiler, Ast, Common, Analysis and their required Bytecode/Config components; upstream CMake/source lists retained as provenance. No CodeGen, JIT, Inliner, Require, upstream CLI or external libraries. One recorded include-only portability patch: `third_party/luau_patches/0001-analysis-cstdlib.patch` adds the direct declaration header for integer `std::abs` on libc++ 23; no behavioral changes. All other vendor bytes are unmodified. |
| GCC libstdc++ / libgcc runtime | static runtime objects from Linux profile toolchains, not vendored sources | https://github.com/gcc-mirror/gcc/tree/releases/gcc-14.2.0 ; `COPYING3`, `COPYING.RUNTIME` | GPLv3 with GCC Runtime Library Exception 3.1, allowing independent modules built by eligible compilers; complete notices in `third_party/licenses/GCC-GPL-3.0.txt` and `GCC-runtime-exception.txt`. |
| LLVM libc++ runtime | static from llvm-mingw/Android NDK; Apple's system libc++ for real iOS SDK links | https://github.com/llvm/llvm-project/blob/llvmorg-19.1.7/libcxx/LICENSE.TXT | Apache-2.0 with LLVM exceptions and retained component notices, copied verbatim to `third_party/licenses/LLVM-libcxx.txt`; the existing compiler-rt/winpthreads notices also apply to Windows runtime objects. |
| SDL3 3.2.10 and FFmpeg 7.1.1 (Linux/Android packages) | shared objects built by `ci/win32/build_deps.sh` (Linux) / `ci/android/build_deps.sh` (Android) | https://libsdl.org ; https://ffmpeg.org | zlib / LGPL-2.1-or-later. Packages include licence notices, complete original source archives and the build recipe. Libraries are replaceable (extract AppImage or repack/re-sign APK). |
| AppImage type2 runtime `fedfad51acc2860b2916a1eef32ba4a500ff31ef` | embedded in Linux AppImages by `tools/appimage_build.sh` | https://github.com/AppImage/type2-runtime | MIT; static libfuse 3.15.0 (LGPL-2.1), squashfuse 0.5.2 (BSD), musl (MIT), zstd (BSD), zlib (zlib). Actual upstream notices, runtime/modified-libfuse sources and build scripts are included in the AppDir (`--appimage-license` is unimplemented in this runtime). `appimagetool` is a checked build-only tool, not a player library. |

The native Config extension does not bundle a font or retail art/audio. It uses
Persona 4's own Config sprites, `FONT0.FNT` glyph/metric data and COMSE sequences
from the user's disc at runtime. Those copyrighted game resources are not part
of this repository or the distributed port.

## sky2hook reuse rules

- Mark every file or function derived from sky2hook with a comment naming the sky2hook source file it came from, for example `/* Derived from sky2hook src/render/fog.cpp (tge-was-taken), reused with permission; see third_party/README.md */`.
- The port is C. Translate sky2hook's C++ rather than vendoring it, unless a whole module is better kept as is. In that case put it under `third_party/sky2hook/` with this note beside it.
- sky2hook hooked emulated code inside PCSX2. Its addresses and struct offsets are evidence to cross-check against the decomp and the retail asm, not ground truth.

## Luau build and boundary

`P4PC_LUAU` defaults ON. `include/p4pc/luau_c.h` is the C boundary;
`src/platform/mods/luau_c.cpp` is an isolated host-adapter static library,
not a decomp/native-game object and not subject to the EE float/guest-pointer
rewriter. The VM/Compiler archives provide the upstream C API directly.
The game binding registers the Lua adapter against each selected `p4port`
profile and links the VM/Compiler into the corresponding player and viewer.
The table and CRT closures inspect compiled profile-specific archives; this
does not relax the reviewed game CRT policy. Analysis and Config remain
tooling-only and are not dependencies of the runtime C boundary.
The paired compiler, existing-profile enumeration, compilation flags and
static runtime policy live in `cmake/host-cxx.cmake`, shared with the private
Basis content decoder. C++ is activated when either `P4PC_LUAU` or
`P4PC_CONTENT_PACKS` is enabled; data content packs do not depend on scripting.
When a COFF profile is selected, profile enumeration sets the caller
directory's `CMAKE_LINK_DEPENDS_USE_LINKER` to OFF before creating adapters,
matching the root's MinGW link policy. CMake's ELF linker depfile option is
not supported by MinGW lld; ordinary archive/target dependencies remain.
Native Win64 adapters use the same `P4_WIN64_PREFIX/lib` presence gate as the
root's native Win64 consumers; a compiler-only COFF mirror does not request
cross-runtime adapters. Wine is optional during configuration: when absent,
the 64w provider/CLI/test binaries still build, while their runtime tests are
not registered and configuration reports the skip. The standalone profile
driver continues to build and exercise each requested ABI independently.

Future consumers opt in with
`p4_host_adapter_register(suffix C|CXX targets...)`. A root-directory deferred
pass follows actual `p4port${suffix}` object consumption after fixture cloning
and attaches only that profile's libraries, choosing the C++ linker where
required. Final executable, shared and module consumers can use either CMake
link signature; adapter dependencies remain private. This covers late test
fixtures and direct object linkage without player/test name lists or copied
foreign-ABI links.
CRT audits receive the actual profile-specific generated bridge archive as a
project definition source, not as an unreviewed host-library exception.
Unusual factories can explicitly call
`p4_host_adapter_attach(target object_target)`. The native
`host_adapter_graph` fixture compiles and runs real C/C++ consumers, including
cloned children, pure-C adapters, repeated registration and an unrelated
executable; it also links shared/module consumers. It starts from a C-only
project and a cached missing C++ compiler, exercising paired compiler
discovery and recovery rather than preselecting C++ externally. When the root
has a native Win64 consumer, it also compiles/links a real COFF probe (without
executing it or requiring Wine).

Every component uses clang/clang++, C++17, `-O2 -g1 -fno-rtti`, function/data
sections and linker dead stripping. VM uses `LUA_USE_LONGJMP=1` and
`-fno-exceptions`. Ast/Compiler/Analysis use real C++ exceptions internally
(upstream parser/compiler error recovery requires them); the C compiler
boundary catches every exception before touching the VM. All C++ RAII work
finishes before protected Lua calls. Native callbacks must be C/longjmp-safe
and must not throw C++ exceptions through the VM. CodeGen is neither vendored
nor a build target on any platform.

The state allocator enforces a hard total VM allocation cap, including
initialization, all coroutines, strings and buffers. Compiler scratch is
separate host memory: `source_limit` bounds source input (default 1 MiB),
not peak compiler allocations. Source is compiled in-process and loaded
immediately; the boundary has no external-bytecode entry point. The upstream
`lua.h` passthrough is a trusted-host API, not script-accessible capability.
The host owns CPU accounting; the interrupt callback latches exhaustion
across nested protected calls and coroutines until the host explicitly resets
it. A script cannot report successful completion after catching that error.
All VM allocations satisfy Luau's 16-byte alignment requirement. Win32 uses
the CRT's `_aligned_realloc`/`_aligned_free` because ordinary i686 UCRT
allocation promises only eight-byte alignment; other requested profiles use
their naturally 16-byte-aligned allocator. This preserves optimized aligned
VM stores instead of disabling SSE or lowering optimization.

Only base/coroutine/table/string/math/utf8/bit32/buffer libraries are opened,
followed by the host's protected registration callback, upstream
`luaL_sandbox`, and `luaL_sandboxthread`. Normal Luau protected calls,
coroutines, environment and metatable APIs remain available with upstream
sandbox semantics; no ad-hoc global removal substitutes for that sandbox.
The reusable thread's safe-environment optimization is disabled immediately
after sandboxing: host globals can expose mutable metatable-backed views, so
Luau must not cache their field values as imports, even in the first loaded
chunk. This does not unfreeze readonly globals, libraries or metatables.
There is no OS, I/O, debug, require or script loader library, and no
experimental integer/class/vector library. `p4_luau_user` recovers host context
through the allocator on every coroutine without taking callback userdata.
Deterministic game arithmetic/buffer bindings and source admission policy are
the mod host's separate responsibility.

`p4_luau_check_source` performs bounded in-process compilation and discards
its private bytecode without loading or executing it. It preserves the VM
stack, allocator usage and sticky interrupt state, reporting compiler errors
through `p4_luau_error`. It does not type-check or link Analysis into the
runtime provider. A two-pass mod loader can instead retain the function from
`p4_luau_load_source` during admission and execute only after every enabled mod
has been admitted, avoiding duplicate compilation.

`p4_luau_check definitions.d.luau mod.luau` runs the actual Analysis frontend,
defaulting to strict mode with a ten-second per-module typecheck limit, no
filesystem module resolver and no executable config. It prints one JSON object with
`ok` and `diagnostics` (file, one-based line/column and end positions,
severity/message); exit 0 = accepted, 1 = diagnostics, 2 = input/tool failure.
Definitions are host-provided tooling input. The CLI does not run game-mod
initialization; upstream compile-time type-function evaluation is still part
of Analysis.
Definitions load after builtin globals, so the authoritative tooling
definitions can replace unavailable runtime globals with declarations such
as `declare os: nil` without changing the VM's normal sandbox semantics.
Source mode directives remain normal Luau tooling behavior; successful
type-checking is not a runtime capability or memory-safety boundary.

### Profiles and reproducible size measurements

The retail-free project is `tools/luau/CMakeLists.txt`. In the CI image, run
`sh tools/luau_profiles.sh <profile> <build-dir>` for `linux-x64`,
`linux-i386`, `win64`, `win32`, `linux-aarch64` or `android-arm64`.
Linux uses static libstdc++
and libgcc, Windows uses llvm-mingw's static libc++/unwind/compiler runtime,
Android requires NDK `c++_static`, and a real iOS SDK build uses Apple's
system libc++. Host OS libc/libm and platform system libraries remain dynamic.
The CI base requires `g++-multilib`; its AArch64 layer additionally requires
`g++-aarch64-linux-gnu`. Android configure commands explicitly select
`ANDROID_STL=c++_static` (no `libc++_shared.so` to package).

The C acceptance executable exercises source compilation/execution, approved
and absent globals, readonly libraries, coroutine host context, source/line
diagnostics, source size admission, VM OOM, nested/sticky interruption and
host reset/recovery. The JSON checker test uses real definitions/type errors.
The profile driver runs these on Linux, Wine, and AArch64 qemu; Android is
compile/link-only without a device. `tools/ios_check.sh` also syntax-checks
the real C boundary acceptance source for `arm64-apple-ios13.0`.
`tools/luau_ios_check.py` checks all vendored C++ units and the C++ boundary
for that Apple target using the AArch64 CI image's GNU C/C++ declarations
and narrowly declared Mach/pthread Apple APIs. These declaration-only
surrogates are explicitly not an Apple libc/libc++ ABI or link qualification.
No Apple SDK is distributed; actual iOS linking/runtime/size requires one.

`tools/luau_size.py` strips private copies of the compiler/load/run probe,
an empty C main linked with identical flags, and the Analysis CLI. Its JSON
reports the probe-minus-baseline delta including the retained static C++
runtime. This is a reproducible isolated added-size measurement, not a
fabricated whole-game executable delta. Analysis is measured separately
because it is not a runtime dependency.

Observed qualification for source `4de3159923d0aa892f89ddc9a11b74217ff5fc92`
(exact stripped bytes; `tests/luau_profile_sizes.json` retains image IDs,
compiler versions, raw size/import records and a reproducible source-content
manifest SHA independent of commit metadata):

| Profile | VM + in-process compiler added bytes | Separate Analysis CLI bytes | C++ runtime linkage | Exercised qualification |
| --- | ---: | ---: | --- | --- |
| Linux x86_64 | 943,488 | 4,898,848 | Static libstdc++/libgcc; dynamic libc/libm/loader only | Compile/link; C sandbox and JSON fixtures pass |
| Linux i386 | 1,077,876 | 5,462,340 | Static libstdc++/libgcc; dynamic libc/libm/loader only | Compile/link; C sandbox and JSON fixtures pass |
| Windows x64 | 947,200 | 4,092,416 | Static libc++/unwind/compiler-rt/winpthread; Kernel32/UCRT imports only | Compile/link; both fixtures pass under headless Wine |
| Windows i686 | 1,079,808 | 5,237,760 | Static libc++/unwind/compiler-rt/winpthread; Kernel32/UCRT imports only | Compile/link; both fixtures pass under headless Wine |
| Linux AArch64 | 935,960 | 4,668,568 | Static libstdc++/libgcc; dynamic libc/libm only | Compile/link; both fixtures pass under qemu-aarch64 |
| Android arm64-v8a | 1,047,928 | 4,345,384 | NDK static libc++; libc/libm/libdl imports only | Compile/link, including the real C test; no device run |
| iOS arm64 | Not measured (no SDK link) | Not measured | Real SDK build uses system libc++ (not linked here) | 140 vendor C++ units, C++ boundary and C test pass Apple-target syntax check with declaration surrogates |

Desktop and AArch64 qualification used private images rebuilt from committed
recipes `073c825e70825282f77bc85f39809c0751f6a680`, not transient package
installation or mutation of shared CI tags. The shared base/AArch64 image
prerequisites are accepted as `0bd03f8a96b4825dd5d21d85aa426937c0946f86`;
this inert foundation is based on that cohort. The standalone measurements
do not claim a full-game build or size delta. Game-binding integration is a
separate reviewed change.

