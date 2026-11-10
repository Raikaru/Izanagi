#!/bin/sh
# Builds and fetches everything the macOS build compiles with, links and ships, for the host architecture (arm64 on Apple
# silicon, x86_64 on Intel), on macOS with the Xcode command line tools:
#   - the compiler: clang 23 of the pinned llvm-mingw release the Linux and Windows builds use (ci/Dockerfile), its macOS
#     universal host build. The 64-bit mirror's rewriter census is per compiler major (tools/port64/unit64.py), so every
#     platform compiles the game with the same clang;
#   - SDL3 and the LGPL-only FFmpeg subset, from the same pinned sources and options as ci/win32/build_deps.sh, as shared
#     libraries whose install names are @rpath/... (the app bundle carries them in Contents/Frameworks);
#   - ANGLE, the OpenGL ES 3.0 implementation on Metal the renderer's default ES path runs on (docs/renderer_port.md 10.4):
#     libEGL.dylib and libGLESv2.dylib as built by Chromium and released inside Electron's macOS zip (pinned release and
#     SHA-256; docs/platform/macos.md says why it is not built here). SwiftShader, which the zip also carries, is not taken:
#     its ICD manifest is not code, so a signed bundle cannot hold it beside the library, and every Mac macOS 12 runs on has Metal.
#     Luau and miniz are vendored under third_party/ and built by CMake.
#
#   sh ci/macos/build_deps.sh <prefix>
#
# <prefix>/llvm: the compiler (bin/clang, bin/clang++, bin/llvm-nm, bin/llvm-objdump, bin/llvm-ar); <prefix>/include, <prefix>/lib: SDL3,
# FFmpeg and ANGLE; <prefix>/share/angle: ANGLE's and Chromium's licence files. Downloads are cached in <prefix>/../dl-cache
# (or $P4PC_DL_CACHE) and checked against the SHA-256 below. Needs: curl, tar, unzip, cmake, ninja, make, python3, shasum.
set -eu

prefix=${1:?usage: build_deps.sh <prefix>}
dl=${P4PC_DL_CACHE:-$prefix/../dl-cache}
[ "$(uname -s)" = Darwin ] || { echo "build_deps: macOS only" >&2; exit 1; }
case $(uname -m) in
  arm64)  arch=arm64;  ffarch=aarch64; electron_arch=arm64 ;;
  x86_64) arch=x86_64; ffarch=x86_64;  electron_arch=x64 ;;
  *) echo "build_deps: unsupported architecture $(uname -m)" >&2; exit 1 ;;
esac
# ANGLE's dylibs below are built for macOS 12; nothing the port ships may need a newer system.
deployment=${MACOSX_DEPLOYMENT_TARGET:-12.0}
export MACOSX_DEPLOYMENT_TARGET=$deployment

LLVM_MINGW_RELEASE=20261006
LLVM_MINGW_SHA=a7a19b11048e89372637d26279cdb7c430ed3cf8c1e224310421312bf986ebf6
LLVM_MINGW_URL=https://github.com/mstorsjo/llvm-mingw/releases/download/$LLVM_MINGW_RELEASE/llvm-mingw-$LLVM_MINGW_RELEASE-ucrt-macos-universal.tar.xz
SDL_VER=3.2.10
SDL_SHA=f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0
SDL_URL=https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VER/SDL3-$SDL_VER.tar.gz
FFMPEG_VER=7.1.1
FFMPEG_SHA=733984395e0dbbe5c046abda2dc49a5544e7e0e1e2366bba849222ae9e3a03b1
FFMPEG_URL=https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VER.tar.xz
# Intel only: FFmpeg's x86 assembly needs nasm (the Linux and Windows builds have Debian's); swscale's SIMD YUV conversion is the
# arithmetic the renderer's x86 movie colour model reproduces (src/render/p4gl_yuv_shader.h, tests/movie_colour_test.c).
NASM_VER=2.16.03
NASM_SHA=1412a1c760bbd05db026b6c0d1657affd6631cd0a63cddb6f73cc6d4aa616148
NASM_URL=https://www.nasm.us/pub/nasm/releasebuilds/$NASM_VER/nasm-$NASM_VER.tar.xz
# Electron 43 is the last line whose macOS framework carries ANGLE as separate dylibs (44 links it into the framework).
ELECTRON_VER=43.8.6
case $electron_arch in
  arm64) ELECTRON_SHA=dd3ef1cf749e11f4426a1b5fb203bff1dbbc8cd589f43c46423ee40a87371390 ;;
  x64)   ELECTRON_SHA=c12e82216da91117623165c8f9f6c8175cb7f9940d01da2cbbd5c6696dcf0b45 ;;
esac
ELECTRON_ZIP=electron-v$ELECTRON_VER-darwin-$electron_arch.zip
ELECTRON_URL=https://github.com/electron/electron/releases/download/v$ELECTRON_VER/$ELECTRON_ZIP

mkdir -p "$prefix" "$dl"
prefix=$(cd "$prefix" && pwd)
dl=$(cd "$dl" && pwd)
work=$prefix/../deps-build-macos-$arch
mkdir -p "$work"
work=$(cd "$work" && pwd)
jobs=$(sysctl -n hw.ncpu)

fetch() { # url sha file
  if [ ! -f "$dl/$3" ]; then curl -fsSL --retry 6 --retry-all-errors --retry-delay 10 -o "$dl/$3.part" "$1" && mv "$dl/$3.part" "$dl/$3"; fi
  echo "$2  $dl/$3" | shasum -a 256 -c - >/dev/null || { echo "build_deps: checksum mismatch for $3" >&2; exit 1; }
}

# ---- the compiler: only what compiling, linking and reading objects needs (the release carries every mingw sysroot too)
if [ ! -x "$prefix/llvm/bin/clang" ] || [ ! -x "$prefix/llvm/bin/llvm-ar" ] || [ "$(cat "$prefix/llvm/release" 2>/dev/null)" != "$LLVM_MINGW_RELEASE" ]; then
  fetch "$LLVM_MINGW_URL" "$LLVM_MINGW_SHA" llvm-mingw-$LLVM_MINGW_RELEASE-macos.tar.xz
  rm -rf "$prefix/llvm" && mkdir -p "$prefix/llvm"
  top=llvm-mingw-$LLVM_MINGW_RELEASE-ucrt-macos-universal
  tar -xJf "$dl/llvm-mingw-$LLVM_MINGW_RELEASE-macos.tar.xz" -C "$prefix/llvm" --strip-components=1 \
    "$top/bin/clang" "$top/bin/clang++" "$top/bin/clang-23" "$top/bin/llvm-nm" "$top/bin/llvm-objdump" "$top/bin/llvm-ar" \
    "$top/lib/libLLVM.dylib" "$top/lib/libLLVM-23.dylib" "$top/lib/libLLVM.23.1.dylib" \
    "$top/lib/libclang-cpp.dylib" "$top/lib/libclang-cpp.23.1.dylib" "$top/lib/clang/23/include"
  "$prefix/llvm/bin/clang" --version | head -n 1
  printf '%s' "$LLVM_MINGW_RELEASE" > "$prefix/llvm/release"
fi

# ---- SDL3
if [ ! -f "$prefix/lib/libSDL3.0.dylib" ]; then
  fetch "$SDL_URL" "$SDL_SHA" SDL3-$SDL_VER.tar.gz
  rm -rf "$work/sdl" && mkdir -p "$work/sdl" && tar -xzf "$dl/SDL3-$SDL_VER.tar.gz" -C "$work/sdl" --strip-components=1
  cmake -S "$work/sdl" -B "$work/sdl/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_OSX_ARCHITECTURES="$arch" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$deployment" -DCMAKE_INSTALL_NAME_DIR=@rpath \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_INSTALL_TESTS=OFF \
    -DSDL_DISABLE_INSTALL_DOCS=ON
  cmake --build "$work/sdl/build"
  cmake --install "$work/sdl/build"
fi

# ---- nasm (Intel), for FFmpeg's x86 assembly
if [ "$arch" = x86_64 ] && { [ ! -x "$prefix/nasm/bin/nasm" ] || [ "$(cat "$prefix/nasm/version" 2>/dev/null)" != "$NASM_VER" ]; }; then
  fetch "$NASM_URL" "$NASM_SHA" nasm-$NASM_VER.tar.xz
  rm -rf "$work/nasm" "$prefix/nasm" && mkdir -p "$work/nasm" && tar -xJf "$dl/nasm-$NASM_VER.tar.xz" -C "$work/nasm" --strip-components=1
  (cd "$work/nasm" && ./configure --prefix="$prefix/nasm" && make -j"$jobs" && mkdir -p "$prefix/nasm/bin" && cp nasm "$prefix/nasm/bin/nasm")
  printf '%s' "$NASM_VER" > "$prefix/nasm/version"
fi

# ---- FFmpeg (LGPL): the same decoders and options as ci/win32/build_deps.sh, with the architecture's assembly (NEON on arm64, the
# x86 SIMD through the nasm above on Intel), as on Linux and Windows.
FFMPEG_OPTS="--enable-decoder=mpeg2video --enable-parser=mpegvideo --enable-decoder=adpcm_adx"
asm=
[ "$arch" = x86_64 ] && asm=--x86asmexe=$prefix/nasm/bin/nasm
if [ ! -f "$prefix/lib/libavcodec.dylib" ] || [ "$(cat "$prefix/ffmpeg.opts" 2>/dev/null)" != "$FFMPEG_OPTS $asm" ]; then
  fetch "$FFMPEG_URL" "$FFMPEG_SHA" ffmpeg-$FFMPEG_VER.tar.xz
  rm -rf "$work/ffmpeg" && mkdir -p "$work/ffmpeg" && tar -xJf "$dl/ffmpeg-$FFMPEG_VER.tar.xz" -C "$work/ffmpeg" --strip-components=1
  # shellcheck disable=SC2086
  (cd "$work/ffmpeg" && ./configure \
    --prefix="$prefix" --arch=$ffarch --target-os=darwin --cc=clang --install-name-dir=@rpath ${asm:+"$asm"} \
    --disable-everything --disable-programs --disable-doc --disable-network --disable-autodetect \
    --disable-avdevice --disable-avformat --disable-avfilter --disable-swresample \
    --enable-avcodec --enable-swscale $FFMPEG_OPTS \
    --enable-shared --disable-static --disable-debug \
    --extra-cflags="-O2 -mmacosx-version-min=$deployment" --extra-ldflags="-mmacosx-version-min=$deployment" \
    && make -j"$jobs" && make install) && printf '%s' "$FFMPEG_OPTS $asm" > "$prefix/ffmpeg.opts"
fi

# ---- ANGLE from Electron's macOS zip: Chromium's builds, self-contained (system frameworks only).
if [ ! -f "$prefix/lib/libGLESv2.dylib" ] || [ "$(cat "$prefix/angle.version" 2>/dev/null)" != "electron-$ELECTRON_VER" ]; then
  fetch "$ELECTRON_URL" "$ELECTRON_SHA" "$ELECTRON_ZIP"
  rm -rf "$work/electron" && mkdir -p "$work/electron"
  libs="Electron.app/Contents/Frameworks/Electron Framework.framework/Versions/A/Libraries"
  unzip -q "$dl/$ELECTRON_ZIP" "$libs/libEGL.dylib" "$libs/libGLESv2.dylib" LICENSE LICENSES.chromium.html -d "$work/electron"
  mkdir -p "$prefix/lib" "$prefix/share/angle"
  for f in libEGL.dylib libGLESv2.dylib; do
    cp "$work/electron/$libs/$f" "$prefix/lib/$f"
  done
  cp "$work/electron/LICENSES.chromium.html" "$prefix/share/angle/LICENSES.chromium.html"
  cp "$work/electron/LICENSE" "$prefix/share/angle/Electron-LICENSE.txt"
  printf '%s' "electron-$ELECTRON_VER" > "$prefix/angle.version"
fi

echo "build_deps: clang, SDL3, FFmpeg and ANGLE for macOS $arch are in $prefix"
