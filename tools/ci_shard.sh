#!/bin/sh
# One land check shard, the part that needs no game files, as the land workflow (.github/workflows/land.yml) runs it in the p4pc-ci
# image (a64: ci/Dockerfile.arm64 on an ARM runner; macos: GitHub's Macs, no container) on a pushed land/<id> branch:
#   sh tools/ci_shard.sh <x64|win32|a64|mobile|macos> <build dir> <output dir>
#   sh tools/ci_shard.sh toolchain [a64|mobile|macos]   (one JSON line: the compilers and Wine a shard's results depend on)
# x64: the Linux x86_64 build (target p4_x64) and its tests labelled port64, the 64-bit scene gates and the Windows coroutine test aside.
# win32: the 32-bit Windows build (cmake/toolchain-win32.cmake) and its host tests under Wine, those labelled gl aside.
# a64: the Linux AArch64 build (target p4_x64, its census included) compiled and run natively, and the a64 scope's tests: port64 without the
# slow, gl and Windows ones.
# mobile: the Android arm64 generation and census (target p4decomp64_generate) and the SDK-less iOS renderer syntax check, with the NDK of
# ci/android/Dockerfile (ANDROID_NDK_HOME) and SDL3/FFmpeg from ci/android/build_deps.sh in P4PC_ANDROID_CACHE: nothing is linked or run.
# macos: the rewriter census of this Mac's architecture (target p4decomp64 against config/port64macos_<arch>_census.txt), with the pinned clang
# of ci/macos/build_deps.sh in DEPS; it reports one test, census_macos_<arch>, failed only when the rule counts differ (the diff is in
# census.log and census-refresh.txt), and any other configure or build failure as a failed build.
# Those are the land check's own selections. Without P4_RETAIL the tests that read the retail executable are not registered, and the
# ones that need the disc skip (exit 77): the land check runs those itself. <output dir> gets result.json (the configure/build and CTest
# return codes, phase seconds, the toolchain, the selection and every selected test's status) and the logs; the land runner downloads
# them, compares the toolchain with its own image's, then deletes the branch and its run.
set -u
toolchain() {   # toolchain [a64]: the a64 build's compiler is Debian's clang, whose target line names the host, so only its version counts
  python3 - "${1:-}" <<'EOF'
import json, os, subprocess, sys
def version(*command, lines=2):
    try:
        output = subprocess.run(command, capture_output=True, text=True, timeout=120).stdout.splitlines()
    except (OSError, subprocess.SubprocessError):
        return None
    return " ".join(line.strip() for line in output[:lines]) or None
if sys.argv[1] == "a64":
    found = {"clang": version("/usr/bin/clang", "--version", lines=1), "cmake": version("cmake", "--version")}
elif sys.argv[1] == "mobile":
    ndk = os.environ.get("ANDROID_NDK_HOME", "")
    found = {"clang": version(ndk + "/toolchains/llvm/prebuilt/linux-x86_64/bin/clang", "--version", lines=1),
             "cmake": version("cmake", "--version")}
elif sys.argv[1] == "macos":
    found = {"clang": version(os.environ.get("DEPS", "") + "/llvm/bin/clang", "--version", lines=1),
             "cmake": version("cmake", "--version")}
else:
    found = {"clang": version("/opt/llvm-mingw/bin/clang", "--version"),
             "mingw": version("/opt/llvm-mingw/bin/i686-w64-mingw32-clang", "--version"),
             "cmake": version("cmake", "--version"), "wine": version("wine", "--version")}
print(json.dumps(found, sort_keys=True, separators=(",", ":")))
EOF
}
if [ "${1:-}" = toolchain ]; then
  toolchain "${2:-}"
  exit $?
fi
usage="usage: sh tools/ci_shard.sh <x64|win32|a64|mobile|macos> <build dir> <output dir>"
shard=${1:?$usage}
build=${2:?$usage}
out=${3:?$usage}
src=$(pwd)
jobs=${P4PC_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu)}
mkdir -p "$build" "$out" || exit 1
build=$(CDPATH= cd -- "$build" && pwd)   # absolute: Wine refuses a relative WINEPREFIX below it
out=$(CDPATH= cd -- "$out" && pwd)
now() { date +%s; }
phases=
build_rc=0
ctest_rc=
step() {   # step <phase> <log> <command...>: runs while the shard is green, timing it into result.json
  local name=$1 log=$2 t
  shift 2
  [ "$build_rc" -eq 0 ] || return 0
  t=$(now)
  "$@" > "$out/$log" 2>&1 || build_rc=$?
  phases="$phases $name=$(( $(now) - t ))"
  echo "ci_shard: $shard $name rc=$build_rc secs=$(( $(now) - t ))"
  [ "$build_rc" -eq 0 ] || tail -n 60 "$out/$log"
}
case $shard in
  x64)
    tree=$build
    step configure configure.log cmake -S "$src" -B "$tree" -G Ninja -DCMAKE_C_COMPILER=/opt/llvm-mingw/bin/clang \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DP4_PORT64=ON -DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 -DP4_PORT64_JOBS="$jobs" -DP4_DECOMP="$src/decomp" -DP4_CENSUS_CHECK=ON
    step build build.log cmake --build "$tree" --target p4_x64 -- -k 0
    set -- -L '^port64$' -LE '^(scenes64|gl)$' -E '^port64_coro_win$'
    ;;
  win32)
    tree=$build/win32
    mkdir -p "$tree" || exit 1
    step configure configure.log cmake -S "$src" -B "$tree" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$src/cmake/toolchain-win32.cmake" \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_CROSSCOMPILING_EMULATOR=wine \
      -DP4_WIN32_PREFIX=/opt/p4win32 -DP4_DECOMP="$src/decomp" -DP4_OPT=O2 -DP4_DEBUG_INFO=g1 -DP4_CENSUS_CHECK=ON
    step build build.log cmake --build "$tree" -- -k 0
    set -- -LE gl
    ;;
  a64)
    tree=$build
    step configure configure.log cmake -S "$src" -B "$tree" -G Ninja -DCMAKE_C_COMPILER=/usr/bin/clang \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DP4_LIBC32=/usr/lib/i386-linux-gnu/libc.so.6 \
      -DP4_PORT64=ON -DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 -DP4_PORT64_JOBS="$jobs" -DP4_PORT64_ARCH=aarch64 -DP4_PORT64_WIN=OFF \
      -DP4_DECOMP="$src/decomp" -DP4_CENSUS_CHECK=ON
    step build build.log cmake --build "$tree" --target p4_x64 -- -k 0
    set -- -L '^port64$' -LE '^(slow|gl)$' -E '^port64_coro_win$'
    ;;
  mobile)
    tree=$build
    ndk=${ANDROID_NDK_HOME:?ANDROID_NDK_HOME names the NDK of ci/android/Dockerfile}
    cache=${P4PC_ANDROID_CACHE:?P4PC_ANDROID_CACHE holds the Android SDL3 and FFmpeg}
    export P4PC_DL_CACHE="$cache/dl"
    step deps deps.log sh "$src/ci/android/build_deps.sh" "$cache/deps" arm64-v8a
    step configure configure.log cmake -S "$src" -B "$tree" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_static -DP4_ANDROID_PREFIX="$cache/deps" \
      -DP4_DECOMP="$src/decomp" -DP4_PORT64=ON -DP4_PORT64_LINK=ON -DP4_PORT64_WIN=OFF -DP4_PORT64_JOBS="$jobs" -DP4_CENSUS_CHECK=ON
    step generation generation.log cmake --build "$tree" --target p4decomp64_generate --parallel "$jobs"
    step ios ios.log sh "$src/tools/ios_check.sh" "$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" \
      "$cache/deps/arm64-v8a/include" "$ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include" "$src"
    [ "$build_rc" -ne 0 ] || ctest_rc=0   # a generation guard: no tests
    ;;
  macos)
    tree=$build
    deps=${DEPS:?DEPS is the ci/macos/build_deps.sh prefix}
    arch=$(uname -m)
    census=$arch
    [ "$arch" != arm64 ] || census=aarch64
    test=census_macos_$arch
    step configure configure.log sh "$src/ci/macos/configure.sh" "$deps" "$tree" -DP4_CENSUS_CHECK=ON
    if [ "$build_rc" -eq 0 ]; then
      t=$(now)
      cmake --build "$tree" --target p4decomp64 -- -k 0 > "$out/census.log" 2>&1
      rc=$?
      phases="$phases census=$(( $(now) - t ))"
      echo "ci_shard: macos census rc=$rc secs=$(( $(now) - t ))"
      printf '{"tests": [{"name": "%s"}]}\n' "$test" > "$out/selection.json"
      if [ $rc -eq 0 ]; then
        echo "1/1 Test #1: $test ....   Passed    0.00 sec" > "$out/ctest.log"
        ctest_rc=0
      elif grep -q 'rewriter rule counts differ' "$out/census.log"; then
        grep -E 'rewriter rule counts differ|^[-+]' "$out/census.log" | head -n 80
        python3 "$src/tools/port64/unit64.py" report --dir "$tree/gen/obj64/units" --census-refresh "$out/census-refresh.txt" \
          --clang "$deps/llvm/bin/clang" --active-units "$tree/gen/obj64/active_units.txt" > /dev/null 2>&1 || :
        echo "1/1 Test #1: $test ....***Failed    0.00 sec" > "$out/ctest.log"
        ctest_rc=1
      else
        tail -n 60 "$out/census.log"
        build_rc=$rc
      fi
    fi
    ;;
  *)
    echo "$usage" >&2
    exit 2
    ;;
esac
if [ "$build_rc" -eq 0 ] && [ "$shard" != mobile ] && [ "$shard" != macos ]; then
  ctest --test-dir "$tree" --show-only=json-v1 "$@" > "$out/selection.json" || build_rc=$?
fi
if [ "$build_rc" -eq 0 ] && [ "$shard" != mobile ] && [ "$shard" != macos ]; then
  t=$(now)
  # junit.xml keeps each test's output (head and tail of a passed one): a test that passes with skipped subtests (unittest's
  # "OK (skipped=N)", e.g. mod_resources_boot without the disc) is a partial pass, which the land runs itself.
  set -- "$@" --output-junit "$out/junit.xml" --test-output-size-passed 4096 --test-output-truncation middle
  if [ "$shard" != win32 ]; then
    xvfb-run -a ctest --test-dir "$tree" -j "$jobs" --output-on-failure "$@" > "$out/ctest.log" 2>&1
    ctest_rc=$?
  else
    # Wine runs headless (no display, no crash dialog) in a prefix of its own, bootstrapped before the tests start in parallel.
    (
      unset DISPLAY WAYLAND_DISPLAY
      export WINEPREFIX="$tree/wineprefix" WINEDEBUG=-all WINEDLLOVERRIDES="winedbg.exe=d;mscoree,mshtml="
      mkdir -p "$WINEPREFIX"
      trap 'wineserver -k >/dev/null 2>&1 || true' EXIT
      wineserver -p </dev/null >/dev/null 2>&1
      timeout -s KILL 300 wineboot --init </dev/null >/dev/null 2>&1
      timeout -s KILL 1500 ctest --test-dir "$tree" -j "$jobs" --timeout 300 --output-on-failure "$@"
    ) > "$out/ctest.log" 2>&1
    ctest_rc=$?
  fi
  phases="$phases ctest=$(( $(now) - t ))"
  echo "ci_shard: $shard ctest rc=$ctest_rc secs=$(( $(now) - t )) $(grep 'tests passed' "$out/ctest.log" | tail -n 1)"
  grep -E '\*\*\*|Not Run' "$out/ctest.log" | grep -v '\*\*\*Skipped' | head -n 40
fi
python3 - "$shard" "$build_rc" "${ctest_rc:-}" "$out" "$phases" "$(git -C "$src" rev-parse HEAD)" "${P4PC_IMAGE_ID:-}" "$(toolchain "$shard")" <<'EOF'
import json, re, sys
from pathlib import Path
shard, build_rc, ctest_rc, out, phases, commit, image, toolchain = sys.argv[1:]
out = Path(out)
row = re.compile(r"^\s*\d+/\d+\s+Test\s+#\d+:\s+(\S+)\s+\.*\s*(Passed|\*\*\*Skipped|\*\*\*Failed|\*\*\*Timeout|\*\*\*Exception|Not Run|\*\*\*Not Run)")
tests, selected, summary = {}, [], None
if (out / "selection.json").is_file():
    selected = sorted(test["name"] for test in json.loads((out / "selection.json").read_text())["tests"])
if (out / "ctest.log").is_file():
    for line in (out / "ctest.log").read_text(errors="replace").splitlines():
        match = row.match(line)
        if match:
            word = match.group(2)
            tests[match.group(1)] = "passed" if word == "Passed" else "skipped" if word == "***Skipped" else "failed"
        elif "tests passed" in line:
            summary = line.strip()
if (out / "junit.xml").is_file():
    import xml.etree.ElementTree as ET
    for case in ET.parse(out / "junit.xml").getroot().iter("testcase"):
        text = "".join(node.text or "" for node in case.iter("system-out"))
        if tests.get(case.get("name")) == "passed" and re.search(r"^OK \(.*skipped=[1-9]", text, re.MULTILINE):
            tests[case.get("name")] = "partial"
for name in selected:
    tests.setdefault(name, "missing")
result = {"schema": 1, "shard": shard, "commit": commit, "image": image or None, "toolchain": json.loads(toolchain or "null"),
          "build_rc": int(build_rc), "ctest_rc": int(ctest_rc) if ctest_rc else None,
          "rc": 0 if build_rc == "0" and ctest_rc == "0" else 1,
          "phases": {name: int(seconds) for name, seconds in (item.split("=") for item in phases.split())},
          "summary": summary, "selected": selected, "tests": tests,
          "failing": sorted(name for name, status in tests.items() if status in ("failed", "missing"))}
(out / "result.json").write_text(json.dumps(result, sort_keys=True, indent=1) + "\n")
print("ci_shard: %s rc=%d %d selected, %d passed, %d skipped, failing %s" % (
    shard, result["rc"], len(selected), sum(s == "passed" for s in tests.values()),
    sum(s == "skipped" for s in tests.values()), result["failing"] or "none"))
sys.exit(result["rc"])
EOF
