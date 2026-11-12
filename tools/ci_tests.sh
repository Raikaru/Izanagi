#!/bin/sh
# The test suite as CI runs it, in the p4pc-ci image after a full build (BUILDING.md): every registered test except the Windows ones under
# xvfb-run, then the Windows tests (label port64win) under Wine, in a prefix bootstrapped once before they start in parallel.
#   sh tools/ci_tests.sh <build dir> [ctest args...]
# Wine tests that time out are run once more (--repeat after-timeout:2): their short timeouts are for a quiet machine, and many
# Wine processes starting in parallel can stall one past it. A test that fails is not rerun.
# Tests that need the game skip (exit 77) unless P4PC_ISO and P4PC_ELF name your disc image and SLUS_217.82.
set -eu
build=${1:?usage: sh tools/ci_tests.sh <build dir> [ctest args...]}
shift
build=$(CDPATH= cd -- "$build" && pwd)
jobs=${P4PC_JOBS:-$(nproc)}
rc=0
xvfb-run -a ctest --test-dir "$build" -j "$jobs" --timeout 900 --output-on-failure -LE port64win "$@" || rc=$?
(
  unset DISPLAY WAYLAND_DISPLAY
  export WINEPREFIX="$build/wineprefix" WINEARCH=win64 WINEDEBUG=-all WINEDLLOVERRIDES="winedbg.exe=d;mscoree,mshtml="
  mkdir -p "$WINEPREFIX"
  trap 'wineserver -k >/dev/null 2>&1 || true' EXIT
  wineserver -p </dev/null >/dev/null 2>&1
  timeout -s KILL 300 wineboot --init </dev/null >/dev/null 2>&1
  ctest --test-dir "$build" -j "$jobs" --timeout 900 --output-on-failure --repeat after-timeout:2 -L port64win "$@"
) || rc=$?
exit $rc
