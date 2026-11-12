#!/bin/sh
# Build an isolated SDL3 variant for an adb-shell runner, never the APK's SDL.
# Usage: build_phone_sdl.sh <existing-deps-prefix> <phone-cache> <patch>
set -eu
base=${1:?existing Android deps prefix required}
cache=${2:?phone cache required}
patch_file=${3:?SDL patch required}
ndk=${ANDROID_NDK_HOME:?ANDROID_NDK_HOME is required}
tarball=${P4PC_DL_CACHE:?P4PC_DL_CACHE is required}/SDL3-3.2.10.tar.gz
[ -f "$base/arm64-v8a/lib/libSDL3.so" ] && [ -f "$tarball" ] || {
  echo 'phone SDL: build the normal arm64 Android dependencies first' >&2; exit 1;
}
printf '%s  %s\n' f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0 "$tarball" | sha256sum -c - >/dev/null
# Inputs name an immutable cache entry; the normal app dependencies are never modified.
key=$({ cat "$patch_file" "$ndk/source.properties" "$0"; sha256sum "$tarball"; \
  (cd "$base/arm64-v8a" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum; \
    find . -type l -printf '%p -> %l\n' | LC_ALL=C sort); } | sha256sum | cut -d ' ' -f 1)
dest=$cache/$key
mkdir -p "$cache"
verify_cache() {
  [ -f "$dest/complete" ] && [ "$(cat "$dest/complete")" = "$key" ] &&
    [ -f "$dest/files.sha256" ] && [ -f "$dest/links.txt" ] || return 1
  (cd "$dest" &&
    sha256sum -c files.sha256 >/dev/null &&
    find arm64-v8a -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum | cmp -s - files.sha256) &&
    (cd "$dest" && find arm64-v8a -type l -printf '%p -> %l\n' | LC_ALL=C sort | cmp -s - links.txt)
}
if [ -e "$dest" ]; then
  verify_cache || { echo 'phone SDL: cached dependencies failed verification' >&2; exit 1; }
  printf '%s\n' "$dest"
  exit 0
fi
stage=$(mktemp -d "$cache/.phone-sdl.XXXXXXXX")
trap 'rm -rf "$stage"' EXIT HUP INT TERM
mkdir -p "$stage/source" "$stage/prefix"
tar -xzf "$tarball" -C "$stage/source" --strip-components=1
(cd "$stage/source" && git apply "$patch_file")
cp -a "$base/arm64-v8a" "$stage/prefix/arm64-v8a"
cmake -S "$stage/source" -B "$stage/build" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$stage/prefix/arm64-v8a" -DSDL_SHARED=ON -DSDL_STATIC=OFF \
  -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_INSTALL_TESTS=OFF -DSDL_DISABLE_INSTALL_DOCS=ON >&2
cmake --build "$stage/build" --parallel "${P4PC_JOBS:-2}" >&2
cmake --install "$stage/build" >&2
(cd "$stage/prefix" && find arm64-v8a -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > files.sha256)
(cd "$stage/prefix" && find arm64-v8a -type l -printf '%p -> %l\n' | LC_ALL=C sort > links.txt)
printf '%s\n' "$key" > "$stage/prefix/complete"
if ! mv -T "$stage/prefix" "$dest" 2>/dev/null; then
  verify_cache || { echo 'phone SDL: concurrent cache failed verification' >&2; exit 1; }
fi
printf '%s\n' "$dest"
