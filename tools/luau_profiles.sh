#!/bin/sh
# Usage: sh tools/luau_profiles.sh <profile> <build-directory>
set -eu
export P4PC_INPUT=script P4PC_MODS=0 P4PC_PACKS=0
profile=${1:?profile: linux-x64 linux-i386 win64 win32 linux-aarch64 android-arm64}
build=${2:?build directory}
src=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
llvm=${P4PC_LLVM_MINGW:-/opt/llvm-mingw}
cc=$llvm/bin/clang
cxx=$llvm/bin/clang++
strip=$llvm/bin/llvm-strip
objdump=$llvm/bin/llvm-objdump
exe_suffix=
runner=
case $profile in
  linux-x64) set -- -DCMAKE_C_COMPILER_TARGET=x86_64-linux-gnu -DCMAKE_CXX_COMPILER_TARGET=x86_64-linux-gnu ;;
  linux-i386) set -- -DCMAKE_C_COMPILER_TARGET=i386-linux-gnu -DCMAKE_CXX_COMPILER_TARGET=i386-linux-gnu -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 ;;
  win64|win32)
    triple=x86_64-w64-mingw32
    [ "$profile" != win32 ] || triple=i686-w64-mingw32
    cc=$llvm/bin/$triple-clang
    cxx=$llvm/bin/$triple-clang++
    exe_suffix=.exe
    runner='xvfb-run -a wine'
    set -- -DCMAKE_SYSTEM_NAME=Windows "-DCMAKE_CROSSCOMPILING_EMULATOR=xvfb-run;-a;wine"
    ;;
  linux-aarch64)
    cc=${P4PC_A64_CLANG:-clang}
    cxx=${P4PC_A64_CLANGXX:-clang++}
    strip=llvm-strip
    objdump=llvm-objdump
    runner='qemu-aarch64 -L /usr/aarch64-linux-gnu'
    set -- -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
      -DCMAKE_C_COMPILER_TARGET=aarch64-linux-gnu -DCMAKE_CXX_COMPILER_TARGET=aarch64-linux-gnu \
      "-DCMAKE_C_FLAGS=--gcc-install-dir=/usr/lib/gcc-cross/aarch64-linux-gnu/14 -isystem/usr/aarch64-linux-gnu/include -isystem/usr/include/aarch64-linux-gnu -fsigned-char -mno-outline-atomics" \
      "-DCMAKE_CXX_FLAGS=--gcc-install-dir=/usr/lib/gcc-cross/aarch64-linux-gnu/14 -isystem/usr/aarch64-linux-gnu/include -isystem/usr/include/aarch64-linux-gnu -fsigned-char -mno-outline-atomics" \
      -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld
    ;;
  android-arm64)
    ndk=${ANDROID_NDK_HOME:?NDK required}
    strip=$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip
    objdump=$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump
    cc=$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/clang
    cxx=$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++
    set -- -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_static \
      "-DCMAKE_C_FLAGS=-fsigned-char -mno-outline-atomics" "-DCMAKE_CXX_FLAGS=-fsigned-char -mno-outline-atomics"
    ;;
  *) echo "unknown profile: $profile" >&2; exit 2 ;;
esac
cmake -S "$src/tools/luau" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$cc" -DCMAKE_CXX_COMPILER="$cxx" "$@"
cmake --build "$build" --parallel "${P4PC_LUAU_JOBS:-2}"
cmake --build "$build" --parallel "${P4PC_LUAU_JOBS:-2}" --target p4_luau_size_probe p4_luau_size_baseline
case $profile in
  win64|win32)
    # Isolated, serial Wine bootstrap, with Xvfb because
    # a fresh prefix may start Wine's desktop services even for a console test.
    export WINEPREFIX="$build/wineprefix" WINEDEBUG=-all
    export WINEDLLOVERRIDES="winedbg.exe=d;mscoree,mshtml="
    export WINEARCH=$profile
    timeout 300 xvfb-run -a wineboot --init
    timeout 60 wineserver -w
    ;;
esac
if [ "$profile" != android-arm64 ]; then
  # runner intentionally word-splits only this fixed profile-selected command.
  timeout 30 $runner "$build/luau_sandbox_test$exe_suffix"
  timeout 120 python3 "$src/tests/luau_check_test.py" $runner "$build/p4_luau_check$exe_suffix"
fi
if [ "$profile" = linux-aarch64 ]; then
  python3 "$src/tools/luau_ios_check.py" --clang "$cxx" --clang-c "$cc"
fi
python3 "$src/tools/luau_size.py" --build "$build" --profile "$profile" \
  --strip "$strip" --objdump "$objdump" --exe-suffix "$exe_suffix"
