#!/bin/sh
# Packages the macOS player build for this Mac's architecture (arm64 or x86_64) as an app bundle in a zip: "Persona 4 PC.app" (p4pc64 as
# Contents/MacOS/p4pc; SDL3, the LGPL-only FFmpeg and ANGLE in Contents/Frameworks), a README, the licence notices and the Luau source.
#
#   sh tools/package_macos.sh [--deps DIR] [--build DIR] [--out DIR] [--version NAME]
#
# Runs on macOS with the Xcode command line tools, from a checkout with the decomp submodule. ci/macos/build_deps.sh fills --deps (the
# pinned clang, SDL3, FFmpeg and ANGLE); p4pc64 is configured with ci/macos/configure.sh (unless --build already is a configured build
# directory) and built there. Checks before it zips: every Mach-O in the bundle loads only system libraries and the bundle's own, the
# FFmpeg dylibs are LGPL-only (tests/ffmpeg_license_test.c against the shipped copies, P4PC_REQUIRE_LGPL=1), and the bundle verifies
# after ad-hoc signing (codesign -s -: the project has no Developer ID; README.txt tells players how to open it). No retail data, no test
# variables: the player starts the app with no arguments. docs/platform/macos.md.
# Output: <out>/p4pc-macos-<arch>-<version>.zip. Defaults: --deps build/macos-deps, --build build/package-macos, --out dist,
# --version `git describe --tags --always --dirty`.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$here/.." && pwd)
deps=$root/build/macos-deps build=$root/build/package-macos out=$root/dist version=
while [ $# -gt 0 ]; do
  case $1 in
    --deps) deps=${2:?--deps needs a folder}; shift 2 ;;
    --build) build=${2:?--build needs a folder}; shift 2 ;;
    --out) out=${2:?--out needs a folder}; shift 2 ;;
    --version) version=${2:?--version needs a name}; shift 2 ;;
    -h|--help) sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "package_macos: unknown argument $1" >&2; exit 2 ;;
  esac
done
[ "$(uname -s)" = Darwin ] || { echo "package_macos: macOS only" >&2; exit 1; }
[ -f "$root/decomp/config/symbol_addrs.txt" ] || { echo "package_macos: the decomp submodule is not checked out (git submodule update --init)" >&2; exit 1; }
sha=$(git -C "$root" rev-parse HEAD)
[ -n "$version" ] || version=$(git -C "$root" describe --tags --always --dirty)
case $(uname -m) in
  arm64)  arch=arm64;  machine="Macs with Apple silicon (M1 and later)"; electron_arch=arm64 ;;
  x86_64) arch=x86_64; machine="Macs with an Intel processor"; electron_arch=x64 ;;
  *) echo "package_macos: unsupported architecture $(uname -m)" >&2; exit 1 ;;
esac
macos_min=${MACOSX_DEPLOYMENT_TARGET:-12.0}
name=p4pc-macos-$arch-$version
app="Persona 4 PC.app"
mkdir -p "$deps" "$build" "$out"
deps=$(CDPATH= cd -- "$deps" && pwd)
build=$(CDPATH= cd -- "$build" && pwd)
out=$(CDPATH= cd -- "$out" && pwd)
stage=$build/package-stage

# 1. The libraries and the compiler, then p4pc64 built against them (a build directory that is already configured is reused).
sh "$root/ci/macos/build_deps.sh" "$deps"
[ -f "$build/CMakeCache.txt" ] || sh "$root/ci/macos/configure.sh" "$deps" "$build" -DP4_CENSUS_CHECK=ON
cmake --build "$build" --target p4pc64

# 2. The bundle: the executable, then every @rpath library it and they load, from the prefix; ANGLE, which SDL loads at run time.
rm -rf "$stage"; mkdir -p "$stage/$name/$app/Contents/MacOS" "$stage/$name/$app/Contents/Frameworks" "$stage/$name/$app/Contents/Resources" \
  "$stage/$name/licenses" "$stage/$name/source" "$stage/$name/examples/mods" "$stage/check"
contents=$stage/$name/$app/Contents
cp "$build/p4pc64" "$contents/MacOS/p4pc"
# the build tree's own run path (the prefix) is not the player's
install_name_tool -delete_rpath "$deps/lib" "$contents/MacOS/p4pc"
rpath_deps() { otool -L "$1" | sed -n 's|^[[:space:]]*@rpath/\([^ ]*\) .*|\1|p'; }
# (the bundle's name has spaces: the walk passes one path at a time, never a word list of paths)
closure() {
  for lib in $(rpath_deps "$1"); do
    [ ! -f "$contents/Frameworks/$lib" ] || continue
    [ -f "$deps/lib/$lib" ] || { echo "package_macos: $1 loads @rpath/$lib, which $deps/lib does not have" >&2; exit 1; }
    cp -L "$deps/lib/$lib" "$contents/Frameworks/$lib"
    chmod 644 "$contents/Frameworks/$lib"
    closure "$contents/Frameworks/$lib"
  done
}
closure "$contents/MacOS/p4pc"
for f in libEGL.dylib libGLESv2.dylib; do
  cp "$deps/lib/$f" "$contents/Frameworks/$f"
done
short=$(printf '%s' "$version" | sed -n 's/^v\{0,1\}\([0-9][0-9]*\(\.[0-9][0-9]*\)\{0,2\}\).*/\1/p')
[ -n "$short" ] || short=0.0.0
sed "s|@SHORT_VERSION@|$short|g; s|@VERSION@|$version|g; s|@MACOS_MIN@|$macos_min|g" "$root/packaging/Info_macos.plist" > "$contents/Info.plist"
plutil -lint "$contents/Info.plist"
printf 'APPL????' > "$contents/PkgInfo"

# 3. Checks: what every Mach-O in the bundle loads (system libraries, or the bundle's own through @rpath / @loader_path), then the FFmpeg
#    licence check against the dylibs that ship.
: > "$stage/check/loads.txt"
for f in "$contents/MacOS/p4pc" "$contents"/Frameworks/*.dylib; do
  otool -L "$f" | tail -n +2 | tee -a "$stage/check/loads.txt" | while read -r path _; do
    case $path in
      /usr/lib/*|/System/Library/*) ;;
      @rpath/*|@loader_path/*|@executable_path/*)
        lib=${path#*/}
        [ -f "$contents/Frameworks/$lib" ] || [ "$(basename "$f")" = "$lib" ] || [ "$path" = "./$(basename "$f")" ] \
          || { echo "package_macos: $f loads $path, which the bundle does not have" >&2; exit 1; } ;;
      ./*) [ "$path" = "./$(basename "$f")" ] || { echo "package_macos: $f loads $path" >&2; exit 1; } ;;
      *) echo "package_macos: $f loads $path (neither the system's nor the bundle's)" >&2; exit 1 ;;
    esac
  done
done
otool -l "$contents/MacOS/p4pc" | grep -A2 LC_RPATH | sed -n 's/^ *path \(.*\) (offset.*/\1/p' > "$stage/check/rpaths.txt"
[ "$(cat "$stage/check/rpaths.txt")" = "@executable_path/../Frameworks" ] || {
  echo "package_macos: the executable's run paths are not just the bundle's Frameworks:" >&2; cat "$stage/check/rpaths.txt" >&2; exit 1; }
# (linked against the prefix, which has the unversioned library names; run against the bundle's copies)
xcrun clang -I"$deps/include" "$root/tests/ffmpeg_license_test.c" -L"$deps/lib" -lavcodec -lavutil -lswscale \
  -Wl,-rpath,"$contents/Frameworks" -o "$stage/check/ffmpeg_license_test"
P4PC_REQUIRE_LGPL=1 DYLD_LIBRARY_PATH="$contents/Frameworks" "$stage/check/ffmpeg_license_test" > "$stage/check/license.txt" 2>&1 || {
  cat "$stage/check/license.txt"; echo "package_macos: FFmpeg licence check failed" >&2; exit 1; }
cat "$stage/check/license.txt"

# 4. Text, licences, sources, version.
ffver=$(sed -n 's/^FFMPEG_VER=//p' "$root/ci/macos/build_deps.sh")
ffsha=$(sed -n 's/^FFMPEG_SHA=//p' "$root/ci/macos/build_deps.sh")
sdlver=$(sed -n 's/^SDL_VER=//p' "$root/ci/macos/build_deps.sh")
elver=$(sed -n 's/^ELECTRON_VER=//p' "$root/ci/macos/build_deps.sh")
elsha=$(sed -n "s/^  $electron_arch) ELECTRON_SHA=\([0-9a-f]*\) ;;\$/\1/p" "$root/ci/macos/build_deps.sh")
sub() { sed "s|@BUILDLINE@|commit $sha|g; s|@VERSION@|$version|g; s|@SHA@|$version|g; s|@ARCH@|$arch|g; s|@MACHINE@|$machine|g; s|@MACOS_MIN@|$macos_min|g; s|@FFMPEG_VER@|$ffver|g; s|@FFMPEG_SHA@|$ffsha|g; s|@SDL_VER@|$sdlver|g; s|@ELECTRON_VER@|$elver|g; s|@ELECTRON_ARCH@|$electron_arch|g; s|@ELECTRON_SHA@|$elsha|g"; }
sub < "$root/packaging/README_macos.txt" | awk '$0 != "@UNLANDED@"' > "$stage/$name/README.txt"
sub < "$root/packaging/NOTICES_macos.txt" > "$stage/$name/THIRD_PARTY_NOTICES.txt"
if grep -n '@[A-Z_]*@' "$stage/$name/README.txt" "$stage/$name/THIRD_PARTY_NOTICES.txt"; then
  echo "package_macos: a placeholder was left in the text" >&2; exit 1
fi
cp "$root/LICENSE" "$stage/$name/licenses/p4pc-MIT.txt"
cp "$root/NOTICE" "$stage/$name/licenses/NOTICE.txt"
for f in FFmpeg-LGPL-2.1.txt SDL3-zlib.txt LodePNG-zlib.txt Luau-MIT.txt Lua-MIT.txt yxml-MIT.txt fdlibm-Sun.txt recvx-decomp-MIT.txt \
         sky2hook-permission.txt; do
  cp "$root/third_party/licenses/$f" "$stage/$name/licenses/"
done
cp "$root/third_party/miniz/LICENSE" "$stage/$name/licenses/miniz-MIT.txt"
cp "$deps/share/angle/LICENSES.chromium.html" "$deps/share/angle/Electron-LICENSE.txt" "$stage/$name/licenses/"
cp -R "$root/examples/mods/p4pc.example" "$stage/$name/examples/mods/"
tar -C "$root" -czf "$stage/$name/source/luau-source.tar.gz" third_party/luau third_party/luau_patches
{
  printf 'Toolchain provenance for macOS %s\n\nCompiler (llvm-mingw %s, macOS host build):\n' "$arch" "$(cat "$deps/llvm/release")"
  "$deps/llvm/bin/clang" --version
  printf '\nSDK: %s (%s)\nLinker: %s\n' "$(xcrun --sdk macosx --show-sdk-version)" "$(xcrun --sdk macosx --show-sdk-path)" "$(ld -v 2>&1 | head -n 1)"
  printf '\nThe C and C++ runtime (libSystem, libc++) are the system'"'"'s; none is linked statically.\n'
  printf 'LLVM project source revision is printed in the compiler version above: https://github.com/llvm/llvm-project\n'
  printf 'llvm-mingw toolchain and build recipes: https://github.com/mstorsjo/llvm-mingw\n'
  printf 'Release and checksum (ci/macos/build_deps.sh):\n'
  sed -n '/^LLVM_MINGW_RELEASE=/p; /^LLVM_MINGW_SHA=/p' "$root/ci/macos/build_deps.sh"
} > "$stage/$name/source/runtime-provenance.txt"
printf '%s, commit %s\nbuilt %s (macOS %s, macOS %s or newer)\n' "$version" "$sha" "$(date -u +%Y-%m-%dT%H:%MZ)" "$arch" "$macos_min" \
  > "$stage/$name/VERSION.txt"

# 5. Ad-hoc signatures, innermost first (the libraries keep no foreign signature), then the zip (ditto keeps the bundle's links and modes).
for f in "$contents"/Frameworks/*.dylib; do
  codesign --force --sign - --timestamp=none "$f"
done
codesign --force --sign - --timestamp=none "$stage/$name/$app"
codesign --verify --deep --strict --verbose=2 "$stage/$name/$app"
rm -f "$out/$name.zip"
(cd "$stage" && ditto -c -k --sequesterRsrc --keepParent "$name" "$out/$name.zip")
echo "package_macos: $out/$name.zip ($(wc -c < "$out/$name.zip" | tr -d ' ') bytes, commit $sha)"
