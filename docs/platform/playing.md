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

The [settings file](settings.md#file-and-precedence) lives in SDL's per-user data folder:
`~/.local/share/p4pc/Persona4/` on Linux, `%APPDATA%\p4pc\Persona4\` on Windows and
`~/Library/Application Support/p4pc/Persona4/` on macOS.
Player saves default to its sibling `save/`, overridden by `P4PC_SAVE_DIR`.
On Windows, Linux and macOS, player startup redirects stdout/stderr to sibling `p4pc.log`. The previous run's log is kept
as `p4pc.prev.log` (and as `p4pc.unclean.log` when that run did not end by itself) instead of being truncated. Android
uses internal `boot.log` (with `boot.prev.log` and `boot.unclean.log`) plus logcat; its launcher has **Export logs**. Fatal native
signals on Linux, macOS and Android also retain `p4pc-crash.log` beside it; attach it too. What the logs hold, and how a player
sends them: [diagnostics](diagnostics.md).
The log's first line, and the first lines of every native crash report, name the build: `p4pc: build 49eee0d109ab` (12 hex digits of the
git commit; `-dirty` when the tree had uncommitted changes, `unknown` when no commit was known). The id comes from the source tree alone
(tools/build_id.py, include/p4pc/build_id.h), never the time, so building one commit twice gives identical binaries. A build made outside git
(the land check's container) takes the commit from `P4PC_CHECK_SHA`; a packaging script can set `P4PC_BUILD_ID=<40 hex>[-dirty]`.

A nonplayer run (test/harness/capture/headless switches, dummy audio, offscreen video)
may read an existing INI but never creates it or asks for a disc. Automated tools force
`P4PC_CONFIG=none` and isolated saves so player preferences and physical input cannot
change their evidence.

## Release contents

Build and packaging commands are in [BUILDING.md](../../BUILDING.md).

Windows packages are `p4pc-win64-<version>.zip` with `p4pc64w.exe`, its non-system DLLs,
`README.txt`, `THIRD_PARTY_NOTICES.txt`, `licenses/` and `VERSION.txt`. Packaging verifies
imports against system/shipped DLLs and requires the `ffmpeg_license` check with
`P4PC_REQUIRE_LGPL=1` against the shipped FFmpeg libraries. Linux packages include SDL3
and LGPL-only FFmpeg built from source with mandatory license checking and intact symlinks.
macOS packages are `p4pc-macos-<arch>-<version>.zip`: an ad-hoc signed `Persona 4 PC.app`
(SDL3, LGPL-only FFmpeg and ANGLE in `Contents/Frameworks`), README, notices and licences;
see [macOS](macos.md) for the bundle checks and the Gatekeeper first start.
No retail files, saves, captures or test environment overrides are shipped.

Staging and build trees stay below the packager's build directory (`build/package-*`);
only completed packages are written to `--out` (default `dist/`). The Linux AppImage is
assembled on a Linux file system so that library symlinks survive. `package_smoke.sh`
extracts and runs a package beside the zip by default, with hidden windows, dummy audio
and scripted input.

See [release packaging](packaging.md) for AppImage extraction/library replacement,
Android signing identity and symbol archives, debug-to-release migration, and crash logs.
