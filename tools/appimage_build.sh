#!/bin/sh
# Host-side assembly from an already checked release prefix. No game data.
# appimage_build.sh <stage> <build-prefix> <name> <architecture>
set -eu
stage=${1:?} prefix=${2:?} name=${3:?} arch=${4:-x86_64}
app=$stage/$name
mkdir -p "$app/usr/bin" "$app/usr/lib" "$app/usr/share/applications" "$app/usr/share/icons/hicolor/scalable/apps" "$app/usr/share/p4pc/source" "$app/licenses"
cp "$prefix/bin/p4pc64" "$app/usr/bin/p4pc"
cp -a "$prefix/lib/." "$app/usr/lib/"
cp "$app/org.p4pc.game.desktop" "$app/usr/share/applications/"
cp "$app/org.p4pc.game.svg" "$app/usr/share/icons/hicolor/scalable/apps/"
ln -sf org.p4pc.game.svg "$app/.DirIcon"
chmod +x "$app/AppRun" "$app/usr/bin/p4pc"
cache=$stage/downloads
mkdir -p "$cache"
fetch() { [ -f "$cache/$2" ] || curl -fL --retry 6 --retry-all-errors --retry-delay 10 "$1" -o "$cache/$2"; }
case $arch in
  x86_64) runtime_sha=617aa77314d57c8b8e4fae9c3f0b35af7b30d5529cb69e98f64f92510ed98a46 ;;
  aarch64) runtime_sha=9a72a744d221576c9347ec074243acc5c3c7568896b12714e0e82e9f0bf47900 ;;
  *) echo "unsupported architecture: $arch" >&2; exit 2 ;;
esac
fetch https://ffmpeg.org/releases/ffmpeg-7.1.1.tar.xz ffmpeg-7.1.1.tar.xz
echo "733984395e0dbbe5c046abda2dc49a5544e7e0e1e2366bba849222ae9e3a03b1  $cache/ffmpeg-7.1.1.tar.xz" | sha256sum -c -
fetch https://github.com/libsdl-org/SDL/releases/download/release-3.2.10/SDL3-3.2.10.tar.gz SDL3-3.2.10.tar.gz
echo "f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0  $cache/SDL3-3.2.10.tar.gz" | sha256sum -c -
cp "$cache/ffmpeg-7.1.1.tar.xz" "$cache/SDL3-3.2.10.tar.gz" "$app/usr/share/p4pc/source/"
fetch "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-$arch" "runtime-$arch"
echo "$runtime_sha  $cache/runtime-$arch" | sha256sum -c -
# Pin a versioned release, not the moving continuous build. Use a versioned
# cache name so an older download cannot be mistaken for this release.
fetch https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage appimagetool-1.9.1
echo "ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0  $cache/appimagetool-1.9.1" | sha256sum -c -
chmod +x "$cache/appimagetool-1.9.1" "$cache/runtime-$arch"
# This runtime's --appimage-license is unimplemented; ship its actual licence
# and dependency notices/sources rather than trusting an empty generated file.
fetch https://raw.githubusercontent.com/AppImage/type2-runtime/fedfad51acc2860b2916a1eef32ba4a500ff31ef/LICENSE AppImage-runtime.txt
fetch https://raw.githubusercontent.com/libfuse/libfuse/fuse-3.15.0/LGPL2.txt libfuse-LGPL2.txt
fetch https://raw.githubusercontent.com/vasi/squashfuse/0.5.2/LICENSE squashfuse-LICENSE.txt
fetch https://git.musl-libc.org/cgit/musl/plain/COPYRIGHT musl-COPYRIGHT.txt
fetch https://raw.githubusercontent.com/facebook/zstd/dev/LICENSE zstd-LICENSE.txt
fetch https://zlib.net/zlib_license.html zlib-LICENSE.html
cp "$cache/"*LICENSE* "$cache/"*COPYRIGHT* "$cache/AppImage-runtime.txt" "$cache/libfuse-LGPL2.txt" "$app/licenses/"
fetch https://github.com/AppImage/type2-runtime/archive/fedfad51acc2860b2916a1eef32ba4a500ff31ef.tar.gz runtime-source.tar.gz
fetch https://github.com/libfuse/libfuse/releases/download/fuse-3.15.0/fuse-3.15.0.tar.xz fuse-3.15.0.tar.xz
echo "70589cfd5e1cff7ccd6ac91c86c01be340b227285c5e200baa284e401eea2ca0  $cache/fuse-3.15.0.tar.xz" | sha256sum -c -
fetch https://github.com/vasi/squashfuse/archive/0.5.2.tar.gz squashfuse-0.5.2.tar.gz
echo "db0238c5981dabbd80ee09ae15387f390091668ca060a7bc38047912491443d3  $cache/squashfuse-0.5.2.tar.gz" | sha256sum -c -
cp "$cache/runtime-source.tar.gz" "$cache/fuse-3.15.0.tar.xz" "$cache/squashfuse-0.5.2.tar.gz" "$app/usr/share/p4pc/source/"
ARCH=$arch nice -n 15 "$cache/appimagetool-1.9.1" --appimage-extract-and-run --no-appstream \
  --mksquashfs-opt=-processors --mksquashfs-opt=2 \
  --runtime-file "$cache/runtime-$arch" "$app" "$stage/$name.AppImage"
chmod +x "$stage/$name.AppImage"
