# Android

The port runs on Android as an SDL3 activity around `libmain.so`, which is the same game as the Linux and Windows builds. It is built for the **x86 32-bit ABI**
(the i386 build of the port: what runs on the emulator, sections 3 and 4) and for **arm64-v8a** (the 64-bit model, [port64.md](port64.md): what runs on a phone, section 4b).
This page is what was built, how to build and run it, and what is still open.

| Part | Where |
|---|---|
| Build image (SDK 34, NDK r27, Gradle 8.7, emulator, x86 system image) | `ci/android/Dockerfile` |
| SDL3 and FFmpeg for an ABI | `ci/android/build_deps.sh` |
| APK build (in the container) | `tools/android_ci_run.sh` |
| Gradle project and Java (launcher, activity, touch pad) | `android/` |
| Native glue (`SDL_main`, log pipe, `exit` status, JNI) | `src/platform/android/android_main.c` |
| Coroutine switch for i386 (no `ucontext` in bionic) | `src/platform/os/coro_asm.c` |
| Run and gate on a device or emulator | `tools/android_prog.py`, `tools/android_bin/` (shims a gate script can take in place of the host executables) |
| iOS EGL plumbing (compiled only for iOS) | `src/render/p4gl_egl.c`, `src/render/p4gl_win.h` |

## 1. Build

```sh
podman build -t p4pc-android ci/android        # once
podman run --rm -v "$PWD:/lane/src" -v "$PWD/build/android-out:/lane/out" -v p4pc-android-cache:/android-cache \
  -e P4PC_ANDROID_ABI=arm64-v8a -e P4PC_ANDROID_BUILD=debug \
  -e P4PC_ANDROID_VERSION_NAME="$(git describe --tags --always)" -e P4PC_ANDROID_VERSION_CODE="$(git rev-list --count HEAD)" \
  p4pc-android sh /lane/src/tools/android_ci_run.sh
```
`P4PC_ANDROID_ABI` is `x86` (default, the emulator), `x86_64` or `arm64-v8a`; `P4PC_ANDROID_BUILD=release`
builds the unsigned release APK and AAB, which you align and sign with your own key (Android SDK `zipalign`,
`apksigner`). See [release packaging](platform/packaging.md) for symbols and legacy same-ID debug-to-release
migration without losing the player's storage.
Ordinary debug APKs are **P4PC Debug**, `org.p4pc.game.debug`, signed by the cached
debug key. They coexist with the release app and have separate files, settings,
saves, media inbox and ISO picker grants. All automated phone tests use this
variant; never replace or uninstall the player's release to run a test.

The container writes `p4pc-<abi>-<debug|release>.apk` and the unstripped `libmain-<abi>.so`
(for `llvm-addr2line` on a tombstone) to `/lane/out`. Inside it:

1. `ci/android/build_deps.sh` builds SDL3 3.2.10 (shared, with its Java classes) and the LGPL FFmpeg 7.1.1 subset of the movie decoder
   (`mpeg2video`, `adpcm_adx`, `swscale`; shared libraries, because the LGPL's relink condition needs them replaceable) for the ABI, once, into the
   host's `android-cache`.
2. CMake with the NDK toolchain builds `libmain.so` (`CMakeLists.txt`, `P4_ANDROID`): the decomp mirror, the port and the viewer in one
   shared library, with the same `gen_port.py` passes as the other targets. The tables step takes bionic's `libc`/`libm`/`libdl`/`liblog` stubs, SDL3,
   FFmpeg and the compiler runtime as host libraries. `__stack_chk_fail` is an explicitly allowed compiler-runtime import backed by Bionic; the NDK's stack protector remains enabled.
3. Gradle packages the libraries, notices, complete SDL/FFmpeg source and Java.
   Debug builds retain the cached debug key. Release builds use R8 shrinking,
   emit unsigned APK/AAB, and export mapping plus separate native symbol files.

No retail data goes into the APK. Native libraries are uncompressed and page
aligned (`extractNativeLibs=false`). The initial x86 debug APK, before bundled
corresponding-source archives, was 11,033,835 bytes. The qualified arm64 release
`468650dd` is 39,839,171 bytes including those sources; its signed AAB is
31,890,440 bytes. Warm rebuilds reuse the ABI dependency and compiler caches.

`P4PC_ANDROID_CMAKE_ARGS=-DP4_OPT=O0` with `P4PC_ANDROID_TAG=o0` builds an experiment in a directory of its own.

Failed Android builds print CRT guard caller/binding rejections; the complete build log
remains in the build directory.

### What an Android shared library changes in the build

- **Position independent code.** Android refuses text relocations, so the port is `-fPIC` here (the Linux build is `-fno-pic -no-pie`, the Windows build is
  an exe with a fixed image base).
- **Absolute symbols.** The generated tables define every retail data address as an absolute symbol (`.set D_00764BA4, 0x764ba4`). An *exported*
  absolute symbol is resolved when the library loads, and bionic's linker adds the load base to it, so the guest address became a host address
  inside the library (every `D_` access reads or writes libmain's own text). `gen_port.py tables --pic` marks them `.hidden`: the static link then
  resolves each use to the constant, with no relocation. Host function thunks go through the PLT (`jmp sqrt@PLT`).
- **Compiler.** NDK r27's clang 18 turns `return;` in a non-void function into an error (clang 20, the Linux and Windows compiler, knows
  `-Wno-return-mismatch`, which is in the decomp's flag files and is unknown here): `-Wno-error=return-type -Wno-return-type`.
- **Host names that bionic supplies statically.** `atexit`, `at_quick_exit` and `pthread_atfork` live in `libc_nonshared.a`, and `__divdi3` and the
  other 64-bit helpers in the compiler runtime archive: `--host-name` and the archive as a `--host-lib`, so they are not stubbed as trapped retail
  functions.
- **Coroutines.** `src/platform/os/coro_asm.c` is the i386 context switch the kernel model's `p4_coro_*` uses on Android (the x86_64 and aarch64 ones are the 64-bit port's, in
  `os_posix.c`). Stack alignment matters: bionic's x86 code uses aligned SSE accesses, so a coroutine starts with `esp + 4` 16-byte aligned.
- **Time and files.** `_FILE_OFFSET_BITS=64` (the disc image is 4.4 GB; `fseeko`), no `_TIME_BITS` (bionic has none on 32-bit).
- **Fixed mappings.** `MAP_FIXED_NOREPLACE` has been in every Android kernel the port supports since 4.17; the NDK header lacks it for old API levels, so it is
  defined. The PS2 RAM (`0x00100000`) and the EE registers (`0x10000000`) map at their retail addresses in the app process.

## 2. The app

`P4Launcher` is the setup screen: **Choose disc image** and (optional) **Choose SLUS_217.82** open the Storage Access Framework document picker
(`ACTION_OPEN_DOCUMENT`); the URIs are kept with persisted permissions, so the next start goes straight to **Start**. The executable is on the disc, so
without it the game copies `SLUS_217.82` out of the image into the app's files directory on first start (`android_main.c`, `iso_walk`).

`P4Activity` (an `SDLActivity`) opens the picked documents as descriptors and hands the game `P4PC_ISO=fd:N` and `P4PC_ELF=fd:M`: the VFS seam
`p4_os_fopen_rb` (`include/p4pc/os.h`) takes a path or `fd:N`, `dup`s the descriptor and `fdopen`s it, so the disc stays a `FILE*` with 64-bit
offsets and nothing is copied. Saves are in the app's storage (`P4PC_SAVE_DIR` = `files/saves`; the memory card layout is the retail one).

Launch extras are the test interface: `iso` and `elf` (a URI, or an absolute path), `env0`, `env1`, ... (`NAME=VALUE`, each set in the environment
before the game starts: `P4PC_AUTOPRESS`, `P4PC_CAPTURE_FRAMES`, ...), `pad=0` (no touch pad). stderr goes through a pipe to logcat (tag `p4pc`)
and to `files/boot.log`; every `exit()` of the process logs `p4pc: exited with status N` (`--wrap=exit`), which is how a run reports its result.

For a controlled restart, `p4_os_request_restart(reason)` calls the Activity's
small JNI entry and schedules `P4RestartActivity` before the caller exits through
`__wrap_exit`; a failed handoff returns -1 to the native error UI.
It does not recreate the SDL Activity or call SDL main again. The helper runs in `:restart`, outside
the game task, and links to a Binder owned by the old process. Binder death (also
handled when the process died before registration) triggers exactly one relaunch
on the main looper. It keeps the original intent's URI strings/extras and the
app UID's grants/inbox; the new Activity reopens ISO/ELF descriptors. A launch
that only supplies process-local `fd:N` values cannot be restarted and returns
failure to the existing native error UI. Debug/release task affinity comes from
`${applicationId}.restart`, never a hard-coded production package.

The once-only `P4PC_TEST_RESTART=<app-owned marker>` witness checks the old
exit-handler marker, unchanged arguments and distinct old/new PIDs. Automated
phone proof must use only the separate P4PC Debug application and take the phone
lock; it must never install, stop, clear or uninstall the release application.
Stage the debug APK and its ISO, then run
`python tools/android_restart_test.py --serial HOST:PORT --out PRIVATE_DIRECTORY`.
It refuses a production-targeting runner before adb, owns the phone lock, verifies
distinct PIDs plus the drain marker and preserved media inbox, retains evidence,
and stops only the debug app. It does not install/uninstall either application.

On the Galaxy S20+ the exact `dbbacb85` Debug APK's native once-only witness
recorded old PID 4991, new PID 5142, identical argument hash and the ordered
`requested` / `drained` / `second` marker. The initial driver incorrectly timed
out because adb shell returned CRLF; its retained native log and marker prove the
fresh-process/drained-exit assertions, but that attempt did not reach the
driver's inbox-sentinel assertion. The driver now normalizes PTY line endings;
its device-free regression exercises that correction. The release app was not
installed, stopped or changed.

**Touch pad** (`PadOverlay`): d-pad, a floating analog stick, four face buttons, L1/R1/L2/R2, Start, Select and CONFIG, drawn over the surface. Every pill carries its label
(`L1 L2 R1 R2 SELECT START CONFIG`). The d-pad, face buttons, shoulders, Start and Select inject the Android key that the existing keyboard binding of that PS2 button uses
(`pad_sdl.c` `kKeys`: arrows, Z X C V, Enter, Backspace, Q E, 1 3) through SDL's key entry points. **CONFIG** injects F1, the keyboard's shortcut to the native Config
screen (`p4settings.c`; Select alone is the game's Select and opens nothing; the gamepad's Select + Start hold has no touch equivalent); pressed again while Config is
open it leaves it. The **stick** is analog: its thumb offset from where the finger touched (travel 1.8 pad units, `u`) goes through JNI (`P4Activity.nativeTouchStick`,
`android_main.c`) to `p4pc_pad_touch_stick`, which `pad_sdl.c` merges into the game's left stick like a gamepad's axis: its own 15 percent dead zone is rescaled away,
then `input.deadzone` and the PCSX2 byte quantization apply, a deflected real gamepad stick keeps its own axes (the touch stick outranks the d-pad's synthesized stick), and in Config a deliberate deflection also
presses the direction the list reads. The D-pad stays for menus (they read digital directions). **Controllers** (Bluetooth or USB) arrive through SDL's gamepad support,
which `pad_sdl.c` already reads; the overlay hides while one is connected.

On wide landscape displays (20:9 phones such as the S20+, 2400 x 1080 or 3200 x 1440) the whole pad fits inside the black side bars of the 4:3 picture. The bar is 6.4 `u`
wide and the left bar stacks, top to bottom, L1, L2, the stick zone (the thumb's first contact inside it is the stick's centre), the d-pad and Select, in 14.4 `u` of height;
the right bar holds R1, R2, CONFIG, the face buttons and Start. Their scale follows the bar width and the display height, not the full display width. Narrower
displays keep the overlaid layout, where there is room for one left control only: a pill under L2 switches the d-pad for the stick (remembered in the app's preferences).

**Display.** SDL's EGL gives an ES 3.0 context (`p4gl_api.h`). The game draws into a 640 x 448 offscreen target and blits it to the window with the 4:3 picture
fitted and black bars (`P4PC_PRESENT=blit` and `P4PC_PRESENT_FIT=4:3` are Android defaults
in `src/platform/config/p4cfg.c`). The window is fullscreen; [player settings](platform/settings.md)
apply, with the INI in the app's files directory and `P4PC_CONFIG=none` in gates. Thus the
window buffer is only ever written: `EGL_BUFFER_DESTROYED`
(nothing promises a swap keeps it) and the surface the system replaces at every pause do not matter. Before SDL initialises, Android's entry sets [`SDL_HINT_ORIENTATIONS`](https://wiki.libsdl.org/SDL3/SDL_HINT_ORIENTATIONS) to `LandscapeLeft LandscapeRight`, matching the activity manifest: SDL's own policy otherwise allows the portrait launcher orientation to persist when the surface returns after HOME.

**Context loss** (`p4gl_context.c`, `context_recover`): the system may take the GPU when the app is paused. SDL makes a new context itself and sends
`SDL_EVENT_RENDER_DEVICE_RESET` (adopted on the next present), and a swap that fails with `EGL_CONTEXT_LOST` creates one. Either way the entry points are resolved
again, the call-state caches reset, every module drops its GL names without deleting them (`p4gl_*_context_lost`), the present target is recreated, and the
textures are uploaded again from the rasters' CPU shadows (the same upload as at unlock); programs, buffers and render targets rebuild on their next use. The game
redraws the whole frame the frame after. `P4PC_TEST_CONTEXT_LOSS=N` replaces the context after the Nth presented frame on any platform: the ctests
`title_mad_ctxloss` and `field_mad_ctxloss` require the frame to match as before.

**Audio** is SDL3's (AAudio/OpenSL ES behind it); nothing platform specific is in the port.

## 3. Running on an emulator

```sh
python tools/android_prog.py setup --apk build/android-out/p4pc-x86-debug.apk --iso <game.iso>  # install, push the disc image and refs/*.rgb
P4PC_ISO=<game.iso> sh tests/<gate>.sh ... tools/android_bin/p4pc                                 # a gate script, with the shim as its program
```
`setup --apk` requires SDK build-tools `aapt2` on PATH, or `ANDROID_AAPT2` /
`--aapt2 <path>`. It validates the debuggable `org.p4pc.game.debug` identity before
any adb command. Settings, import and package smokes use this same isolated runner.


`android_prog.py run` is a stand-in for the host executables: `tools/android_bin/p4pc` and `p4pc_viewer` are shims the gate scripts (`tests/*.sh`) call with
their usual environment and arguments; the run forwards the `P4PC_*` variables to the app, copies reference frames and the `P4PC_WALK_SCRIPT` file to the device and frames and saves back, and
exits with the status the app logged. The same scripts and thresholds as every other platform. `ANDROID_ADB`, `ANDROID_SERIAL` and
`P4PC_ANDROID_TIMEOUT` configure it.

The emulator runs headless (`-no-window -no-audio`, hardware acceleration KVM on Linux or WHPX on Windows). The AVD is `x86` API 30 `google_apis` with
a 12 GB data partition (the disc image is 4.4 GB, pushed to `/sdcard/Android/media/org.p4pc.game.debug/files/`: Android 11 keeps `adb` out of `Android/data`).
**Two settings matter**: `hw.gpu.enabled=yes` with `-gpu host -feature GLESDynamicVersion`, because without them the emulator's guest EGL offers ES 2.0
only (`EGL_BAD_CONFIG: no ES 3 support` at `eglCreateContext`); the host GPU translates ES 3.1 to the desktop driver.

On a machine with `/dev/kvm` the same image runs in the container (`ci/android/Dockerfile` creates the AVD `p4pc`). On the Linux hosts available to
this project the emulator crashed at boot with KVM on a nested AMD EPYC-Turin virtual machine (a segmentation fault after the HAL starts; `-accel off` boots but
is too slow to use), so the runs recorded here are from the Windows machine (WHPX, RTX 4080).

## 4. Results (x86 emulator, Android 11, RTX 4080 behind the GLES translator)

All of `tests/CMakeLists.txt`'s scene gates, run with the same scripts and thresholds as the host (through the `tools/android_bin` shims), against the same reference frames. The table is
the run after the colour-output fix and the libm fix (docs/renderer_port.md 10.6a, 10.6b): before the colour fix, field, sky and butterfly were over their limits (1.9968, 3.4902, 3.8974), exactly as on Windows with the
same NVIDIA driver, and an Android frame was the Windows native frame to 0.0001% MAD, so the difference was the GPU's rounding, not Android.

| Gate | Android x86 | Limit | Windows NVIDIA (WGL ES / GL) |
|---|---|---|---|
| title_mad | 1.2093% pass | 1.25 | 1.2092% |
| field_mad | 1.5040% pass | 1.6 | 1.5039% |
| field_mad_ctxloss (context replaced at frame 300) | 1.5035% pass | 1.6 | 1.5039% |
| scene_sky_black_tint | 2.6807% pass | 2.8 | 2.6806% |
| scene_near_fog | 15.3052% pass | 15.6 | 15.3062% |
| scene_save_butterfly_glow | 3.27% (not rerun on the emulator: 53 minutes; the RTX 4080 reads 3.425% at 1283767: a device limit of 3.5; llvmpipe, 3.3083, is judged at 3.4) | 3.4 | 3.2716% |
| scene_riverbank_vignette | 8.7191% pass | 8.8 | 8.7191% |
| scene_arcana_rankup_sparkles | 13.2380% pass | 13.4 | 13.2380% |
| battle_menu, scene_battle_floor_a, camp_menu, save_load | pass | | |
| newgame_to_field (title, New Game by autopress, the field after 30500 flips) | pass, 432 s | | |
| retail_save_load, systems_*, save_play | skipped (no retail saves beside the ELF) | | |

Speed: the emulator forwards every GL call to the host, so a frame with draws costs several times a desktop frame: the unattended new-game run is 50 to 60 flips a second
(Linux llvmpipe: 1000) and butterfly_glow takes 53 minutes (Windows: 42 s). `P4PC_TURBO=2` skips the blit and swap of frames whose draws it drops.

Live run (no test variables, from the launcher's disc-image extra): the logo, the opening movie with its sound, the title; the picture fitted 4:3 (1440 x 1080 in a
2340 x 1080 screen) with the bars black and the system bars hidden; a tap on the drawn cross logs `p4pc pad: port0 buttons 0x4000` and a tap on the d-pad `0x0080`;
`HOME` then the launcher: SDL's `onPause`/`onStop`/`onStart`/`onResume`, no new context (the emulator keeps it), the game carries on.

## 4b. arm64-v8a on a Galaxy S20+ (the 64-bit model on a real device)

For phone testing, keep the phone on its charger with its battery protection enabled (85% limit), the screen off, wireless ADB connected, and the phone ventilated. Test runs use the separate debug app and its `/data/local/tmp/p4pc` workspace; they must never install, stop or modify the release app.

`P4PC_ANDROID_ABI=arm64-v8a` (section 1) builds the 64-bit port as `libmain.so` (`P4_ANDROID64` in `CMakeLists.txt`): the rewritten mirror (`tools/port64/unit64.py`, compiled with the NDK's clang for
`aarch64-linux-android24`, `-fPIC`, the i386 layout oracle of the rewrite for `i686-linux-android24`), the port sources, the tables of those objects (`gen_port.py tables --ptr64 --arm64 --pic` with the NDK's
libc, libm, SDL3 and FFmpeg as host libraries), and `android_main.c` around them; `android_ci_run.sh` passes `-DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2` (the optimised mirror through the EE float rewrite, as the
shipping x86_64 build). 2690 steps, about 7 minutes cold on the host with a warm ccache. `x86_64` (the emulator's other ABI) is accepted by the script and stops at `src/platform/ee_float.c`: its x87 asm does not take Android's x86_64
`long double`; not pursued.

The 32-bit layout oracle keeps the NDK target's vendor, OS and API suffix and replaces only its architecture:
`aarch64-none-linux-android24` becomes `i686-none-linux-android24`. The CMake substitution consumes the whole triple in one match;
a prefix-only anchored replacement re-matches successive suffixes on older CMake and produces the invalid `i686-i686-i686-android24`.
Android census baselines must be reviewed against the valid NDK oracle, never refreshed against that malformed target.

The corrected NDK r27.2 / clang 18 baseline was measured from all 1231 active
units with zero unsupported constructs. Its broader oracle coverage adds 7066
compile-time layout assertions (45043 to 52109); the only runtime-rule counter
changes are five units with explicitly restored retail arguments. The preserved
`none` vendor and canonical three-component Android targets have identical
header/predefined macros and 32-bit layout; the vendor is not an ABI difference.

What running on the phone needed, found in this order:

1. **The game's thread has to be on the stack arena.** The 64-bit model keeps every stack in guest memory (the arena of the guest block), so the game runs on stacks ART never heard of, and ART judges a thread's
   Java calls by the stack bounds it attached with: SDL's first call into Java (`SDL_InitSubSystem`) failed with a `StackOverflowError: stack size 15MB` (sp below the thread's stack end), and once that was passed, a framework
   native called from it (`Parcel.nativeWriteInterfaceToken`) aborted with "jstring is an invalid JNI transition frame reference" (the reference is on the stack, which ART checks against the same bounds). `android_main.c`
   creates the game thread with `pthread_attr_setstack` on the arena (`p4_guest_init` runs in `main` first), attaches it to the JVM before anything runs on it (`SDL_GetAndroidJNIEnv`: ART's first attach reads the whole range, and
   the arena has no guard pages yet) and reserves the arena's first 64 KiB (the region ART protects) and last 512 KiB (bionic's thread block and the native frames of `p4pc_main`) from `p4_guest_stack_alloc`. The game's own
   stacks are the arena's middle, so every Java call SDL makes from any of them passes ART's checks.
2. **`ee_libm.c` func_0044a700** (the retail `__kernel_rem_pio2f`, sinf's huge-argument slow path): `t5 = W(t5 + t7)` truncated the table pointer it had reloaded from its shadow slot to 32 bits, and a garbage angle in
   `RwMatrixRotate` (`FldFbnApplyTable`, the field-10 scenes) crashed with SIGSEGV at `0xfffffff33c55c4` in 2 of 3 runs of a gate; fixed (`t5 = t5 + t7`), 5 runs in 5 passed, and the other reloads of that function do not add to the pointer.
   `guest.c` now names the host address of a fault that is not a guest address (`p4 fault: signal 11 at host address ...`).
3. **Two renderer findings only a real GPU shows** (renderer_port.md 10.6c): pixel centres on quad edges (the title's letterbox bars a row low) and the shadow camera's clear (a black polygon under the player).

Device: SM-G986U1 (Snapdragon 865, Adreno 650, 8 GB), Android 13, OpenGL ES 3.2 (Qualcomm `V@0502.0`), colour output rounds to nearest (the shaders' `P4_ROUND_BIAS`), debug APK (CheckJNI on), disc image and
references in `/sdcard/Android/media/org.p4pc.game/files` (the app's own media directory), run with `tools/android_prog.py` and the gate scripts through `tools/android_bin` over adb.

This bring-up table and the `cfcbe012` viewer MADs below are historical offline-viewer qualifications, predating the isolated `.debug` application ID. The package/media paths in their evidence describe those old runs, not current test instructions. They are not scores for the later canonical real-play fixtures, which validate the actual captured guest state and use their own limits (see `renderer_port.md`).

| Gate (viewer, on the phone) | Adreno 650 | Limit | llvmpipe (a Linux host) |
|---|---|---|---|
| title_mad | 0.7241% pass | 2.0 | 0.7172% |
| field_mad (600 frames, 46 s) | 1.4947% pass | 1.6 | 1.3896% |
| scene_sky_black_tint | 2.7253% pass | 2.8 | 2.4986% |
| scene_near_fog | 15.6120% | 15.6 (15.7 for a device) | 15.5371% |
| scene_save_butterfly_glow | 3.2052% pass | 3.4 | 3.0523% |
| scene_riverbank_vignette | 8.7081% pass | 8.8 | 8.7079% |
| scene_arcana_rankup_sparkles | 13.2889% pass | 13.4 | 13.3375% |

The game itself (`p4pc`, no test variables) boots from the disc image: logo, the opening movie (FFmpeg's MPEG-2 decode, `swscale` without NEON colour conversion), the title; 30 flips a second during the movie
(33.3 ms a frame, 593 of 600 over 20 ms because the movie holds each picture two flips), battery 29.2 to 29.3 degrees and skin 31.5 degrees after two minutes (no throttling at that load, which is not gameplay).
`newgame_to_field` (the title, New Game by autopress, the Velvet Room, Inaba, the Dojima house, 30500 flips to the living room) passes in 37 s: 950 flips a second with the draws dropped, ~300 a second over the last 4000 flips
drawn (`P4PC_TURBO_WINDOW=4000`: the game's logic and the Adreno together, vsync off), and the frame at flip 30499 is 0.3965% from `newgame_field.rgb` (limit 5.0).
`battle_menu` and `camp_menu` pass (6 s and 9 s). Before the ABI cutover (`4c5af607`), `scene_battle_floor_a` died with SIGSEGV in `btlEplStepColorPairFade` through an indirect-call stub. The post-cutover phone package (built from `33e8100e`) passes the same 1400-flip battle frame at 14.0654% (limit 14.4). The package includes the by-value float-struct rewrite and corrected effect-allocator return definitions; these changes were not isolated from one another in the hardware run.
On that package, `save_load` passes in 9.7 s (loaded/saved images differ in six volatile counter bytes, reset in fourteen), `story_to_0415` passes in 180.4 s (flip 140000, field 7:2, 04/15), and `voiced_event_0417` passes in 45.9 s (flip 25000, field 6:6, 04/18). `castle_2f_0417` faulted in `btlPacketStep` after 56 s, executing guest bytes near 0x1ae3c0, the same callback failure as x86_64, not an Android-only crash.

After the callback batch `e72d017d`, the combined phone package built from `cfcbe012` passes all ten acceptance/control gates: battle floor 14.0654% (5.9 s), save/load (8.7 s), story to 04/15 (181.2 s), castle 2F (110.1 s), home 04/17 (147.7 s), voiced event (46.3 s), forced-context-loss field 1.3926% (28.7 s), sky 2.6015%, riverbank 8.6183%, and arcana 13.2699% (about 29 s each). The viewer cases include the boot-lifecycle correction described in `port64.md` 6.7; all three field-10 cases keep the full rasterised history. A physical controller remains unexercised.
armeabi-v7a is not built (the port has no 32-bit ARM model).

Further hardware checks (PortARM, repeated on the accepted `ab36ae98` runtime, including the indirect-call hash lookup; APK built from `fc11803c`; private evidence in `<private data>/PortARM/phone-current-main`, captures and retail data never committed):

- **Touch:** no title driver or autopress: overlay Start skipped the movie, Cross opened the title menu, d-pad Down selected New Game, and Cross chose Normal difficulty and confirmed Yes. The run reached the fiction disclaimer and opening Velvet Room limousine movie. Button presses used stationary 180 ms `adb shell input swipe` gestures (an instantaneous adb tap can miss the 30 Hz poll). In the living-room field, Cross dismissed the save tutorial and 450 ms left/right swipes on the drawn d-pad moved and turned Yu and the field camera followed. The controls stayed outside the 4:3 picture at 2400 x 1080.
- **Pause and context replacement:** HOME for about 40 s and resume through Android Recents retained PID 27803 and 2400 x 1080. SDL's landscape orientation hint is set before initialisation; the manifest alone had been insufficient on resume. `P4PC_TEST_CONTEXT_LOSS=1800` then reported `GL context replaced; textures uploaded again`; the field, player, UI and subsequent touch movement survived. The driver did not naturally report `EGL_CONTEXT_LOST`.
- **Gameplay thermals and pace:** a separate 189.3 s foreground run included 162.8 s of repeated left/right overlay gestures, with no background interval. Eight steady 600-flip windows measured 33.331–33.336 ms per flip (about 30 flips/s). The HAL's current readings were skin 29.0 -> 29.9 C, battery 26.7 -> 27.3 C, AP 26.1 -> 29.0 C; Android thermal status was 0 at both endpoints. This is a short active living-room sample with dummy audio, not a long thermal soak or sustained-combat qualification.
- **Real audio:** a 12 s non-dummy opening-movie run registered app UID 10324 with AAudio: OUTPUT, `Run yes`, state 4 (STARTED), 48 kHz, two channels, write/read counters 566496/565632, zero XRuns. AudioFlinger associated PID 28728 with `org.p4pc.game`. The user's speaker media volume was already 0 and remained 0 before/after, so no acoustic listening claim is made; the app was stopped immediately after the stream-state capture. Other unattended tests use the dummy driver.

Each runner invocation clears the app's own `files/boot.log`; it leaves the device-wide logcat buffer intact.

**Accepted corrected-oracle/argument cohort (`298e3018`, exact-source arm64 APK):** all 16 selected hardware gates pass in 1003.7 s, with the valid `i686-none-linux-android24` layout oracle and reviewed NDK 18 census enabled. Binary SHA256 checks match all 31 pinned references on both the host and the phone. The live touch-driven process maps the guest block at the 4 GiB-aligned `0x6e00000000`. Battle floor is 13.3707%, forced-context-loss field 1.3917%, asynchronous minimap 0.1822%, and save/load passes in 8.9 s. Story to 04/15 passes in 222.3 s, castle 2F in 127.4 s, home 04/17 in 161.1 s and the voiced event in 39.4 s, at their declared field/date endpoints. The full-history sky viewer control is 2.5979%. Evidence stays private in `<private data>/PortARM/phone-298e3018/`; the automated gate logs are under `gates/`.

| Canonical real-play gate | Adreno 650 MAD | Applied full-frame limit |
|---|---:|---:|
| Near-fog | 3.8448% | 4.15% |
| Velvet windows | 0.8534% | 1.05% |
| Riverbank | 1.1291% | 1.30% |
| Rain | 2.8323% | 3.0% |
| Arcana rank-up | 2.1018% | 3.65% |
| TV-world entrance | 1.3767% | 1.65% |
| Shadow haze | 1.4428% | 1.65% |

All seven real-play guest-state checks pass. Near-fog additionally measures 3.0234% in the upper `(0,0,640,200)` region against 3.35%; these Adreno results use the default non-NVIDIA limits, not NVIDIA's tighter full-frame bounds.

With the overlay enabled (`pad=1`) and no title driver, autopress or scripted input, physical Start/Cross/Down presses select New Game, confirm Normal difficulty and reach the opening limousine movie. Living-room left/right gestures move Yu and the camera; the controls remain in the sidebars of the 2400 x 1080 display.

A real 40 s HOME/Recents pause retains PID 5765 and landscape. Forced context replacement at flip 1800 reuploads textures, and the field and subsequent touch movement survive. No naturally occurring EGL loss is claimed. The complete field/pause test lasts 199.58 s, including a 117.73 s post-resume active-gesture interval; steady post-recovery 600-flip windows measure 33.308–33.335 ms/flip (about 30 flips/s), excluding the pause and context-reupload windows. Current HAL readings over that active interval are skin 32.7 -> 32.6 C, battery 30.5 -> 30.2 C and AP 31.0 -> 31.3 C, with thermal status 0 at both endpoints. This is a short living-room sample, not a sustained-combat thermal soak.

A short real-output run (12 s wait before capture) registers app UID 10327 / PID 6958 with AAudio and AudioFlinger: STARTED, 48 kHz stereo float, write/read 559776/558912, zero XRuns. Speaker media volume remains 0 before/after; no acoustic listening claim is made. The app is stopped immediately after capture. The test APK and its app-owned media are removed and the phone lock released before waiting on host-side qualification.


## 5. iOS: EGL on a CAMetalLayer

SDL3's UIKit video driver offers only EAGL. `src/render/p4gl_egl.c` is the context layer for iOS (`P4PC_GL_VIA_EGL`, automatic for `CMAKE_SYSTEM_NAME iOS`,
`-DP4PC_EGL_NATIVE=ON` elsewhere): it `dlopen`s ANGLE's `libEGL` from the app bundle's frameworks, opens the **Metal** display
(`EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE`), makes an ES 3.0 context and an `eglCreateWindowSurface` on the `CAMetalLayer` of an `SDL_WINDOW_METAL` window
(`SDL_Metal_CreateView`, `SDL_Metal_GetLayer`), and swaps with `eglSwapBuffers`. `src/render/p4gl_win.h` maps the window-system calls of `p4gl_context.c`,
`p4gl_gl.c` and `pad_sdl.c` to either this or `SDL_GL_*`; entry points come from `eglGetProcAddress`. Context loss and the blit present need nothing more.

CI has no iOS SDK (Apple's licence does not allow redistributing one and osxcross needs it), so `tools/ios_check.sh` (last step of `tools/android_ci_run.sh`) compiles
`p4gl_egl.c` and a unit that uses `p4gl_win.h`'s EGL mapping for **arm64-apple-ios13.0** with `-Wall -Wextra -Werror`: clang's own builtin headers, SDL3's public headers (they read
`__APPLE__` and `TargetConditionals.h`), the Khronos EGL headers, and stub libc declarations (`ci/ios/stubs`, from Darwin's documented signatures). That checks syntax, types and the
64-bit pointer model of the plumbing. It is not a link or a run: ANGLE's frameworks in the bundle, the Metal back end, `SDL_Metal_CreateView` on a real `UIWindow` and code signing
need Apple hardware.

## 6. Open

- **arm64-v8a.** Builds and runs on a Galaxy S20+; the hardware story, battle, save/load and viewer gates in section 4b pass.
- **Devices.** The x86 emulator (RTX 4080 GLES translator) and Adreno 650 have been exercised. Still untested: Mali/PowerVR, a physical Bluetooth controller, naturally occurring `EGL_CONTEXT_LOST` on pause (forced replacement passes), sustained-load thermals, acoustic output at an audible user-selected volume, and `adb`-less document-picker launch on real storage providers (the picker path was run with `file://` URIs through the same `openFileDescriptor`).
