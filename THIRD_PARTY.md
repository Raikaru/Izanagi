# Third-party components and licences

The port's own code is MIT-licensed ([LICENSE](LICENSE)). Code reconstructed from
the retail game is not licensed by us ([NOTICE](NOTICE)). Everything below
keeps its own licence. Release packages include notices for the components they
ship in `THIRD_PARTY_NOTICES.txt` and `licenses/`; source-only dependencies are
described below. [third_party/README.md](third_party/README.md) records exact
versions, pins and local changes.

| Component | Where | Licence | Licence text |
|---|---|---|---|
| FFmpeg 7.1.1 (libavcodec, libavutil, libswscale) | not vendored; pinned release tarball, replaceable shared libraries; MPEG-2/parser, ADX and Golden HCA only (no avformat); exact tarball and recipe ship in `source/` in every package; [HCA review](docs/golden_mode/audio.md) | LGPL-2.1-or-later; GPL/nonfree/version3 disabled, packaging requires the unchanged LGPL licence checks | [FFmpeg-LGPL-2.1.txt](third_party/licenses/FFmpeg-LGPL-2.1.txt) |
| SDL 3.2.10 | not vendored; built from the pinned release tarball (Windows, Linux packages) or Debian's packages (development builds) | Zlib | [SDL3-zlib.txt](third_party/licenses/SDL3-zlib.txt) |
| Luau 0.744 and inherited Lua | `third_party/luau/`; the Zig port in `luau-zig/src/` and pinned conformance sources in `luau-zig/tests/upstream-0.744/` | MIT (including inherited Lua notices); see the specific exceptions below | [Luau-MIT.txt](third_party/licenses/Luau-MIT.txt), [Lua-MIT.txt](third_party/licenses/Lua-MIT.txt) |
| doctest 2.4.9, including portions from Catch and lest | `luau-zig/tests/upstream-0.744/extern/doctest.h` | MIT and Boost-1.0; the header names the reused portions | [doctest MIT](luau-zig/tests/upstream-0.744/extern/doctest_LICENSE.txt), [Boost-1.0](LICENSES/BSL-1.0.txt) |
| Unicode 10.0.0 confusables data | `third_party/luau/Ast/src/Confusables.cpp` and its Zig translation `luau-zig/src/compiler/confusables.zig` | Unicode data terms and MIT for the Luau code | [Unicode-DFS-2016](LICENSES/Unicode-DFS-2016.txt), [Luau-MIT.txt](third_party/licenses/Luau-MIT.txt) |
| miniz 3.0.2 | `third_party/miniz/` | MIT | [third_party/miniz/LICENSE](third_party/miniz/LICENSE) |
| LodePNG | `third_party/lodepng/` | Zlib | [LodePNG-zlib.txt](third_party/licenses/LodePNG-zlib.txt) |
| bcdec 0.985 (BC7 only) | `third_party/bcdec/bcdec.h`, static-inline Golden CPU texture decoder | MIT option selected | [bcdec-MIT.txt](third_party/licenses/bcdec-MIT.txt) |
| yxml | `third_party/yxml/` | MIT | [yxml-MIT.txt](third_party/licenses/yxml-MIT.txt) |
| cgltf 1.15 | `third_party/cgltf/cgltf.h` | MIT (the header also bundles jsmn, whose MIT notice is inside the same file) | [cgltf-MIT.txt](third_party/licenses/cgltf-MIT.txt) |
| fdlibm (Sun Microsystems), via the retail newlib libm | translated in `src/platform/ee_libm.c` and `src/platform/ee_dlibm.c` | fdlibm notice (permission to use, copy, modify and distribute, provided the notice is kept) | [fdlibm-Sun.txt](third_party/licenses/fdlibm-Sun.txt) |
| newlib stdio/stdlib (UC Berkeley, AT&T), via the retail executable | translated in `src/platform/ee_printf.c`, `ee_scanf.c`, `ee_strtod.c`, `ee_dtoa.c`, `ee_decimal.c` | BSD (advertising clause rescinded) and the AT&T dtoa notice | [newlib-BSD.txt](third_party/licenses/newlib-BSD.txt) (verbatim from those file headers) |
| dlmalloc (Doug Lea), via the retail newlib allocator | translated in `src/platform/ee_heap.c` | public domain | none required |
| recvx-decomp CRI headers (AshfordFamily) | in the decomp submodule, `include/cri/recvx-decomp-cri/` | MIT | [recvx-decomp-MIT.txt](third_party/licenses/recvx-decomp-MIT.txt) |
| sky2hook (tge-was-taken) | parts of `src/render/`, each marked with a comment naming the source file | reuse permitted by the author | [sky2hook-permission.txt](third_party/licenses/sky2hook-permission.txt) |
| llvm-mingw runtime: mingw-w64 CRT, winpthreads, compiler-rt, libc++/libc++abi/libunwind | statically linked into the Windows executables | permissive; Apache-2.0 WITH LLVM-exception | [mingw-w64-runtime.txt](third_party/licenses/mingw-w64-runtime.txt), [winpthreads.txt](third_party/licenses/winpthreads.txt), [llvm-mingw-compiler-rt.txt](third_party/licenses/llvm-mingw-compiler-rt.txt), [LLVM-libcxx.txt](third_party/licenses/LLVM-libcxx.txt) |
| GCC libstdc++ / libgcc | statically linked into the Linux executables when the C++ adapter is used | GPL-3.0 WITH GCC-exception-3.1 | [GCC-GPL-3.0.txt](third_party/licenses/GCC-GPL-3.0.txt), [GCC-runtime-exception.txt](third_party/licenses/GCC-runtime-exception.txt) |
| AppImage type2 runtime, libfuse, squashfuse, musl, zstd, zlib | embedded in the Linux AppImage by `tools/appimage_build.sh` | MIT; LGPL-2.1; BSD; MIT; BSD; Zlib | fetched and shipped inside the AppImage |
| ANGLE (Chromium) via Electron 43.8.6 | not vendored; `libEGL.dylib`/`libGLESv2.dylib` taken from the SHA-pinned Electron macOS zip by `ci/macos/build_deps.sh` and shipped in Contents/Frameworks | BSD 3-clause (ANGLE and the code it contains); Electron's own licence likewise | not in this repository: fetched with the zip at build time, shipped as `licenses/LICENSES.chromium.html` and `licenses/Electron-LICENSE.txt` in the macOS package |
| Android NDK r27 runtime (libc++, libc++abi, libunwind, compiler-rt) | not vendored; static from the NDK (`c++_static`) selected by `ci/android/Dockerfile` | Apache-2.0 WITH LLVM exceptions, plus Google's NDK notices | [LLVM-libcxx.txt](third_party/licenses/LLVM-libcxx.txt); the NDK `NOTICE` and `NOTICE.toolchain` are copied from the NDK install and shipped in the APK/AAB assets |
| Okumura LZARI + Ross Ridge mymc | adapted in `src/platform/memcard/import_max.c` (attributed in that file's header) | public domain | none required |
| NASM 2.16.03 | not vendored; built by `ci/macos/build_deps.sh` as the assembler for FFmpeg's x86 assembly | BSD 2-clause | build tool only: not linked into or shipped with any package |

The clean-room reverb implementation in `src/platform/sound/p4snd_reverb.c`
uses the hardware descriptions in [psx-spx, Sound Processing Unit: SPU Reverb
Registers, SPU Reverb Formula and Reverb Buffer Resampling](https://psx-spx.consoledev.net/soundprocessingunitspu/).

The Zig compiler, VM, standard libraries and require implementation contain
translations or adaptations of [Luau 0.744 at commit
`32c28089305f488f901aaf1dbf3fd1cb65fa3d11`](https://github.com/luau-lang/luau/tree/32c28089305f488f901aaf1dbf3fd1cb65fa3d11);
the accompanying conformance snapshot records upstream Git blob IDs in its
`manifest.json` and `target-manifest.json`. Its upstream Luau and Lua licence
texts are also retained in that snapshot. Luau's notice attributes copyright
2019–2026 to Roblox Corporation and 1994–2019 to Lua.org, PUC-Rio; individual
Lua-derived tests retain their original headers.

The snapshot's doctest header attributes doctest to Viktor Kirilov, copyright
2016–2021, and identifies code taken from Catch and lest under Boost-1.0.
Its notices are retained verbatim; the header does not identify the precise
upstream Catch or lest revisions or a complete set of copyright holders for
their reused code. Those details remain unverified, not inferred from later
versions. The pinned upstream
[doctest header](https://github.com/luau-lang/luau/blob/32c28089305f488f901aaf1dbf3fd1cb65fa3d11/extern/doctest.h)
and [doctest 2.4.9 licence](https://github.com/doctest/doctest/blob/v2.4.9/LICENSE.txt)
are the provenance for these notices.

Luau's confusables table identifies [Unicode's 10.0.0 data
file](https://www.unicode.org/Public/security/10.0.0/confusables.txt), which
states © 2017 Unicode, Inc. The included Unicode licence is the data-files
and software exhibit from [terms retrieved 1 May
2017](https://www.spec.org/cpu2017/docs/licenses/Unicode.txt), also corroborated
by [Unicode's ICU 59.1 notice](https://github.com/unicode-org/icu/blob/release-59-1/icu4c/LICENSE).
Unicode's [current terms](https://www.unicode.org/copyright.html) refer to
Unicode License v3; these are distinct from the historical notice reproduced
here. No determination is made here about any additional rights or terms.

## Not covered by this repository's licence

- **The game.** Persona 4, its code, data, audio, video and trademarks belong to
  Atlus and SEGA. The original executable and game assets are not distributed
  in this repository or its release packages.
- **Reconstructed code** (`overrides/`, `drafts/`, `src/cri/` and the other files
  NOTICE lists; SPDX `LicenseRef-reconstructed`): no licence granted, see
  [LICENSES/LicenseRef-reconstructed.txt](LICENSES/LicenseRef-reconstructed.txt).
- **The decompilation** ([Raikaru/Persona4-Decompilation](https://github.com/Raikaru/Persona4-Decompilation),
  the `decomp/` submodule) has its own terms. That includes the RenderWare and CRI
  SDK headers it compiles against.

## FFmpeg HCA alignment modification

Shipping FFmpeg7.1.1 includes the LGPL-2.1-or-later
[`ci/ffmpeg-7.1.1-hca-window-alignment.patch`](ci/ffmpeg-7.1.1-hca-window-alignment.patch).
It aligns only HCA's constant window for its SIMD DSP; no coefficients or
arithmetic change. Every platform's package ships the pinned upstream archive,
the exact `ci/<platform>/build_deps.sh` and this patch under its source directory,
and identifies FFmpeg as modified. Dynamic replacement and the existing LGPL
checks remain mandatory.
