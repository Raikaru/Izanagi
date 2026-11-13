#!/bin/sh
# Runs inside the p4pc-android container (ci/android/Dockerfile; docs/android.md): the checkout mounted at /lane/src, an output folder at /lane/out and a
# cache volume at /android-cache. Builds the SDL3 and FFmpeg libraries for the ABI (cached in /android-cache), libmain.so with the NDK, and the APK with
# Gradle. Output: /lane/out/p4pc-<abi>-debug.apk (and the unstripped libmain.so beside it, for ndk-stack and gdb).
#   P4PC_ANDROID_ABI   x86 (default), x86_64, arm64-v8a
#   P4PC_ANDROID_BUILD debug|release (the APK's signing and Java settings; the native code is built the same way as every other target)
#   P4PC_ANDROID_CMAKE_ARGS, P4PC_ANDROID_TAG   one extra CMake argument (-DP4_OPT=O0) and the build directory suffix that keeps its cache apart
#   P4PC_ANDROID_VERSION_NAME, P4PC_ANDROID_VERSION_CODE   the APK's version (for example `git describe --tags` and `git rev-list --count HEAD`)
# A retail SLUS_217.82 mounted at /opt/p4 is optional: the tables step then checks it against config/retail_fnptr_words.txt.
set -eu
src=/lane/src
build=${1:-/lane/build}
# A repository may add modes in tools/android_ci_local.sh, sourced here with src and build set.
if [ -f "$src/tools/android_ci_local.sh" ]; then
  . "$src/tools/android_ci_local.sh"
fi
abi=${P4PC_ANDROID_ABI:-x86}
cache=/android-cache
. "$src/tools/build_cache.sh"
p4pc_build_cache_profile "android-$abi" "${P4PC_CHECK_LOG_DIR:-$build}/build-trace"
if [ -n "${P4PC_JOBS:-}" ]; then export CMAKE_BUILD_PARALLEL_LEVEL=$P4PC_JOBS; fi
now() { date +%s; }
mkdir -p "$build" "$cache" /lane/out
# the debug key is created once and kept in the cache, so every build is signed alike and `adb install -r` updates instead of failing
export GRADLE_USER_HOME=$cache/gradle P4PC_DL_CACHE=$cache/dl ANDROID_USER_HOME=$cache/android-user

t=$(now)
sh "$src/ci/android/build_deps.sh" "$cache/deps" "$abi" > "$build/deps-$abi.log" 2>&1 || { tail -n 40 "$build/deps-$abi.log"; exit 1; }
echo "P4PC android deps rc=0 secs=$(( $(now) - t ))"
phone=${P4PC_ANDROID_PHONE_RUNNER:-0}
case "$phone:$abi" in
  0:*) prefix=$cache/deps ;;
  1:arm64-v8a) prefix=$(sh "$src/ci/android/build_phone_sdl.sh" "$cache/deps" "$cache/phone-deps" "$src/ci/android/sdl-offscreen-android.patch") ;;
  *) echo "P4PC android: phone runner requires arm64-v8a" >&2; exit 1 ;;
esac

phone_suffix=
[ "$phone" != 1 ] || phone_suffix="-phone-${prefix##*/}"
retail_arg=
[ ! -f /opt/p4/SLUS_217.82 ] || retail_arg=-DP4_RETAIL=/opt/p4/SLUS_217.82
nb="$build/android-$abi${P4PC_ANDROID_TAG:+-$P4PC_ANDROID_TAG}$phone_suffix"
# arm64-v8a and x86_64 are the 64-bit model (the rewritten mirror, optimised through the EE float rewrite like the shipping x86_64 build); x86 is the i386 build
case $abi in arm64-v8a|x86_64) model64="-DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 -DP4_PORT64_WIN=OFF" ;; *) model64= ;; esac
if [ -f "$nb/build.ninja" ]; then
  echo "P4PC android configure skipped (cached) rc=0 secs=0"
else
  t=$(now)
  python3 "$src/tools/build_cache.py" run android.configure -- cmake -S "$src" -B "$nb" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" -DANDROID_ABI="$abi" -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_static \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DP4_ANDROID_PREFIX="$prefix" \
    $model64 ${P4PC_ANDROID_CMAKE_ARGS:-} -DP4_ANDROID_PHONE_RUNNER="$phone" -DP4_DECOMP="$src/decomp" $retail_arg > "$nb.configure.log" 2>&1 || { tail -n 40 "$nb.configure.log"; exit 1; }
  echo "P4PC android configure rc=0 secs=$(( $(now) - t ))"
fi

t=$(now)
rc=0
target=main
[ "$phone" != 1 ] || target=p4pc_phone
python3 "$src/tools/build_cache.py" run android.build -- cmake --build "$nb" --target "$target" -- -k 0 > "$nb.build.log" 2>&1 || rc=$?
echo "P4PC android build rc=$rc secs=$(( $(now) - t )) steps=$(grep -c '^\[[0-9]*/' "$nb.build.log" || true)"
if [ $rc -ne 0 ]; then
  grep -E 'error:|FAILED|undefined|rewriter rule counts differ|rule skipped:|constructs the rewriter does not handle|CRT guard rejected|absent from reviewed allowlist| -> ' "$nb.build.log" || true
  exit $rc
fi
if [ "$phone" = 1 ]; then
  cp "$nb/p4pc_phone" /lane/out/p4pc_phone
  for library in libSDL3.so libavcodec.so libavutil.so libswscale.so; do
    cp -L "$prefix/arm64-v8a/lib/$library" "/lane/out/$library"
  done
  exit 0
fi

# Preserve every library's symbols outside the APK, paired to this build.
libs="$build/android-libs-$abi"
symbols="$build/android-symbols-$abi"
rm -rf "$libs" "$symbols" && mkdir -p "$libs/$abi" "$symbols/$abi"
cp "$nb/libmain.so" /lane/out/libmain-$abi.so
strip="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
objcopy="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objcopy"
for so in "$nb/libmain.so" "$cache/deps/$abi/lib/"*.so; do
  name=$(basename "$so")
  "$objcopy" --only-keep-debug "$so" "$symbols/$abi/$name.debug"
  "$strip" --strip-unneeded -o "$libs/$abi/$name" "$so"
done
tar -czf "/lane/out/symbols-$abi.tar.gz" -C "$symbols" .
# Notices and complete library source accompany every APK/AAB as assets. The
# libraries remain separate ELF objects, replaceable by repacking and re-signing.
assets="$src/build/android/package-assets"
mkdir -p "$assets/licenses" "$assets/source"
cp "$src/third_party/licenses/FFmpeg-LGPL-2.1.txt" "$src/third_party/licenses/SDL3-zlib.txt" "$assets/licenses/"
cp "$src/third_party/miniz/LICENSE" "$assets/licenses/miniz-MIT.txt"
cp "$src/third_party/licenses/LodePNG-zlib.txt" "$src/third_party/licenses/cgltf-MIT.txt" "$src/third_party/licenses/yxml-MIT.txt" "$src/third_party/licenses/fdlibm-Sun.txt" "$src/third_party/licenses/newlib-BSD.txt" \
  "$src/third_party/licenses/recvx-decomp-MIT.txt" "$src/third_party/licenses/sky2hook-permission.txt" "$assets/licenses/"
cp "$src/LICENSE" "$assets/licenses/p4pc-MIT.txt"
cp "$src/NOTICE" "$assets/licenses/NOTICE.txt"
cp "$src/third_party/licenses/Luau-MIT.txt" "$src/third_party/licenses/Lua-MIT.txt" \
  "$src/third_party/licenses/LLVM-libcxx.txt" "$assets/licenses/"
cp "$ANDROID_NDK_HOME/NOTICE" "$assets/licenses/Android-NDK-NOTICE.txt"
cp "$ANDROID_NDK_HOME/NOTICE.toolchain" "$assets/licenses/Android-NDK-toolchain-NOTICE.txt"
tar -czf "$assets/source/luau-source.tar.gz" -C "$src" third_party/luau third_party/luau_patches
case $abi in
  arm64-v8a) runtime_target=aarch64-linux-android ;;
  x86) runtime_target=i686-linux-android ;;
  x86_64) runtime_target=x86_64-linux-android ;;
esac
sh "$src/tools/package_runtime_provenance.sh" android "$runtime_target" "$assets/source/runtime-provenance.txt"
cp "$cache/dl/ffmpeg-7.1.1.tar.xz" "$cache/dl/SDL3-3.2.10.tar.gz" "$assets/source/"
cp "$src/ci/android/build_deps.sh" "$assets/source/"
cp "$src/packaging/NOTICES_android.txt" "$assets/THIRD_PARTY_NOTICES.txt"

t=$(now)
variant=${P4PC_ANDROID_BUILD:-debug}
task=assemble$(echo "$variant" | sed 's/^./\U&/')
[ "$variant" != release ] || task="$task bundleRelease assembleMigration"
rm -rf "$src/android/app/build/outputs" # AGP updates an existing APK in place and leaves the dead space of the old one in the file
python3 "$src/tools/build_cache.py" run android.package -- gradle --no-daemon --console=plain -p "$src/android" $task -Pp4.abi="$abi" -Pp4.libs="$libs" \
  -Pp4.versionName="${P4PC_ANDROID_VERSION_NAME:?set the source version}" \
  -Pp4.versionCode="${P4PC_ANDROID_VERSION_CODE:?source commit count required}" \
  -Pp4.sdlJava="$cache/deps/sdl-java/app/src/main/java" > "$build/gradle-$abi.log" 2>&1 || { tail -n 40 "$build/gradle-$abi.log"; exit 1; }
apk=$(find "$src/android" \( -name "app-$variant.apk" -o -name "app-$variant-unsigned.apk" \) 2>/dev/null | head -n 1)
[ -n "$apk" ] || { echo "P4PC android: no APK produced"; exit 1; }
cp "$apk" "/lane/out/p4pc-$abi-$variant.apk"
# Keep the normal package for installs, and separately build the unsigned APK
# used for reproducibility comparisons (debug signatures vary by host).
rm -rf "$src/android/app/build/outputs"
python3 "$src/tools/build_cache.py" run android.package-unsigned -- gradle --no-daemon --console=plain -p "$src/android" $task -Pp4.abi="$abi" -Pp4.libs="$libs" \
  -Pp4.versionName="${P4PC_ANDROID_VERSION_NAME:?set the source version}" \
  -Pp4.versionCode="${P4PC_ANDROID_VERSION_CODE:?source commit count required}" \
  -Pp4.sdlJava="$cache/deps/sdl-java/app/src/main/java" -Pp4.unsigned=true > "$build/gradle-$abi-unsigned.log" 2>&1 || { tail -n 40 "$build/gradle-$abi-unsigned.log"; exit 1; }
unsigned_apk=$(find "$src/android" -name "app-$variant-unsigned.apk" 2>/dev/null | head -n 1)
[ -n "$unsigned_apk" ] || { echo "P4PC android: no unsigned APK produced"; exit 1; }
cp "$unsigned_apk" "/lane/out/p4pc-$abi-$variant-unsigned.apk"
echo "P4PC android apk rc=0 secs=$(( $(now) - t )) size=$(stat -c %s "/lane/out/p4pc-$abi-$variant.apk") $apk"
if [ "$variant" = release ]; then
  cp "$src/android/app/build/outputs/bundle/release/app-release.aab" "/lane/out/p4pc-$abi-release.aab"
  cp "$src/android/app/build/outputs/mapping/release/mapping.txt" "/lane/out/mapping-$abi.txt"
  # Private one-time migration bridge, never a player artifact: signing it with
  # the release key permits run-as restoration before updating to the true release.
  cp "$src/android/app/build/outputs/apk/migration/app-migration-unsigned.apk" "/lane/out/p4pc-$abi-migration.apk"
  if [ "$abi" = arm64-v8a ]; then
    "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang" \
      -O2 -g -DP4_GUEST_CHECK -ffunction-sections -fdata-sections -I"$src/include" \
      "$src/tests/crash_log_test.c" "$src/src/platform/os/os_posix.c" "$src/src/platform/guest.c" \
      -Wl,--gc-sections -ldl -llog -o "/lane/out/crash-log-test-$abi"
  fi
fi

# iOS EGL-on-CAMetalLayer plumbing (src/render/p4gl_egl.c, P4PC_GL_VIA_EGL): no iOS SDK is available in CI (Apple's licence forbids shipping it and osxcross needs
# one), so tools/ios_check.sh compiles it for arm64-apple-ios with clang's own headers, SDL3's public headers and stub libc declarations. Not a link or a run.
sh "$src/tools/ios_check.sh" "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" "$cache/deps/x86/include" "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include" "$src" > "$build/ios-check.log" 2>&1 \
  && echo "P4PC ios-plumbing compile check rc=0 (arm64-apple-ios13.0, no SDK, syntax only)" \
  || { echo "P4PC ios-plumbing compile check FAILED"; head -n 30 "$build/ios-check.log"; exit 1; }
