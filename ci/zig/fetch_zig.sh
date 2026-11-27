#!/bin/sh
# Installs the pinned Zig (cmake/zig/pin.cmake: the nightly version and the SHA-256 of each official archive) into <dir>, so <dir>/zig
# is the compiler CMake takes with -DP4_ZIG=<dir>/zig (or P4_ZIG in the environment). Used by ci/macos/build_deps.sh; any Linux,
# macOS or Windows (MSYS/Git Bash) host can run it directly. ci/Dockerfile and ci/android/Dockerfile install the same archive inline.
#
#   sh ci/zig/fetch_zig.sh <dir> [host]       host: x86_64-linux, aarch64-linux, x86_64-macos, aarch64-macos, x86_64-windows
#   sh ci/zig/fetch_zig.sh --print-version    prints the pinned version (for install paths such as <prefix>/zig-<version>)
#
# An existing <dir> whose zig reports the pinned version is reused as is. Any other existing <dir> is never touched: the script
# fails, so pick an unused directory (a version-named one keeps older installs usable). The archive is unpacked beside <dir> and
# renamed into place only once it is complete and reports the pinned version, so an interrupted run leaves no <dir> behind.
# The archive is cached in $P4PC_DL_CACHE when set (ziglang.org/builds drops old nightlies: a cached archive outlives that; it is
# checked against the pinned SHA-256 on every use). A checksum mismatch or an unknown host fails; nothing falls back.
# Needs: curl, tar and xz (unzip on Windows), and sha256sum or shasum.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
pin="$root/cmake/zig/pin.cmake"
value() { sed -n "s/^set($1 *\"\\([^\"]*\\)\")\$/\\1/p" "$pin"; }
version=$(value P4_ZIG_VERSION)
[ -n "$version" ] || { echo "fetch_zig: no P4_ZIG_VERSION in $pin" >&2; exit 1; }
url_base=$(value P4_ZIG_URL_BASE)
[ -n "$url_base" ] || { echo "fetch_zig: no P4_ZIG_URL_BASE in $pin" >&2; exit 1; }
if [ "${1:-}" = --print-version ]; then
  printf '%s\n' "$version"
  exit 0
fi
dir=${1:?usage: fetch_zig.sh <dir> [host] | fetch_zig.sh --print-version}

host=${2:-}
if [ -z "$host" ]; then
  case $(uname -m) in
    x86_64|amd64) arch=x86_64 ;;
    aarch64|arm64) arch=aarch64 ;;
    *) echo "fetch_zig: unsupported host architecture $(uname -m)" >&2; exit 1 ;;
  esac
  case $(uname -s) in
    Linux) os=linux ;;
    Darwin) os=macos ;;
    MINGW*|MSYS*|CYGWIN*) os=windows ;;
    *) echo "fetch_zig: unsupported host system $(uname -s)" >&2; exit 1 ;;
  esac
  host=$arch-$os
fi
sha=$(value "P4_ZIG_SHA256_$host")
[ -n "$sha" ] || { echo "fetch_zig: no pinned archive for host $host in $pin" >&2; exit 1; }
case $host in
  *-windows) ext=zip; exe=zig.exe ;;
  *) ext=tar.xz; exe=zig ;;
esac
# The official name, "+" included (zig-x86_64-linux-0.18.0-dev.935+ead8f5a7a.tar.xz); the archive's top directory has the same name.
name=zig-$host-$version
url=$url_base/$name.$ext

if [ -x "$dir/$exe" ] && [ "$("$dir/$exe" version 2>/dev/null)" = "$version" ]; then
  echo "fetch_zig: Zig $version already in $dir"
  exit 0
fi
if [ -e "$dir" ] || [ -L "$dir" ]; then
  echo "fetch_zig: $dir exists and is not Zig $version; it is left untouched. Choose an unused install directory" >&2
  exit 1
fi

parent=$(dirname -- "$dir")
mkdir -p "$parent"
stage=
tmp_dl=
cleanup() {
  [ -z "$stage" ] || rm -rf "$stage"
  [ -z "$tmp_dl" ] || rm -rf "$tmp_dl"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

if [ -n "${P4PC_DL_CACHE:-}" ]; then
  dl=$P4PC_DL_CACHE
  mkdir -p "$dl"
else
  tmp_dl=$(mktemp -d)
  dl=$tmp_dl
fi
archive="$dl/$name.$ext"
if [ ! -f "$archive" ]; then
  curl -fsSL --proto '=https' --retry 6 --retry-all-errors --retry-delay 10 -o "$archive.part" "$url"
  mv "$archive.part" "$archive"
fi
if command -v sha256sum >/dev/null 2>&1; then
  got=$(sha256sum "$archive" | cut -d' ' -f1)
else
  got=$(shasum -a 256 "$archive" | cut -d' ' -f1)
fi
if [ "$got" != "$sha" ]; then
  rm -f "$archive"
  echo "fetch_zig: $url has SHA-256 $got, pinned $sha" >&2
  exit 1
fi

stage=$(mktemp -d "$parent/.fetch_zig.XXXXXX")
if [ "$ext" = zip ]; then
  unzip -q "$archive" -d "$stage"
else
  tar -xJf "$archive" -C "$stage"
fi
[ -d "$stage/$name" ] || { echo "fetch_zig: $archive has no top directory $name" >&2; exit 1; }
got_version=$("$stage/$name/$exe" version) || { echo "fetch_zig: the unpacked $exe does not run on this host" >&2; exit 1; }
[ "$got_version" = "$version" ] || { echo "fetch_zig: the unpacked $exe reports '$got_version', not $version" >&2; exit 1; }
if [ -e "$dir" ] || [ -L "$dir" ]; then
  echo "fetch_zig: $dir appeared during the install; it is left untouched" >&2
  exit 1
fi
mv "$stage/$name" "$dir"
echo "fetch_zig: Zig $version for $host is in $dir"
