# Third-party components and licences

The port's own code is MIT-licensed ([LICENSE](LICENSE)). Code reconstructed from
the retail game is not licensed by us ([NOTICE](NOTICE)). Everything below
keeps its own licence. Release packages ship the notices and licence texts in
`THIRD_PARTY_NOTICES.txt` and `licenses/`. [third_party/README.md](third_party/README.md)
records exact versions, pins and local changes.

| Component | Where | Licence | Licence text |
|---|---|---|---|
| FFmpeg 7.1.1 (libavcodec, libavutil, libswscale) | not vendored; built from the pinned release tarball by the `ci/*/build_deps.sh` scripts and shipped as replaceable shared libraries; the exact tarball and build recipe ship in `source/` in every package | LGPL-2.1-or-later; the build uses `--disable-gpl --disable-nonfree`, and packaging stops unless `tests/ffmpeg_license_test.c` confirms an LGPL-only build | [FFmpeg-LGPL-2.1.txt](third_party/licenses/FFmpeg-LGPL-2.1.txt) |
| SDL 3.2.10 | not vendored; built from the pinned release tarball (Windows, Linux packages) or Debian's packages (development builds) | Zlib | [SDL3-zlib.txt](third_party/licenses/SDL3-zlib.txt) |
| Luau 0.744 | `third_party/luau/`, local patches in `third_party/luau_patches/` | MIT (including the inherited Lua notice) | [Luau-MIT.txt](third_party/licenses/Luau-MIT.txt), [Lua-MIT.txt](third_party/licenses/Lua-MIT.txt) |
| miniz 3.0.2 | `third_party/miniz/` | MIT | [third_party/miniz/LICENSE](third_party/miniz/LICENSE) |
| LodePNG | `third_party/lodepng/` | Zlib | [LodePNG-zlib.txt](third_party/licenses/LodePNG-zlib.txt) |
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

## Not covered by this repository's licence

- **The game.** Persona 4, its code, data, audio, video and trademarks belong to
  Atlus and SEGA. None of it is in this repository or in the release packages.
- **Reconstructed code** (`overrides/`, `drafts/`, `src/cri/` and the other files
  NOTICE lists; SPDX `LicenseRef-reconstructed`): no licence granted, see
  [LICENSES/LicenseRef-reconstructed.txt](LICENSES/LicenseRef-reconstructed.txt).
- **The decompilation** ([Raikaru/Persona4-Decompilation](https://github.com/Raikaru/Persona4-Decompilation),
  the `decomp/` submodule) has its own terms. That includes the RenderWare and CRI
  SDK headers it compiles against.
