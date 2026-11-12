#!/bin/bash
# Builds and runs, for x86_64 (or i386 with P64_TARGET), the host tests of port-owned code that do not need the
# decomp mirror: the areas of docs/port64.md M3 that are checkable before the 64-bit mirror links. Each test is
# the ctest target of the same name (tests/CMakeLists.txt) with the same sources. Needs clang, SDL3 and libav*
# headers and libraries for the target.
#   tools/port64/test64.sh [out-dir] [test ...]
# Other targets: P64_TARGET (the clang target flags) and P64_RUN (a runner), e.g. AArch64 under qemu-user (docs/port64.md 7):
#   P64_TARGET="--target=aarch64-linux-gnu --gcc-install-dir=/usr/lib/gcc-cross/aarch64-linux-gnu/14 -isystem/usr/aarch64-linux-gnu/include \
#     -isystem/usr/include/aarch64-linux-gnu -fsigned-char -ffp-contract=off -fuse-ld=lld -L/usr/lib/aarch64-linux-gnu" \
#   P64_RUN="qemu-aarch64 -L / -E LD_LIBRARY_PATH=/usr/lib/aarch64-linux-gnu" tools/port64/test64.sh
# Exit 0 when every test built passes or skips (exit 77); a build or run failure prints its log and exits 1.
set -u
R=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-${TMPDIR:-/tmp}/p64_tests}; shift || true
TGT=${P64_TARGET:---target=x86_64-linux-gnu}
LIBDIR=${P64_LIBDIR:-}
mkdir -p "$OUT"
cd "$R"
S=src
FILES="$S/platform/files"
SND="$S/platform/sound"
OS="$S/platform/os/os_posix.c"
GST="$S/platform/gport.c $S/platform/guest.c"
MOVIE="$S/movie/sfd_demux.c $S/movie/mpeg2_dec.c $S/movie/adx_dec.c"
PNG="$S/viewer/p4png.c third_party/miniz/miniz.c third_party/miniz/miniz_tdef.c"
CFLAGS="$TGT -DP4_PC=1 -D_FILE_OFFSET_BITS=64 -O1 -g -w -include p4pc/compat.h -std=gnu11 -Iinclude -Itests -I$S/platform -I$S/movie -I$S/render -I$FILES -I$S/cri -I$S/viewer -I$SND -Ithird_party"
LIBS="-lm"

# name|sources|libs|args
TESTS=(
"vu0_helpers|tests/vu0_helpers_test.c $S/vu0.c $S/platform/ee_float.c|-lm|"
"memcard|tests/memcard_test.c $S/platform/memcard/ps2_memcard.c $OS|-lm|$OUT/memcard_save"
"fpbit|tests/fpbit_test.c $S/platform/ps2_fpbit.c $S/platform/guest.c $OS|-lm|"
"vag|tests/vag_test.c $SND/vag.c||"
"kernel_threads|tests/kernel_threads_test.c tests/state_dump_test_stubs.c $S/platform/state_dump.c $S/platform/kernel_thread.c $S/platform/ps2_kernel.c $S/platform/guest.c $OS|-lm|"
"ee_float|tests/ee_float_test.c|-lm|"
"fn_adapt|tests/fn_adapt_test.c tests/guest_test.c $GST $OS|-lm|"
"files|tests/files_test.c $FILES/vfs.c $FILES/iso9660.c $OS||"
"sdr|tests/sdr_test.c tests/state_dump_test_stubs.c $S/platform/state_dump.c $S/platform/kernel_thread.c $S/platform/ps2_kernel.c $SND/ps2_sndcom.c $SND/sdr.c $SND/sdr_mem.c $SND/hd_bank.c $SND/vag.c $SND/p4snd_mixer.c $SND/p4snd_reverb.c $FILES/vfs.c $FILES/iso9660.c $GST $OS|-lm -lSDL3|"
"mixer_device_dummy|tests/mixer_device_test.c $SND/p4snd_mixer.c $SND/p4snd_reverb.c $OS|-lm -lSDL3|dummy"
"native_tex|tests/native_tex_test.c $S/render/p4gl_nativetex.c $FILES/vfs.c $FILES/iso9660.c $OS||"
"sfmpvp|tests/sfmpvp_test.c tests/guest_test.c $GST $S/movie/sfmpvp.c $S/movie/movie_upload.c $PNG $FILES/vfs.c $FILES/iso9660.c $OS $MOVIE|-lavcodec -lswscale -lavutil -lm|"
"mwply|tests/mwply_test.c tests/guest_test.c $GST $S/movie/mwply.c $PNG $FILES/vfs.c $FILES/iso9660.c $OS $MOVIE|-lavcodec -lswscale -lavutil -lm|-DP4_MOVIE_STANDALONE=1"
"sfd_decode|tests/sfd_decode_test.c $FILES/vfs.c $FILES/iso9660.c $OS $MOVIE|-lavcodec -lswscale -lavutil -lm|"
"cri_acs|tests/cri_acs_test.c tests/guest_test.c $GST $OS|-lm|"
"cri_acssnd|tests/cri_acssnd_test.c tests/guest_test.c $GST $OS|-lm|"
"cri_acs_step|tests/cri_acs_step_test.c tests/guest_test.c $GST $OS|-lm|"
"cri_adxm|tests/cri_adxm_test.c tests/guest_test.c $GST $OS|-lm|"
"cri_init|tests/cri_init_test.c tests/guest_test.c $GST $OS|-lm|"
"cri_stm|tests/cri_stm_test.c tests/guest_test.c $GST $OS|-lm|"
"cri_cvfs|tests/cri_cvfs_test.c tests/guest_test.c $GST $OS|-lm|"
"cri_adxt_start|tests/cri_adxt_start_test.c tests/guest_test.c $GST $OS|-lm|"
"pad_keyboard|tests/pad_keyboard_test.c $S/platform/pad/pad_sdl.c $S/platform/config/p4cfg.c $S/platform/pad/walk_script.c $S/platform/pad/ps2_pad.c $OS|-lm -lSDL3|"
)

fail=0
for t in "${TESTS[@]}"; do
  IFS='|' read -r name srcs libs args <<<"$t"
  if [ $# -gt 0 ] && ! printf '%s\n' "$@" | grep -qx "$name"; then continue; fi
  exe="$OUT/$name"
  if ! clang $CFLAGS $srcs -o "$exe" $libs >"$OUT/$name.build.log" 2>&1; then
    echo "BUILD FAIL $name"; tail -15 "$OUT/$name.build.log"; fail=1; continue
  fi
  # shellcheck disable=SC2086
  P4PC_INPUT=script SDL_AUDIODRIVER=dummy timeout 300 ${P64_RUN:-} "$exe" $args >"$OUT/$name.run.log" 2>&1
  rc=$?
  if [ $rc -eq 0 ]; then echo "PASS $name"; elif [ $rc -eq 77 ]; then echo "SKIP $name"; else echo "FAIL $name (rc=$rc)"; tail -15 "$OUT/$name.run.log"; fail=1; fi
done
exit $fail
