#!/bin/sh
# Packages the x86_64 player build as a standard AppImage, with replaceable SDL3
# and LGPL-only FFmpeg libraries, their complete sources, licences and notices.
#
#   sh tools/package_linux.sh [--work DIR] [--out DIR] [--jobs N] [--version NAME]
#
# Runs in the p4pc-ci image (ci/Dockerfile) or an equivalent Debian trixie with network access for the source downloads, from a
# checkout with the decomp submodule. SDL3 and FFmpeg come from ci/win32/build_deps.sh <work>/deps /usr x86_64-linux-gnu (the
# distribution's FFmpeg is built with --enable-gpl, so it is not shipped); p4pc64 is configured and built in <work>/build against
# them, and tests/ffmpeg_license_test.c runs against the shipped libraries with P4PC_REQUIRE_LGPL=1 (the check is mandatory).
# No retail data, no test variables. The program is linked against the image's glibc (Debian trixie, 2.41): it runs on
# distributions with a glibc at least that new; the package notes list what else it takes from the system.
# Output: <out>/p4pc-linux64-<version>.AppImage. Defaults: --work build/package-linux, --out dist,
# --version `git describe --tags --always --dirty`.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$here/.." && pwd)
work=$root/build/package-linux out=$root/dist jobs=$(nproc) version=
while [ $# -gt 0 ]; do
  case $1 in
    --work) work=${2:?--work needs a folder}; shift 2 ;;
    --out) out=${2:?--out needs a folder}; shift 2 ;;
    --jobs) jobs=${2:?--jobs needs a number}; shift 2 ;;
    --version) version=${2:?--version needs a name}; shift 2 ;;
    -h|--help) sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "package_linux: unknown argument $1" >&2; exit 2 ;;
  esac
done
[ -f "$root/decomp/config/symbol_addrs.txt" ] || { echo "package_linux: the decomp submodule is not checked out (git submodule update --init)" >&2; exit 1; }
sha=$(git -C "$root" rev-parse HEAD)
[ -n "$version" ] || version=$(git -C "$root" describe --tags --always --dirty)
arch=x86_64 triple=x86_64-linux-gnu
name=p4pc-linux64-$version
mkdir -p "$work" "$out"
work=$(CDPATH= cd -- "$work" && pwd)
out=$(CDPATH= cd -- "$out" && pwd)
deps=$work/deps build=$work/build pkg=$work/build/pkg stage=$work/stage
lin_cc=${P4PC_LLVM_MINGW:-/opt/llvm-mingw}/bin/clang
launcher=
command -v ccache >/dev/null && launcher="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"

# 1. SDL3 and LGPL-only FFmpeg from source, then p4pc64 linked against them.
sh "$root/ci/win32/build_deps.sh" "$deps" /usr "$triple"
# shellcheck disable=SC2086
cmake -S "$root" -B "$build" -G Ninja -DCMAKE_C_COMPILER="$lin_cc" $launcher -DP4_DECOMP="$root/decomp" \
  -DP4_PORT64=ON -DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 -DP4_PORT64_JOBS="$jobs" -DP4_LINUX64_PREFIX="$deps" -DP4_FFMPEG_INC="$deps/include" \
  -UP4_SDL3_LIB64 -UP4_AVCODEC_LIB64 -UP4_AVUTIL_LIB64 -UP4_SWSCALE_LIB64 -DP4_CENSUS_CHECK=ON
cmake --build "$build" --target p4pc64 -j "$jobs"

# 2. The program and the libraries it ships with, the licence check against exactly those libraries, and what it takes from the system.
rm -rf "$pkg"; mkdir -p "$pkg/bin" "$pkg/lib"
cp "$build/p4pc64" "$pkg/bin/"
cp -a "$deps"/lib/libSDL3.so* "$deps"/lib/libavcodec.so* "$deps"/lib/libavutil.so* "$deps"/lib/libswscale.so* "$pkg/lib/"
sh "$here/package_runtime_provenance.sh" linux "$triple" "$pkg/runtime-provenance.txt"
"$lin_cc" -I"$deps/include" "$root/tests/ffmpeg_license_test.c" -L"$deps/lib" -lavcodec -lavutil -lswscale -o "$pkg/ffmpeg_license_test64"
LD_LIBRARY_PATH="$pkg/lib" P4PC_REQUIRE_LGPL=1 "$pkg/ffmpeg_license_test64" > "$pkg/license.txt" 2>&1 || {
  cat "$pkg/license.txt"; echo "package_linux: FFmpeg licence check failed" >&2; exit 1; }
cat "$pkg/license.txt"
rm -f "$pkg/ffmpeg_license_test64"
readelf -d "$pkg/bin/p4pc64" "$pkg/lib"/libSDL3.so.0.* 2>/dev/null | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p' | sort -u > "$pkg/needed.txt"

# 3. Text, licences, sources, launcher.
rm -rf "$stage/$name" "$stage/$name.AppImage"; mkdir -p "$stage/$name/licenses" "$stage/$name/usr/share/p4pc/source" "$stage/downloads"
# The AppImage ships the FFmpeg and SDL source archives that build_deps.sh already fetched and checked; appimage_build.sh checks them again.
dl=${P4PC_DL_CACHE:-$work/dl-cache}
cp "$dl"/ffmpeg-*.tar.xz "$dl"/SDL3-*.tar.gz "$stage/downloads/"
mkdir -p "$stage/$name/examples/mods"
cp -R "$root/examples/mods/p4pc.example" "$stage/$name/examples/mods/"
ffver=$(sed -n 's/^FFMPEG_VER=//p' "$root/ci/win32/build_deps.sh")
ffsha=$(sed -n 's/^FFMPEG_SHA=//p' "$root/ci/win32/build_deps.sh")
sdlver=$(sed -n 's/^SDL_VER=//p' "$root/ci/win32/build_deps.sh")
sub() { sed "s#@BUILDLINE@#commit $sha#; s#@VERSION@#$version#g; s#@SHA@#$version#g; s#@ARCH@#$arch#g; s#@FFMPEG_VER@#$ffver#g; s#@FFMPEG_SHA@#$ffsha#g; s#@SDL_VER@#$sdlver#g"; }
sub < "$root/packaging/README_linux.txt" | awk '$0 !~ /^@[A-Z_]+@$/' > "$stage/$name/README.txt"
sub < "$root/packaging/NOTICES_linux.txt" > "$stage/$name/THIRD_PARTY_NOTICES.txt"
cp "$root/LICENSE" "$stage/$name/licenses/p4pc-MIT.txt"
cp "$root/NOTICE" "$stage/$name/licenses/NOTICE.txt"
cp "$root"/third_party/licenses/FFmpeg-LGPL-2.1.txt "$root"/third_party/licenses/SDL3-zlib.txt "$root"/third_party/licenses/LodePNG-zlib.txt \
  "$root"/third_party/licenses/Luau-MIT.txt "$root"/third_party/licenses/Lua-MIT.txt "$root"/third_party/licenses/yxml-MIT.txt \
  "$root"/third_party/licenses/GCC-GPL-3.0.txt "$root"/third_party/licenses/GCC-runtime-exception.txt \
  "$root"/third_party/licenses/fdlibm-Sun.txt "$root"/third_party/licenses/recvx-decomp-MIT.txt \
  "$root"/third_party/licenses/sky2hook-permission.txt "$stage/$name/licenses/"
cp "$root/third_party/miniz/LICENSE" "$stage/$name/licenses/miniz-MIT.txt"
tar -C "$root" -czf "$stage/$name/usr/share/p4pc/source/luau-source.tar.gz" third_party/luau third_party/luau_patches
cp "$pkg/runtime-provenance.txt" "$stage/$name/usr/share/p4pc/source/runtime-provenance.txt"
cp "$root/packaging/org.p4pc.game.desktop" "$root/packaging/org.p4pc.game.svg" "$stage/$name/"
cp "$root/ci/win32/build_deps.sh" "$stage/$name/usr/share/p4pc/source/"
cat > "$stage/$name/AppRun" <<'LAUNCH'
#!/bin/sh
# Keep the player's XDG paths; never write settings or saves inside the AppDir.
d=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
LD_LIBRARY_PATH="$d/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" exec "$d/usr/bin/p4pc" "$@"
LAUNCH
{
  printf '%s, commit %s\nbuilt %s (%s Linux)\n' "$version" "$sha" "$(date -u +%Y-%m-%dT%H:%MZ)" "$arch"
  printf 'licence check (LGPL-only FFmpeg, run against the shipped libraries):\n'; sed 's/^/  /' "$pkg/license.txt"
  printf 'the program takes from the system (shared libraries):\n'; sed 's/^/  /' "$pkg/needed.txt"
} > "$stage/$name/VERSION.txt"

# 4. The AppImage (symlinked library names stay symlinks: assemble on a Linux file system).
sh "$here/appimage_build.sh" "$stage" "$pkg" "$name" "$arch"
cp "$stage/$name.AppImage" "$out/$name.AppImage"
echo "package_linux: $out/$name.AppImage ($(wc -c < "$out/$name.AppImage") bytes, commit $sha)"
