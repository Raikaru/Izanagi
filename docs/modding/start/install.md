# Install and set up

[Modding home](../README.md) · [First mod](first-mod.md)

## Game and editor

1. Install the port for your platform following the [release instructions](../../../README.md#install).
   On first launch, select an image made from your own USA Persona 4 PS2 disc.
   [Disc setup](../../bring_your_own_iso.md) explains what the port needs.
2. Run the game once normally and close it. This creates `p4pc.ini` in your
   [settings directory](../../platform/settings.md#file-and-precedence). On Windows,
   paste `%APPDATA%\p4pc\Persona4\` into File Explorer; on Linux it is
   `~/.local/share/p4pc/Persona4/`; on macOS it is
   `~/Library/Application Support/p4pc/Persona4/`.
3. Create a `mods` folder next to `p4pc.ini` if it does not exist. A plain-text
   editor is enough; save `mod.ini` and `main.luau` as text, not rich text.

**If you cannot find the INI:** launch the game normally, not with a test runner
or `P4PC_CONFIG=none`. A custom `P4PC_CONFIG` or `[mods] directory` changes the
location; `P4PC_MOD_DIR` takes precedence over the mod directory.

## Optional type checker

The current release does not include an installable `p4mod` command. You can
build the existing source checker separately with CMake, Ninja and Clang/Clang++
(C++17), without retail data. From the repository root, in a shell with those
tools on `PATH`:

```sh
cmake -S tools/luau -B build/luau-check -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/luau-check --target p4_luau_check
build/luau-check/p4_luau_check include/p4pc/p4.d.luau examples/tutorials/00-first-mod/main.luau
```

On Windows run `build\luau-check\p4_luau_check.exe` instead. To check your own
mod, replace the last argument with its `main.luau` path (quote paths with
spaces). `{"diagnostics":[],"ok":true}` means static analysis found no errors;
it does **not** validate the manifest, run the mod, or prove gameplay. Diagnostics
include file, line and column. The checker is a source-build tool, not the test
driver `tests/mod_api_check_test.py` and not a substitute for trying the game.

If your mod uses capability-only operations (for example `p4.has`), check
against the composed definitions instead of the base file:
`python tools/p4_capabilities.py compose build/p4.d.luau`, then pass
`build/p4.d.luau` as the first argument.

**If CMake says Luau profiles require Clang:** install Clang and set both compiler
options above; a CMake build directory already configured for another compiler
needs a fresh directory. If a file cannot be opened, run from the repository root
and check both paths. The checker accepts a definitions file plus one or more
source files; its `--help` is not an interactive help command.

Next: [your first mod](first-mod.md).
