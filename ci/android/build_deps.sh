#!/bin/sh
# Builds the Android libraries p4pc links, for one ABI: SDL3 (libSDL3.so, plus its Java sources for the app) and the LGPL FFmpeg
# subset of the movie decoder (libavcodec, libavutil, libswscale as shared libraries, which the relink condition of the LGPL needs), like
# ci/win32/build_deps.sh does for Windows.
#
#   sh ci/android/build_deps.sh <prefix> [abi]        abi: x86 (default), x86_64, arm64-v8a, armeabi-v7a
#
# <prefix>/<abi> receives include/ and lib/; <prefix>/sdl-java holds SDL's android-project (the Java classes). Needs the NDK
# (ANDROID_NDK_HOME), curl, tar, xz, cmake, ninja, make, python3 and, for FFmpeg on x86 and x86_64, nothing else (asm is off: it
# needs text relocations on 32-bit x86, which Android refuses for apps).
set -eu

prefix=${1:?usage: build_deps.sh <prefix> [abi]}
abi=${2:-x86}
ndk=${ANDROID_NDK_HOME:?ANDROID_NDK_HOME is not set}
api=24
dl=${P4PC_DL_CACHE:-$prefix/../dl-cache}

SDL_VER=3.2.10
SDL_SHA=f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0
SDL_URL=https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VER/SDL3-$SDL_VER.tar.gz
FFMPEG_VER=7.1.1
FFMPEG_SHA=733984395e0dbbe5c046abda2dc49a5544e7e0e1e2366bba849222ae9e3a03b1
FFMPEG_URL=https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VER.tar.xz

case $abi in
  x86) triple=i686-linux-android; arch=x86 ;;
  x86_64) triple=x86_64-linux-android; arch=x86_64 ;;
  arm64-v8a) triple=aarch64-linux-android; arch=aarch64 ;;
  armeabi-v7a) triple=armv7a-linux-androideabi; arch=arm ;;
  *) echo "build_deps: unknown abi $abi" >&2; exit 2 ;;
esac
tc=$ndk/toolchains/llvm/prebuilt/linux-x86_64
mkdir -p "$prefix/$abi" "$dl"
prefix=$(cd "$prefix" && pwd)
dl=$(cd "$dl" && pwd)
out=$prefix/$abi
work=$prefix/../deps-build-$abi
mkdir -p "$work"
work=$(cd "$work" && pwd)

fetch() { # url sha file
  if [ ! -f "$dl/$3" ]; then curl -fsSL -o "$dl/$3.part" "$1" && mv "$dl/$3.part" "$dl/$3"; fi
  echo "$2  $dl/$3" | sha256sum -c - >/dev/null || { echo "build_deps: checksum mismatch for $3" >&2; exit 1; }
}

# ---- SDL3
if [ ! -f "$out/lib/libSDL3.so" ]; then
  fetch "$SDL_URL" "$SDL_SHA" SDL3-$SDL_VER.tar.gz
  rm -rf "$work/sdl" && mkdir -p "$work/sdl" && tar -xzf "$dl/SDL3-$SDL_VER.tar.gz" -C "$work/sdl" --strip-components=1
  cmake -S "$work/sdl" -B "$work/sdl/build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-$api \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$out" \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_INSTALL_TESTS=OFF -DSDL_DISABLE_INSTALL_DOCS=ON
  cmake --build "$work/sdl/build"
  cmake --install "$work/sdl/build"
  rm -rf "$prefix/sdl-java" && cp -r "$work/sdl/android-project" "$prefix/sdl-java"
fi

# ---- FFmpeg (LGPL): mpeg2video + its parser (src/movie/mpeg2_dec.c), adpcm_adx (src/movie/adx_dec.c), libswscale.
FFMPEG_OPTS="--enable-decoder=mpeg2video --enable-parser=mpegvideo --enable-decoder=adpcm_adx"
if [ ! -f "$out/lib/libavcodec.so" ] || [ "$(cat "$out/ffmpeg.opts" 2>/dev/null)" != "$FFMPEG_OPTS" ]; then
  rm -f "$out"/lib/libavcodec-*.so "$out"/lib/libavutil-*.so "$out"/lib/libswscale-*.so # an earlier version of this script renamed them
  fetch "$FFMPEG_URL" "$FFMPEG_SHA" ffmpeg-$FFMPEG_VER.tar.xz
  rm -rf "$work/ffmpeg" && mkdir -p "$work/ffmpeg" && tar -xJf "$dl/ffmpeg-$FFMPEG_VER.tar.xz" -C "$work/ffmpeg" --strip-components=1
  cc=$tc/bin/$triple$api-clang
  (cd "$work/ffmpeg" && ./configure \
    --prefix="$out" --arch=$arch --target-os=android --enable-cross-compile --enable-pic \
    --cc="$cc" --ar=$tc/bin/llvm-ar --nm=$tc/bin/llvm-nm --ranlib=$tc/bin/llvm-ranlib --strip=$tc/bin/llvm-strip \
    --disable-everything --disable-programs --disable-doc --disable-network --disable-autodetect --disable-asm \
    --disable-avdevice --disable-avformat --disable-avfilter --disable-swresample \
    --enable-avcodec --enable-swscale $FFMPEG_OPTS \
    --enable-shared --disable-static --disable-debug --extra-cflags="-O2" \
  && make -j"$(nproc)" && make install) || { echo "build_deps: FFmpeg failed" >&2; exit 1; }
  printf '%s' "$FFMPEG_OPTS" > "$out/ffmpeg.opts"
fi

echo "build_deps: SDL3 and FFmpeg for $abi are in $out"
