#!/bin/sh
# Packages the Windows x64 player build: p4pc64w.exe, the DLLs it loads, p4mod.exe (the modder's CLI, docs/modding/p4mod.md) with
# the p4.d.luau and capability index it reads, a README, the licence notices and the Luau source, as one zip.
#
#   sh tools/package_win.sh [--build DIR] [--out DIR] [--jobs N] [--version NAME]
#
# Runs in the p4pc-ci image (ci/Dockerfile), from a checkout with the decomp submodule: llvm-mingw cross-builds p4pc64w in --build
# (configured here with the 64-bit build, -DP4_PORT64=ON) against SDL3 and the LGPL-only FFmpeg of /opt/p4win64 (ci/win32/build_deps.sh).
# Checks before it zips: the exe imports only system DLLs and the shipped ones (tools/check_package_imports.py), and the FFmpeg DLLs
# are LGPL-only (tests/ffmpeg_license_test.c, built as ffmpeg_license_test64w.exe, run under Wine against the shipped DLLs with
# P4PC_REQUIRE_LGPL=1). No retail data, no test variables: the player starts the exe with no arguments.
# Output: <out>/p4pc-win64-<version>.zip. Defaults: --build build/package-win, --out dist,
# --version `git describe --tags --always --dirty`.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$here/.." && pwd)
build=$root/build/package-win out=$root/dist jobs=$(nproc) version=
while [ $# -gt 0 ]; do
  case $1 in
    --build) build=${2:?--build needs a folder}; shift 2 ;;
    --out) out=${2:?--out needs a folder}; shift 2 ;;
    --jobs) jobs=${2:?--jobs needs a number}; shift 2 ;;
    --version) version=${2:?--version needs a name}; shift 2 ;;
    -h|--help) sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "package_win: unknown argument $1" >&2; exit 2 ;;
  esac
done
[ -f "$root/decomp/config/symbol_addrs.txt" ] || { echo "package_win: the decomp submodule is not checked out (git submodule update --init)" >&2; exit 1; }
sha=$(git -C "$root" rev-parse HEAD)
[ -n "$version" ] || version=$(git -C "$root" describe --tags --always --dirty)
arch=win64 exe=p4pc64w.exe
name=p4pc-$arch-$version
mkdir -p "$build" "$out"
build=$(CDPATH= cd -- "$build" && pwd)
out=$(CDPATH= cd -- "$out" && pwd)
stage=$build/package-stage
lin_cc=${P4PC_LLVM_MINGW:-/opt/llvm-mingw}/bin/clang
launcher=
command -v ccache >/dev/null && launcher="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"

# 1. The Windows x64 executable, cross-built beside the Linux 64-bit objects, the mirror optimised as shipped.
# shellcheck disable=SC2086
cmake -S "$root" -B "$build" -G Ninja -DCMAKE_C_COMPILER="$lin_cc" $launcher -DP4_DECOMP="$root/decomp" \
  -DP4_PORT64=ON -DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 -DP4_PORT64W_OPT=O2 -DP4_PORT64_JOBS="$jobs" -DP4_CENSUS_CHECK=ON
cmake --build "$build" --target p4pc64w p4mod64w -j "$jobs"
win=$build/win64
sh "$here/package_runtime_provenance.sh" windows x86_64-w64-mingw32 "$win/runtime-provenance.txt"

# 2. The files: the exe and every DLL beside it.
rm -rf "$stage"; mkdir -p "$stage/$name/licenses" "$stage/$name/source" "$stage/$name/examples/mods" "$stage/check"
cp "$win/$exe" "$win"/*.dll "$stage/$name/"
mod_tool=$build/tools/luau/p4mod64w
cp "$mod_tool/p4mod64w.exe" "$stage/$name/p4mod.exe"
cp "$mod_tool/p4.d.luau" "$mod_tool/p4mod-capabilities.txt" "$stage/$name/"
cp -R "$root/examples/mods/p4pc.example" "$stage/$name/examples/mods/"
# Bundled mods, installed read-only beside the exe and disabled until chosen in MODS.
! git -C "$root" cat-file -e HEAD:mods 2>/dev/null || git -C "$root" archive HEAD mods | tar -xf - -C "$stage/$name"

# 3. Text, licences, sources, version (Windows line ends for the text a player opens).
ffver=$(sed -n 's/^FFMPEG_VER=//p' "$root/ci/win32/build_deps.sh")
ffsha=$(sed -n 's/^FFMPEG_SHA=//p' "$root/ci/win32/build_deps.sh")
sdlver=$(sed -n 's/^SDL_VER=//p' "$root/ci/win32/build_deps.sh")
sub() { sed "s|@BUILDLINE@|commit $sha|g; s|@VERSION@|$version|g; s|@SHA@|$version|g; s|@ARCH@|$arch|g; s|@EXE@|$exe|g; s|@FFMPEG_VER@|$ffver|g; s|@FFMPEG_SHA@|$ffsha|g; s|@SDL_VER@|$sdlver|g; s|@APPDATA@|%APPDATA%\\\\p4pc\\\\Persona4\\\\|g"; }
sub < "$root/packaging/README_win.txt" | awk '$0 !~ /^@[A-Z_]+@$/' | sed 's/$/\r/' > "$stage/$name/README.txt"
sub < "$root/packaging/NOTICES_win.txt" | sed 's/$/\r/' > "$stage/$name/THIRD_PARTY_NOTICES.txt"
cp "$root/LICENSE" "$stage/$name/licenses/p4pc-MIT.txt"
cp "$root/NOTICE" "$stage/$name/licenses/NOTICE.txt"
cp "$root"/third_party/licenses/*.txt "$stage/$name/licenses/"
cp "$root/third_party/miniz/LICENSE" "$stage/$name/licenses/miniz-MIT.txt"
tar -C "$root" -czf "$stage/$name/source/luau-source.tar.gz" third_party/luau third_party/luau_patches
cp "$win/runtime-provenance.txt" "$stage/$name/source/runtime-provenance.txt"
# The FFmpeg source tarball and build recipe ship with the package, as on Linux/Android:
# the exact tarball ci/win32/build_deps.sh pins, fetched and sha256-checked again here.
curl -fL --retry 6 --retry-all-errors --retry-delay 10 "https://ffmpeg.org/releases/ffmpeg-$ffver.tar.xz" -o "$stage/$name/source/ffmpeg-$ffver.tar.xz"
echo "$ffsha  $stage/$name/source/ffmpeg-$ffver.tar.xz" | sha256sum -c -
cp "$root/ci/win32/build_deps.sh" "$stage/$name/source/"
printf '%s, commit %s\r\nbuilt %s (%s)\r\n' "$version" "$sha" "$(date -u +%Y-%m-%dT%H:%MZ)" "$arch" > "$stage/$name/VERSION.txt"

# 4. Checks: imports, then the FFmpeg licence check against the DLLs that ship.
echo "package_win: imports of $exe:"
python3 "$here/pe_imports.py" "$stage/$name/$exe" | tr -d '\r' | tee "$stage/check/imports.txt"
python3 "$here/check_package_imports.py" "$stage/$name/$exe" "$stage/$name"
python3 "$here/check_package_imports.py" "$stage/$name/p4mod.exe" "$stage/$name"
cp "$win/ffmpeg_license_test64w.exe" "$stage/$name/ffmpeg_license_test.exe"
(
  unset DISPLAY WAYLAND_DISPLAY
  export WINEPREFIX="$build/wineprefix" WINEARCH=win64 WINEDEBUG=-all WINEDLLOVERRIDES="winedbg.exe=d;mscoree,mshtml="
  cd "$stage/$name" && P4PC_REQUIRE_LGPL=1 timeout -s KILL 300 wine ffmpeg_license_test.exe
) || { echo "package_win: FFmpeg licence check failed or could not run" >&2; exit 1; }
rm -f "$stage/$name/ffmpeg_license_test.exe"

# 5. The zip.
rm -f "$out/$name.zip"
python3 - "$stage" "$name" "$out/$name.zip" <<'ZIP'
import os, sys, zipfile
stage, name, target = sys.argv[1:]
with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    for directory, subdirectories, files in os.walk(os.path.join(stage, name)):
        subdirectories.sort()
        for file in sorted(files):
            path = os.path.join(directory, file)
            archive.write(path, os.path.relpath(path, stage))
ZIP
echo "package_win: $out/$name.zip ($(wc -c < "$out/$name.zip") bytes, commit $sha)"
