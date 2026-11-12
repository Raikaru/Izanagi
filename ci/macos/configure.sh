#!/bin/sh
# Configures a macOS build directory (docs/platform/macos.md): the 64-bit build for this Mac's architecture, compiled with the pinned
# clang of ci/macos/build_deps.sh <deps> against the SDK of the Xcode command line tools, linked by Apple's ld, with the mirror optimised
# as shipped. Extra arguments go to cmake (for example -DP4_CENSUS_CHECK=OFF while a census baseline is reviewed).
#
#   sh ci/macos/configure.sh <deps> <build dir> [cmake arguments...]
#
# Then: cmake --build <build dir> --target p4_x64 (everything the tests labelled port64 need) or --target p4pc64 (the player).
set -eu
deps=${1:?usage: configure.sh <deps> <build dir> [cmake arguments...]}
build=${2:?usage: configure.sh <deps> <build dir> [cmake arguments...]}
shift 2
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
deps=$(CDPATH= cd -- "$deps" && pwd)
deployment=${MACOSX_DEPLOYMENT_TARGET:-12.0}
case $(uname -m) in
  arm64)  triple=arm64-apple-macos$deployment ;;
  x86_64) triple=x86_64-apple-macos$deployment ;;
  *) echo "configure: unsupported architecture $(uname -m)" >&2; exit 1 ;;
esac
[ -f "$root/decomp/config/symbol_addrs.txt" ] || { echo "configure: the decomp submodule is not checked out (git submodule update --init)" >&2; exit 1; }
[ -x "$deps/llvm/bin/clang" ] || { echo "configure: $deps is not a ci/macos/build_deps.sh prefix" >&2; exit 1; }
launcher=
command -v ccache >/dev/null && launcher="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
# -fuse-ld=ld: Apple's linker (the clang release's own default is lld, which has no Mach-O driver in that package)
# shellcheck disable=SC2086
cmake -S "$root" -B "$build" -G Ninja \
  -DCMAKE_C_COMPILER="$deps/llvm/bin/clang" -DCMAKE_CXX_COMPILER="$deps/llvm/bin/clang++" \
  -DCMAKE_C_COMPILER_TARGET="$triple" -DCMAKE_CXX_COMPILER_TARGET="$triple" \
  -DCMAKE_OSX_SYSROOT="$(xcrun --sdk macosx --show-sdk-path)" -DCMAKE_OSX_DEPLOYMENT_TARGET="$deployment" \
  -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=ld -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=ld -DCMAKE_MODULE_LINKER_FLAGS=-fuse-ld=ld \
  $launcher -DP4_DECOMP="$root/decomp" -DP4_MACOS_PREFIX="$deps" \
  -DP4_PORT64=ON -DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 -DP4_PORT64_JOBS="$(sysctl -n hw.ncpu)" -DP4_PORT64_WIN=OFF "$@"
