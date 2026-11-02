#!/bin/sh
# Builds the Windows (i686-w64-mingw32 or x86_64-w64-mingw32, UCRT) or, with the triple x86_64-linux-gnu, the native Linux x86_64 libraries p4pc links: SDL3 and the LGPL
# FFmpeg subset the movie decoder uses (libavcodec with the MPEG-2 decoder and parser, libavutil,
# libswscale), as shared libraries (DLLs) so the LGPL relink condition holds like the Linux build.
#
#   sh ci/win32/build_deps.sh <prefix> [llvm-mingw-dir [triple]]
#   sh ci/win32/build_deps.sh <prefix> /usr x86_64-linux-gnu      (tools/package_linux.sh: <prefix>/lib/libSDL3.so.0, libavcodec.so.61 ...)
#
# <prefix> receives include/, lib/ (import libraries) and bin/ (SDL3.dll, avcodec-61.dll, ...).
# Source tarballs are fetched into <prefix>/../dl-cache (or $P4PC_DL_CACHE) and checked against
# the pinned sha256 below. llvm-mingw (clang, lld, mingw-w64 UCRT headers/CRT) is the toolchain
# the Windows build uses; its location comes from the second argument or $P4PC_LLVM_MINGW
# (default /opt/llvm-mingw). Needs: curl, tar, xz, cmake, ninja, make, nasm, python3.
set -eu

prefix=${1:?usage: build_deps.sh <prefix> [llvm-mingw-dir]}
llvm=${2:-${P4PC_LLVM_MINGW:-/opt/llvm-mingw}}
dl=${P4PC_DL_CACHE:-$prefix/../dl-cache}
triple=${3:-i686-w64-mingw32}
case $triple in
  i686-w64-mingw32)   sysproc=i686;  ffarch=x86 ;;
  x86_64-w64-mingw32) sysproc=AMD64; ffarch=x86_64 ;;
  x86_64-linux-gnu)   sysproc=x86_64; ffarch=x86_64 ;;
  aarch64-linux-gnu) sysproc=aarch64; ffarch=aarch64 ;;
  *) echo "build_deps: unsupported triple $triple" >&2; exit 1 ;;
esac

SDL_VER=3.2.10
SDL_SHA=f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0
SDL_URL=https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VER/SDL3-$SDL_VER.tar.gz
FFMPEG_VER=7.1.1
FFMPEG_SHA=733984395e0dbbe5c046abda2dc49a5544e7e0e1e2366bba849222ae9e3a03b1
FFMPEG_URL=https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VER.tar.xz

export PATH="$llvm/bin:$PATH"
native=0; linux_cross=0
case $triple in x86_64-linux-gnu) native=1 ;; aarch64-linux-gnu) native=1; linux_cross=1 ;; esac
if [ $native = 1 ]; then
  cc=clang; cxx=clang++
  if [ $linux_cross = 1 ]; then cc=aarch64-linux-gnu-gcc; cxx=aarch64-linux-gnu-g++; fi
  sdl_sentinel=lib/libSDL3.so; av_sentinel=lib/libavcodec.so
else cc=$triple-clang; cxx=$triple-clang++; sdl_sentinel=bin/SDL3.dll; av_sentinel=bin/avcodec-61.dll; fi
command -v $cc >/dev/null || { echo "build_deps: $cc not found under $llvm/bin" >&2; exit 1; }
mkdir -p "$prefix" "$dl"
prefix=$(cd "$prefix" && pwd)
dl=$(cd "$dl" && pwd)
work=$prefix/../deps-build-$triple
mkdir -p "$work"
work=$(cd "$work" && pwd)

fetch() { # url sha file
  if [ ! -f "$dl/$3" ]; then curl -fsSL --retry 6 --retry-all-errors --retry-delay 10 -o "$dl/$3.part" "$1" && mv "$dl/$3.part" "$dl/$3"; fi
  echo "$2  $dl/$3" | sha256sum -c - >/dev/null || { echo "build_deps: checksum mismatch for $3" >&2; exit 1; }
}

# ---- SDL3
if [ ! -f "$prefix/$sdl_sentinel" ]; then
  fetch "$SDL_URL" "$SDL_SHA" SDL3-$SDL_VER.tar.gz
  rm -rf "$work/sdl" && mkdir -p "$work/sdl" && tar -xzf "$dl/SDL3-$SDL_VER.tar.gz" -C "$work/sdl" --strip-components=1
  if [ $native = 1 ]; then sdl_target="-DCMAKE_C_COMPILER=$cc -DCMAKE_CXX_COMPILER=$cxx"
  else sdl_target="-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_PROCESSOR=$sysproc -DCMAKE_C_COMPILER=$cc -DCMAKE_CXX_COMPILER=$cxx -DCMAKE_RC_COMPILER=$triple-windres"; fi
  if [ $linux_cross = 1 ]; then
    sdl_target="-DCMAKE_C_COMPILER=$cc -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_LIBRARY_ARCHITECTURE=aarch64-linux-gnu"
    export PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig
  fi
  # shellcheck disable=SC2086
  cmake -S "$work/sdl" -B "$work/sdl/build" -G Ninja $sdl_target \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_INSTALL_TESTS=OFF \
    -DSDL_DISABLE_INSTALL_DOCS=ON
  cmake --build "$work/sdl/build"
  cmake --install "$work/sdl/build"
fi

# ---- FFmpeg (LGPL): the decoders the port uses. mpeg2video + its parser (src/movie/mpeg2_dec.c) and
# adpcm_adx (src/movie/adx_dec.c), libswscale for the picture conversion. Add a decoder here when a
# source starts using one: a changed option list rebuilds FFmpeg.
FFMPEG_OPTS="--enable-decoder=mpeg2video --enable-parser=mpegvideo --enable-decoder=adpcm_adx"
if [ ! -f "$prefix/$av_sentinel" ] || [ "$(cat "$prefix/ffmpeg.opts" 2>/dev/null)" != "$FFMPEG_OPTS" ]; then
  fetch "$FFMPEG_URL" "$FFMPEG_SHA" ffmpeg-$FFMPEG_VER.tar.xz
  rm -rf "$work/ffmpeg" && mkdir -p "$work/ffmpeg" && tar -xJf "$dl/ffmpeg-$FFMPEG_VER.tar.xz" -C "$work/ffmpeg" --strip-components=1
  if [ $native = 1 ]; then ff_target="--arch=$ffarch --target-os=linux --cc=$cc"
  else ff_target="--arch=$ffarch --target-os=mingw32 --enable-cross-compile --cross-prefix=$triple- --cc=$cc --ar=llvm-ar --nm=llvm-nm --ranlib=llvm-ranlib --strip=llvm-strip"; fi
  if [ $linux_cross = 1 ]; then
    ff_target="--arch=aarch64 --target-os=linux --enable-cross-compile --cross-prefix=aarch64-linux-gnu- --cc=$cc"
  fi
  # shellcheck disable=SC2086
  (cd "$work/ffmpeg" && ./configure \
    --prefix="$prefix" $ff_target \
    --disable-everything --disable-programs --disable-doc --disable-network --disable-autodetect \
    --disable-avdevice --disable-avformat --disable-avfilter --disable-swresample \
    --enable-avcodec --enable-swscale $FFMPEG_OPTS \
    --enable-shared --disable-static --disable-debug \
    --extra-cflags="-O2" && make -j"$(nproc)" && make install) && printf '%s' "$FFMPEG_OPTS" > "$prefix/ffmpeg.opts"
fi

echo "build_deps: SDL3 and FFmpeg for $triple are in $prefix"
