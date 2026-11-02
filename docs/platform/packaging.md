# Release packaging

[Playing](playing.md) · [Settings](settings.md) · [Saves](saves_memcard.md)

Packages contain no game data. Players supply their own NTSC-U PS2 disc image.
Build from a clean committed checkout, in the `p4pc-ci` container ([BUILDING.md](../../BUILDING.md)).
The `build` workflow (`.github/workflows/build.yml`) runs both desktop packagers for
every pushed `v*` tag and attaches their output to the release.

## Windows

`sh tools/package_win.sh [--build DIR] [--out DIR]` builds `p4pc64w.exe` and creates
`<out>/p4pc-win64-<version>.zip` (default `dist/`; `<version>` is `git describe --tags --always --dirty`):
the exe, the SDL3 and LGPL-only FFmpeg DLLs it loads, `README.txt`, `THIRD_PARTY_NOTICES.txt`,
`licenses/`, the Luau source and `VERSION.txt`. Before it zips, it checks that the exe
imports only system DLLs and the shipped ones, and runs `tests/ffmpeg_license_test.c`
under Wine against the shipped FFmpeg DLLs with `P4PC_REQUIRE_LGPL=1`.
`P4PC_ISO=<your disc image> sh tools/package_smoke.sh <zip>` boots a package on a
Windows machine the way a player starts it (Git Bash), hidden and with scripted input.

## Linux

`sh tools/package_linux.sh [--work DIR] [--out DIR]` builds the x86_64 release with SDL3 and an
LGPL-only FFmpeg, then creates `<out>/p4pc-linux64-<version>.AppImage`. Debian 13 / glibc 2.41
is the baseline, not Ubuntu 22.04. Mesa or the player's vendor driver is supplied
by the OS; it is not bundled. AppDir follows `usr/bin`, `usr/lib`, desktop-entry
and hicolor-icon conventions. The icon is port-owned, not extracted game art.
The host-side packer is the checksum-verified appimagetool 1.9.1 release,
cached under its versioned name rather than the mutable continuous build.

Make the file executable and run it. Without FUSE, use
`./p4pc-linux64-<sha>.AppImage --appimage-extract-and-run`. To replace FFmpeg,
extract with `--appimage-extract`, replace ABI-compatible `squashfs-root/usr/lib/`
objects and run `squashfs-root/AppRun`. Complete FFmpeg/SDL sources and the build
recipe are in `usr/share/p4pc/source`; notices and licences are in the AppDir.
The launcher does not override XDG paths: settings, saves and logs remain under
`$XDG_DATA_HOME/p4pc/Persona4/` (default `~/.local/share/p4pc/Persona4/`).

An arm64 Linux package needs the AArch64 run environment (`ci/Dockerfile.a64`); the
packager builds x86_64 only.

## Android

`P4PC_ANDROID_BUILD=release` with `tools/android_ci_run.sh` ([Android](../android.md)) builds
the unsigned arm64-v8a release APK and AAB. Align and sign them with your own release key:
`zipalign -f -p 4`, then `apksigner sign` / `apksigner verify` for the APK and `jarsigner`
for the AAB. Keep the keystore and its password outside the repository and back them up:
a later release must be signed with the same key to update an installed one.

Set `P4PC_ANDROID_VERSION_NAME` to `git describe --tags --always --long` and
`P4PC_ANDROID_VERSION_CODE` to the commit count of that revision, so it increases along the release branch.
Release Java uses R8 shrinking/optimization; JNI callback names are kept.
Native libraries are stripped. Retain the exact per-release `symbols-*.tar.gz`,
unstripped `libmain` and R8 mapping; never symbolicate a crash against a newer build.
Use NDK `llvm-addr2line -f -C -e libmain.so.debug <module-offset>` or `ndk-stack`;
R8's `retrace` uses the matching `mapping.txt`.

The APK/AAB contains notices, licence files, complete unmodified SDL/FFmpeg
source archives and the Android build recipe. FFmpeg remains dynamically linked;
modified libraries can be repacked and signed with the player's own key.

Windows, Linux and Android packaging also stages Luau and inherited Lua MIT
notices, plus the exact vendored Luau sources and local patches. The notices
describe Luau and the C++ runtime conditionally: a build before game linkage
need not contain them. `source/runtime-provenance.txt` (under
`usr/share/p4pc/` on Linux and `assets/` on Android) records the actual build
toolchain, hashes of its static runtime archives, and source provenance.
Linux records Debian source-package and `Built-Using` information for
libstdc++/libgcc; Windows records llvm-mingw/LLVM; Android records the NDK
revision and Android LLVM source/cherry-pick information. GCC runtime-exception
and LLVM libc++ notices accompany the corresponding platform packages.

### Isolated automated phone tests

The ordinary debug build installs as **P4PC Debug**, `org.p4pc.game.debug`, using
the persistent debug key. Its UID, files, settings, saves, picker grants and media
inbox (`Android/media/org.p4pc.game.debug/files/import`) are separate from the
player's release app, `org.p4pc.game`. Install the `*-debug.apk` from an
`arm64-v8a` debug build with `android_prog.py setup`; all settings/import/package
smokes and `android_bin` gates must use this variant.

`setup --apk` checks the manifest with SDK `aapt2` **before any adb command** and
rejects the release ID or a non-debuggable APK. Put `aapt2` on PATH or supply
`ANDROID_AAPT2` / `setup --aapt2 <build-tools>/aapt2.exe`. The runner has no
production-package override; never uninstall/update the real app to run a test.

### Migrating a legacy same-ID debug install

A release key cannot replace an old debug signature installed as `org.p4pc.game`
with `adb install -r`. This is a deliberate one-time migration, not a test path.
Before migration, force-stop that legacy app and back up
everything `run-as` can read, including saves, INI, imported cards and preferences.
Verify the backup's complete file list and SHA-256 values before uninstalling.
Preserve external media too. The release build also writes a migration APK (`app-migration`): a private
debuggable bridge you sign with the release key and never distribute. Install that bridge
after removing the old debug app, restore and hash-check the complete sandbox,
then `adb install -r` the true non-debuggable release; its data remains intact.
Reselect the ISO through the picker if Android revoked its old URI permission.
Verify restored saves and settings in the release. Future same-key release updates
retain data without the bridge. Never silently uninstall on a signature mismatch.

## Native crash logs

Linux and Android handle SIGSEGV, SIGBUS, SIGILL, SIGFPE and SIGABRT. A crash writes
`p4pc-crash.log` beside the existing log: Linux's `p4pc.log`, Android's `boot.log`
in the app's internal files directory. It records signal/code, fault address,
registers, instruction/module offset, a native stack and the same guest address /
coroutine guard-page note as Windows. Android additionally writes tag `p4pc` to
logcat and the next launcher visit names the retained log. Normal startup does
not truncate it. Automated tools may select a private `P4PC_LOG_FILE`.

An alternate signal stack protects the game thread's report from stack exhaustion.
The minimal first record uses `write`; libc backtracing and symbol lookup are
best-effort after heap corruption (not async-signal-safe guarantees). A second
fault cannot recursively overwrite the report. Crash exit is `128 + signal`.

`tests/crash_log_test.c` is a standalone deliberate-fault binary linked to the
production OS and guest layers, never a game crash switch. Build it with
`-DP4_GUEST64 -DP4_GUEST_CHECK -ffunction-sections -fdata-sections`, `os_posix.c`,
`guest.c`, `-Wl,--gc-sections -ldl`; Android also needs `-llog`. Run each signal in
a separate process with `<private-dir>/boot.log <signal-number>` and retain the
crash file before the next case. SIGSEGV accesses an inaccessible guest address,
so the fixture also exercises real guest context reporting.

## Qualification (2026-10-27)

Revision `468650dd`: the qualified packages were the x86_64 AppImage
(40,749,560 bytes), aarch64 AppImage (40,725,000 bytes), signed arm64 APK
(39,839,171 bytes), signed AAB (31,890,440 bytes), native symbol archive and
R8 mapping. APK v2/v3 signatures verify; versionCode is 618. Its public signing
certificate SHA-256 is
`aeb1172b1fa4566562be778772a6bafe7e5e4665b3dc1b2c4ac001ab9738fe7d`.
The AAB verifies with the expected self-signed-certificate warning. Inspection
confirmed stripped APK code, matching symbol/line sections outside it, and bundled
corresponding sources/notices (aapt removes SDL's outer gzip wrapper).

The x86_64 AppImage ran in a fresh `debian:stable-slim` container with only Mesa
runtime packages added; no CI libraries or extracted ELF were mounted:

```text
first-run: requests own disc image PASS
AppImage: extracted own-disc executable, real title offscreen, exit=0 PASS
menu harness: real title selector is ready
menu harness: native settings check title: PASS
```

The arm64 AppImage also passed this real-title script in a clean arm64 Debian
container under explicit `qemu-aarch64-static` (734.84 seconds including setup,
1200-second game budget). Native ARM desktop hardware was not exercised. The host
binfmt mask rejects AppImage's `AI` ELF padding, so cross-architecture tests must
invoke QEMU explicitly rather than changing the shipped AppImage.

On the locked Galaxy S20+ (Android 13), the non-debuggable release selected the
ISO through the actual Storage Access Framework picker, reached the real title,
read the restored `resolution=scale:1` INI, and imported one real PSV from the
media inbox. LOAD GAME then restored field 7:2 on 04/11 and exited 0. All 87
original sandbox files and all 63 original media files were restored byte-exact;
post-release hashes confirmed every original sandbox file still unchanged.
The imported 171,112-byte save also matched its source SHA-256. The release was
left running at the title with scripted input and silent audio; relaunch normally
for player input/audio.

All five fatal-signal fixtures passed on Linux and the phone (exits
139/135/132/136/134). The standalone Android binary also wrote under the real
app UID into `files/p4pc-crash.log`; the release launcher's next-launch notice
was observed. These are real fixture excerpts, not game fault switches:

```text
Linux:   p4pc: CRASH signal 11 code 2, fault address 0x7f14deadbeef
Android: p4pc: CRASH signal 11 code 2, fault address 0x72deadbeef
Both:    p4 guest fault: access on guest address 0xdeadbeef
Android: p4pc:   frame 2: 0x59af1fae30 (/data/local/tmp/p4pc-crash-test+0x3e30)
```

Private logs, inspected title/import/loaded-save images, artifact hashes and the
preservation receipt are in `<scratch>/release/PortPackaging-evidence/`.
The verified pre-migration backup is in
`<private data>/PortPackaging/phone-backup-20261027/`; neither it nor any game data
or signing secrets is committed. Temporary on-phone fault binaries/reports were
removed after qualification.
