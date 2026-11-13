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
echo "aa154fc9070614bbe7921f89db11efd1dba7a1f3a41685958110e2230f9c0ca1  $cache/AppImage-runtime.txt" | sha256sum -c -
fetch https://raw.githubusercontent.com/libfuse/libfuse/fuse-3.15.0/LGPL2.txt libfuse-LGPL2.txt
echo "dc626520dcd53a22f727af3ee42c770e56c97a64fe3adb063799d8ab032fe551  $cache/libfuse-LGPL2.txt" | sha256sum -c -
fetch https://raw.githubusercontent.com/vasi/squashfuse/0.5.2/LICENSE squashfuse-LICENSE.txt
echo "9e909cc8a8ba27b1a649c964cf9eda37de911f3138c2b64e6d2f01703904ac13  $cache/squashfuse-LICENSE.txt" | sha256sum -c -
# musl, zstd and zlib are the Alpine 3.21 versions the pinned type2-runtime
# (fedfad51, scripts/docker/Dockerfile FROM alpine:3.21) builds against:
# musl 1.2.5, zstd 1.5.6, zlib 1.3.2 (Alpine 3.21 at that commit).
fetch "https://git.musl-libc.org/cgit/musl/plain/COPYRIGHT?h=v1.2.5" musl-COPYRIGHT.txt
echo "f9bc4423732350eb0b3f7ed7e91d530298476f8fec0c6c427a1c04ade22655af  $cache/musl-COPYRIGHT.txt" | sha256sum -c -
fetch https://raw.githubusercontent.com/facebook/zstd/v1.5.6/LICENSE zstd-LICENSE.txt
echo "7055266497633c9025b777c78eb7235af13922117480ed5c674677adc381c9d8  $cache/zstd-LICENSE.txt" | sha256sum -c -
fetch https://raw.githubusercontent.com/madler/zlib/v1.3.2/LICENSE zlib-LICENSE.txt
echo "e32ff4e00d9d94930537635291da39e7e612703334bf6fde8c7f1686fe8a45a2  $cache/zlib-LICENSE.txt" | sha256sum -c -
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
