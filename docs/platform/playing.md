# Playing and packages

[Platform index](../platform.md) · [Settings](settings.md) · [Save import](saves_memcard.md)

## Player startup and paths

A no-argument launch is the player path (`src/loader.c`, `src/platform/config/p4iso.c`).
Developer/gate invocations supply the ELF explicitly and do not run first-start dialogs.

Disc selection order is `P4PC_ISO`, then `[game] iso` in `p4pc.ini`, then SDL's system
file picker. `p4iso_locate_elf` requires an ISO 9660 disc with `SLUS_217.82` in its root.
A wrong file shows an error and asks again; cancel quits with a message. The validated
choice is persisted. The executable is extracted to `SLUS_217.82` beside the INI once,
with its size checked on subsequent launches; players need not extract it manually.

The [settings file](settings.md#file-and-precedence) lives in SDL's per-user data folder.
Player saves default to its sibling `save/`, overridden by `P4PC_SAVE_DIR`.
On Windows and Linux, player startup redirects stdout/stderr to sibling `p4pc.log`,
truncated on each launch. Android uses internal `boot.log` plus logcat. Fatal native
signals on Linux/Android also retain `p4pc-crash.log` beside it; attach both logs.

A nonplayer run (test/harness/capture/headless switches, dummy audio, offscreen video)
may read an existing INI but never creates it or asks for a disc. Automated tools force
`P4PC_CONFIG=none` and isolated saves so player preferences and physical input cannot
change their evidence.

## Release contents

Run the package tools in the `p4pc-ci` container from a clean checkout
([BUILDING.md](../../BUILDING.md)):

```sh
sh tools/package_win.sh             # x64 player zip
sh tools/package_linux.sh           # Linux x86_64 AppImage
P4PC_ISO=<your disc image> sh tools/package_smoke.sh dist/p4pc-win64-<version>.zip   # on Windows, Git Bash
```

Windows packages are `p4pc-win64-<version>.zip` with `p4pc64w.exe`, its non-system DLLs,
`README.txt`, `THIRD_PARTY_NOTICES.txt`, `licenses/` and `VERSION.txt`. Packaging verifies
imports against system/shipped DLLs and requires the `ffmpeg_license` check with
`P4PC_REQUIRE_LGPL=1` against the shipped FFmpeg libraries. Linux packages include SDL3
and LGPL-only FFmpeg built from source with mandatory license checking and intact symlinks.
No retail files, saves, captures or test environment overrides are shipped.

Staging and build trees stay below the packager's build directory (`build/package-*`);
only completed packages are written to `--out` (default `dist/`). The Linux AppImage is
assembled on a Linux file system so that library symlinks survive. `package_smoke.sh`
extracts and runs a package beside the zip by default, with hidden windows, dummy audio
and scripted input.

See [release packaging](packaging.md) for AppImage extraction/library replacement,
Android signing identity and symbol archives, debug-to-release migration, and crash logs.
