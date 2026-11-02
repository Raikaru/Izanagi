# Izanagi

Izanagi is an unofficial native port of **Shin Megami Tensei: Persona 4** (PlayStation 2,
USA, `SLUS-21782`) to Windows, Linux and Android. It runs the game's own code,
recompiled from the [Persona4-Decompilation](https://github.com/Raikaru/Persona4-Decompilation)
project, on a native platform layer and an OpenGL renderer. It is not an emulator.

**Status: playable, work in progress.** Expect crashes, rendering differences and
unfinished features. Bug reports with logs are welcome (see [Reporting bugs](#reporting-bugs)).

**No game data is included or downloaded.** You need your own copy of the game:
the port reads the game's executable, models, textures, music and text from a disc
image you made from your own disc. See [Bring your own ISO](docs/bring_your_own_iso.md).

This project is not affiliated with or endorsed by Atlus or SEGA. Persona and
Shin Megami Tensei are trademarks of their respective owners.

## Install

Release packages are attached to the [GitHub releases](https://github.com/Raikaru/Izanagi/releases):

- **Windows (x64):** unzip `p4pc-win64-*.zip`, keep its DLLs beside the exe, run `p4pc64w.exe`.
- **Linux (x86_64, glibc 2.41 or newer):** `chmod +x p4pc-linux64-*.AppImage`, then run it.
  Without FUSE add `--appimage-extract-and-run`. Mesa or a vendor OpenGL driver is required.
- **Android (arm64) and Linux aarch64:** build from source ([BUILDING.md](BUILDING.md)).

On first launch the port asks for your `.iso`, checks it is the USA disc and remembers it.
Keep the ISO where it is; the port reads game files from it on every run.

## Playing

- **Settings:** the game's own **Config** screen is extended with display, audio, controls
  and mod options. Open it with **Esc**, **F1**, a **Select + Start** hold, or the corner
  button on touch screens. Choices are saved to `p4pc.ini`
  ([where it lives](docs/platform/settings.md#file-and-precedence)).
- **Widescreen:** `[video] aspect = 16:9` (or `auto` to follow the window, including
  ultrawide); `4:3` is the default. Movies use `movie_fit = bars`, `zoom` or `stretch`.
- **Saves:** stored in the per-user data folder next to `p4pc.ini`, not inside the package.
  Back them up before uninstalling on Android.

### Continue a PS2 save

Import a USA Persona 4 save, then use the game's **LOAD GAME** screen:

```sh
p4pc64w --import-save SAVE.PSV          # Windows; on Linux pass the same arguments to the AppImage
p4pc64w --import-save CARD.ps2
p4pc64w --import-save SAVE.max --slot 0 # --slot N replaces slot N (0..15)
```

PSV, PSU, MAX, CBS, SPS/XPS and PCSX2 memory card images or folders are accepted.
**System → Config → Import save** does the same from the game. Details:
[save import](docs/platform/saves_memcard.md#importing-ps2-saves).

## Reporting bugs

Open an issue with:

- the package name (for example `p4pc-win64-v0.1.0.zip`; `VERSION.txt` inside it),
- what you did and what happened,
- `p4pc.log`, and `p4pc-crash.log` if it exists (both beside `p4pc.ini`; Android: see
  [packaging](docs/platform/packaging.md)).

Do not attach your ISO, the game executable, extracted game files or memory card images.

## Building

See [BUILDING.md](BUILDING.md). In short: clone with the `decomp` submodule, build the
`p4pc-ci` container from `ci/`, and configure CMake inside it. The build needs no game
files.

## Layout

| Path | What |
|---|---|
| `decomp/` | The decompilation (git submodule, never edited here) |
| `overrides/` | PC replacements for decomp functions, one file per retail address |
| `src/` | Port code: loader, platform layer, OpenGL renderer, audio, movies, mods |
| `include/p4pc/` | Port headers |
| `config/` | Build inputs: unit lists, symbol placements, the retail function-pointer table |
| `tools/` | Build-time generators, packaging scripts and developer tools |
| `tests/` | CTest suites |
| `third_party/` | Vendored libraries and licence notices ([index](third_party/README.md)) |

## Documentation

- [docs/bring_your_own_iso.md](docs/bring_your_own_iso.md): dumping and checking your disc, first launch
- [docs/porting.md](docs/porting.md): how decomp code is compiled, configured and linked
- [docs/platform.md](docs/platform.md): runtime subsystems and settings
- [docs/renderer_port.md](docs/renderer_port.md): the OpenGL renderer
- [docs/pc_port.md](docs/pc_port.md) and [docs/port64.md](docs/port64.md): platforms and the 64-bit model
- [docs/modding/](docs/modding/): mods
- [CONTRIBUTING.md](CONTRIBUTING.md): rules for changes

## License

The original code in this repository (the platform layer, renderer, tools and tests)
is released under the [MIT licence](LICENSE). Third-party components keep their own
licences ([THIRD_PARTY.md](THIRD_PARTY.md)).

Code that reproduces or translates the retail game's machine code, including
`overrides/`, `drafts/` and the CRI middleware translations in `src/cri/`, is a
reconstruction of software owned by its original rights holders. No licence to that
material is granted (SPDX `LicenseRef-reconstructed`; see [NOTICE](NOTICE)), and no
game assets are distributed.
