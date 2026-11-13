# Troubleshooting

[Modding home](../README.md) · [Publish a mod](publish.md)

- **My mod is absent from MODS:** put a real directory containing `mod.ini` in
  the configured mods root, not an extra nested archive folder. The default root
  is `mods` beside `p4pc.ini`; environment overrides can change it. Check the
  id and the six mandatory `[mod]` keys in [package rules](../files.md#package).
- **A mod is installed but does nothing:** enable it in MODS and restart. An
  installed mod is not enabled merely because its folder exists. Confirm its
  own settings too.
- **Incompatible API / unavailable section:** use a runtime supporting the
  `api` minor and every `requires` key your mod declares; consult the
  [version index](../reference/README.md) and [capabilities](../capabilities.md).
  An unknown native section is refused, not silently ignored.
- **No status text:** ensure the toggle is on before the next battle, then
  inspect the log for a full status-row allocation. Rows are shared and limited;
  see the [first tutorial](../start/first-mod.md#3-enable-and-try-it).
- **A checker reports an error:** use the file/line/column from its JSON; check
  paths and compiler requirements in [setup](../start/install.md#optional-type-checker).
  Static checking does not run the mod.
- **Recovery offers a restart:** read which package was named. An error after
  exposure requires a fresh process; the game does not pretend it safely
  unloaded active native work. If no mod is identified, do not guess. See
  [recovery](../../platform/settings.md#optional-package-recovery).
- **`SDL_CreateWindow` or OpenGL fails in a scripted run:** check the platform's
  graphics driver and window setup. On Windows, `SDL_VIDEODRIVER=offscreen` may
  fail to initialize OpenGL; do not treat that failure as a mod or gameplay
  result. The current build may also fault after that failure.
- **When asking for help include the port version, mod version, steps and `p4pc.log`.
  Do not share your disc image, extracted game files or saves.
