#!/bin/sh
# One part of the build workflow's Linux and Windows work (.github/workflows/build.yml), run in the p4pc-ci image (ci/Dockerfile) from the
# checkout root, with the decomp submodule:
#   sh tools/ci_build.sh <part> [--out DIR]
# The parts run in parallel jobs. Each configures the tree as the single job it replaced did, builds only what its tests need and runs
# its share of the tests (labels, see below); together they run every test that one job ran, which the coverage part checks.
#   x64        Linux x86_64: target p4_x64, then the tests labelled port64 (the Windows ones aside) under xvfb, and the host tests that exist
#              only in the 64-bit build (those the 32-bit-only configuration of the host part does not register; their programs are built
#              first, p4_x64 does not hold them).
#   win64      Windows x64: target p4_x64w (cross-built with llvm-mingw), the Wine tests (label port64win), then the Windows zip
#              (tools/package_win.sh) from the same build directory, into <out> (default dist).
#   host       the 32-bit host build with the 64-bit build off, and every test not labelled port64 or port64win, under xvfb.
#   appimage   the Linux x86_64 AppImage (tools/package_linux.sh) into <out> (default dist).
#   coverage   configures the tree both ways and fails unless the three test parts above select every registered test between them.
# Environment: P4PC_BUILD_TOOLCHAIN_ID=sha256:<the image ID> turns on the generated-source cache under /ccache (tools/build_cache.sh);
# /ccache holds ccache's directory and that cache; the build directory is build/ (the part's own, nothing is shared between jobs but the caches).
set -eu
part=${1:?usage: sh tools/ci_build.sh <x64|win64|host|appimage|coverage> [--out DIR]}
shift
out=dist
while [ $# -gt 0 ]; do
  case $1 in
    --out) out=${2:?--out needs a folder}; shift 2 ;;
    *) echo "ci_build: unknown argument $1" >&2; exit 2 ;;
  esac
done
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$here/.." && pwd)
cd "$root"
jobs=${P4PC_JOBS:-$(nproc)}
cc=${P4PC_LLVM_MINGW:-/opt/llvm-mingw}/bin/clang
build=${P4PC_CI_BUILD:-$root/build}
mkdir -p "$build"
. "$here/build_cache.sh"

# Both configurations; ctest registers the same tests for the 64-bit options, which only add targets and the tests of those targets.
configure_64() {   # configure_64 <dir>: the options the single job used (plus the job pool of the package scripts, which changes no command)
  cmake -S "$root" -B "$1" -G Ninja -DCMAKE_C_COMPILER="$cc" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DP4_PORT64=ON -DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 -DP4_PORT64W_OPT=O2 -DP4_PORT64_JOBS="$jobs" -DP4_CENSUS_CHECK=ON
}
configure_host() { # configure_host <dir>: no 64-bit build (-DP4_PORT64 defaults to ON, which would put the 64-bit and Windows x64 mirrors in `all`: the
  # x64 and win64 parts build those; the registered host tests are the same without them, which the coverage part checks)
  cmake -S "$root" -B "$1" -G Ninja -DCMAKE_C_COMPILER="$cc" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DP4_PORT64=OFF -DP4_CENSUS_CHECK=ON
}
names() {          # names <build dir> <ctest selection args...>: the registered tests the selection names, one per line
  dir=$1; shift
  ctest --test-dir "$dir" -N "$@" | sed -n 's/^ *Test *#[0-9]*: //p' | sort -u
}
# The label selections of the parts.
x64_sel="-L ^port64\$ -LE ^port64win\$"
win_sel="-L ^port64win\$"
host_sel="-LE ^(port64|port64win)\$"
# Host-labelled tests the 32-bit-only configuration does not register: their programs are 64-bit ones.
x64_extras() {     # x64_extras <64-bit build dir> <host-only build dir>: the names only the first registers (the second's twice, so uniq -u drops them)
  # shellcheck disable=SC2086
  { names "$1" $host_sel; names "$2" $host_sel; names "$2" $host_sel; } | sort | uniq -u
}
extras_regex() {   # an anchored alternation of the names in stdin, nothing for none
  awk '{ r = r (NR > 1 ? "|" : "") $0 } END { if (NR) print "^(" r ")$" }'
}
count() { wc -l | tr -d ' '; }

case $part in
  x64)
    p4pc_build_cache_profile linux-x86_64 "$build/build-trace"
    configure_64 "$build"
    configure_host "$build/listing-host" > /dev/null
    extras=$(x64_extras "$build" "$build/listing-host" | extras_regex)
    # the programs of the host tests of the 64-bit build, which p4_x64 does not build (ctest names the ones it cannot find before they are built)
    missing=
    [ -z "$extras" ] || missing=$(ctest --test-dir "$build" -N -V -R "$extras" 2>&1 | sed -n "s|^Could not find executable $build/||p" | sort -u)
    # shellcheck disable=SC2086
    cmake --build "$build" --target p4_x64 $missing -- -k 0
    # shellcheck disable=SC2086
    echo "ci_build: x64 runs $(names "$build" $x64_sel | count) tests (port64) and the host tests of the 64-bit build: ${extras:-none}"
    rc=0
    # shellcheck disable=SC2086
    sh "$here/ci_tests.sh" "$build" --plain $x64_sel || rc=$?
    if [ -n "$extras" ]; then sh "$here/ci_tests.sh" "$build" --plain -R "$extras" || rc=$?; fi
    exit $rc
    ;;
  win64)
    p4pc_build_cache_profile windows-x86_64 "$build/build-trace"
    configure_64 "$build"
    cmake --build "$build" --target p4_x64w -- -k 0
    # shellcheck disable=SC2046
    echo "ci_build: win64 runs $(names "$build" $win_sel | count) tests (port64win)"
    rc=0
    sh "$here/ci_tests.sh" "$build" --wine || rc=$?
    # The package of this tree: its configure adds nothing to the build that is already done.
    sh "$here/package_win.sh" --build "$build" --out "$out" --jobs "$jobs" || rc=$?
    exit $rc
    ;;
  host)
    p4pc_build_cache_profile linux-i386 "$build/build-trace"
    configure_host "$build"
    cmake --build "$build" -- -k 0
    # shellcheck disable=SC2086
    echo "ci_build: host runs $(names "$build" $host_sel | count) tests"
    # shellcheck disable=SC2086
    sh "$here/ci_tests.sh" "$build" --plain $host_sel
    ;;
  appimage)
    p4pc_build_cache_profile linux-x86_64 "$build/build-trace"
    sh "$here/package_linux.sh" --work "$build/package-linux" --out "$out" --jobs "$jobs"
    ;;
  coverage)
    configure_64 "$build/on" > /dev/null
    configure_host "$build/host" > /dev/null
    names "$build/on" > "$build/all.txt"
    # shellcheck disable=SC2086
    { names "$build/on" $x64_sel; x64_extras "$build/on" "$build/host"; names "$build/on" $win_sel; names "$build/host" $host_sel; } | sort -u > "$build/parts.txt"
    echo "ci_build: $(count < "$build/all.txt") tests registered, $(count < "$build/parts.txt") selected by the parts"
    # shellcheck disable=SC2086
    echo "ci_build: x64 $(names "$build/on" $x64_sel | count) + extras $(x64_extras "$build/on" "$build/host" | count), win64 $(names "$build/on" $win_sel | count), host $(names "$build/host" $host_sel | count)"
    if ! diff "$build/all.txt" "$build/parts.txt"; then
      echo "ci_build: the parts do not run exactly the registered tests (< registered only, > selected only); change the selections in tools/ci_build.sh" >&2
      exit 1
    fi
    echo "ci_build: every registered test is selected"
    ;;
  *)
    echo "ci_build: unknown part $part" >&2
    exit 2
    ;;
esac
