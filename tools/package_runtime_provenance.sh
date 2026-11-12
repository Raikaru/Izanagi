#!/bin/sh
# Record the actual static C++ runtime selected by the package build toolchain.
# Usage: package_runtime_provenance.sh linux|windows|android <target-triple> <output>
set -eu
platform=${1:?} target=${2:?} out=${3:?}
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
case $platform in
  linux) cxx=/usr/bin/clang++; target_arg="--target=$target" ;;
  windows) cxx="${P4PC_LLVM_MINGW:-/opt/llvm-mingw}/bin/$target-clang++"; target_arg= ;;
  android) cxx="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++"; target_arg="--target=${target}24" ;;
  *) echo "unknown runtime platform: $platform" >&2; exit 2 ;;
esac
archive() {
  file=$($cxx $target_arg -print-file-name="$1")
  [ -f "$file" ] || { echo "missing runtime archive $1: $file" >&2; exit 1; }
  sha256sum "$file"
  if [ "$platform" = linux ]; then
    owner=$(dpkg-query -S "$file")
    package=${owner%: *}
    printf 'Debian package: %s\n' "$package"
    dpkg-query -s "$package"
  fi
}
{
  printf 'Static runtime provenance for %s (%s)\n\nCompiler:\n' "$platform" "$target"
  "$cxx" $target_arg --version
  printf '\nArchive SHA-256 and paths (toolchain inputs; not separate shipped DLLs):\n'
  case $platform in
    linux)
      archive libstdc++.a
      archive libgcc.a
      printf '\nSource packages and exact versions are in the Debian Source and Built-Using fields above.\n'
      printf 'Debian source archives: https://snapshot.debian.org/package/\n'
      ;;
    windows)
      archive libc++.a
      archive libc++abi.a
      archive libunwind.a
      archive libwinpthread.a
      printf '\nLLVM project source revision is printed in the compiler version above:\nhttps://github.com/llvm/llvm-project\n'
      printf 'llvm-mingw toolchain and build recipes: https://github.com/mstorsjo/llvm-mingw\n'
      printf 'mingw-w64/winpthreads source: https://sourceforge.net/projects/mingw-w64/\n'
      printf 'Source checkout toolchain image recipe (release and checksum):\n'
      sed -n '/^ARG LLVM_MINGW_RELEASE=/p; /^ARG LLVM_MINGW_SHA256=/p' "$root/ci/Dockerfile"
      ;;
    android)
      archive libc++_static.a
      archive libc++abi.a
      archive libunwind.a
      printf '\nNDK version:\n'
      cat "$ANDROID_NDK_HOME/source.properties"
      printf '\nAndroid LLVM source revision and cherry-pick information:\n'
      cat "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/clang_source_info.md"
      printf '\nAndroid LLVM source: https://android.googlesource.com/toolchain/llvm-project/\n'
      ;;
  esac
  printf '\nCompiler support archive:\n'
  support=$($cxx $target_arg -print-libgcc-file-name)
  [ -f "$support" ] || { echo "missing compiler support archive: $support" >&2; exit 1; }
  sha256sum "$support"
  if [ "$platform" = linux ]; then
    owner=$(dpkg-query -S "$support")
    dpkg-query -s "${owner%: *}"
  fi
} > "$out"
