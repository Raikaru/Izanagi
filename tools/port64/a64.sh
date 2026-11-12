#!/bin/bash
# AArch64 without hardware (docs/port64.md 6.7): cross-compile the mirror, the port, the tests and the tools with clang, run what needs no disc under qemu-user.
#   tools/port64/a64.sh configure   [build-dir]   cmake for -DP4_PORT64_ARCH=aarch64 (the arm64 Debian multiarch libraries and gcc-aarch64-linux-gnu are the sysroot)
#   tools/port64/a64.sh build       [build-dir]   the AArch64 targets: p4pc64, p4pc_viewer64, ovdiff64, the mirror-linked tests
#   tools/port64/a64.sh unit        [tests...]    the standalone host tests (tools/port64/test64.sh) under qemu-aarch64
#   N=20 tools/port64/a64.sh ovdiff [build-dir]   ovdiff_subset under qemu-aarch64 (needs SLUS_217.82 as $P4PC_ELF, no disc)
# Tests that read the disc run on the host that has it, in the image of ci/Dockerfile.a64 (qemu-user and the arm64 libraries; the host's binfmt runs an AArch64
# binary directly): `docker run ... p4pc-ci-a64 <binary>`.
# Prerequisites (Debian trixie): dpkg --add-architecture arm64; apt install qemu-user gcc-aarch64-linux-gnu libc6-dev-arm64-cross libsdl3-dev:arm64
# libavcodec-dev:arm64 libavutil-dev:arm64 libswscale-dev:arm64 libgl1-mesa-dri:arm64 libglx-mesa0:arm64 libegl-mesa0:arm64 libgles2:arm64 clang lld llvm.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd)
cmd=${1:-}; shift || true
B=${1:-$R/build-a64}
RUN="qemu-aarch64 -L / -E LD_LIBRARY_PATH=/usr/lib/aarch64-linux-gnu"
TARGET="--target=aarch64-linux-gnu --gcc-install-dir=/usr/lib/gcc-cross/aarch64-linux-gnu/14 -isystem/usr/aarch64-linux-gnu/include -isystem/usr/include/aarch64-linux-gnu -fsigned-char -ffp-contract=off -fuse-ld=lld -L/usr/lib/aarch64-linux-gnu"
case $cmd in
  configure)
    cmake -S "$R" -B "$B" -G Ninja -DCMAKE_C_COMPILER=clang -DP4_DECOMP="$R/decomp" -DP4_RETAIL="${P4PC_ELF:?P4PC_ELF names SLUS_217.82}" \
      -DP4_PORT64_ARCH=aarch64 -DP4_PORT64_OPT=O2 -DP4_PORT64_LINK=ON -DP4_PORT64_WIN=OFF ;;
  build)
    ninja -C "$B" p4pc64 p4pc_viewer64 ovdiff64 render_smoke64 movie_smoke64 movie_task_smoke64 movie_start_test64 axrna_test64 cri_adxi_rpc_test64 \
      adx_wav_test64 ahx_wav_test64 aixp_wav_test64 fn_adapt_test64 ee_float_test64 ;;
  unit)
    P64_TARGET="$TARGET" P64_RUN="$RUN" exec bash "$R/tools/port64/test64.sh" "${TMPDIR:-/tmp}/a64_tests" "$@" ;;
  ovdiff)
    P4_MIPSDIFF_DIR="$R/tools/mipsdiff" exec $RUN "$B/tests/ovdiff64" -n "${N:-20}" -o "${TMPDIR:-/tmp}/ova64.tsv" "${P4PC_ELF:?}" "@$R/tests/ovdiff_subset.txt" ;;
  *) sed -n 2,12p "$0"; exit 2 ;;
esac
