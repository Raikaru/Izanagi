# Make a mod

You can change files, react to game events, add settings to MODS, and show a HUD
notice. You need your own USA Persona 4 PS2 disc image; no game data is supplied.
Start with [setup](start/install.md), then make [your first mod in 15 minutes](start/first-mod.md).

## Choose your path

- **Data only:** replace files using a package with `mod.ini` and `files/`; no
  script is needed. See [file packages](files.md#package). Named skill, enemy
  and item edits are available through [game data](systems/data-and-edits.md)
  (`data.luau`/`data/edits.xml` with capability `data.edit@1`/`data.xml@1`, and
  `data.session@1` for changes during a run). Advanced authors can also write
  table-source XML; do not guess row bytes or ship game assets.
- **Luau:** write `main.luau` for behavior, settings and events. The first-mod
  tutorial uses only the supported event, setting and UI APIs, not raw addresses.

The goal is to make realistic modding possible in plain Luau, including new
items, skills, Personas, enemies, encounters, shops, text, quests, audio,
visuals, UI, save data, randomization, difficulty and quality-of-life mods.
That is the direction of the API, **not a claim those capabilities all exist in
today's release**. In particular, there is still no ailment event and no
battle message-window helper; a battle-start HUD line is neither.

## Browse

1. **Start here:** [install and tools](start/install.md) · [first mod](start/first-mod.md)
2. **Learn by building:** [tutorials](learn/README.md)
3. **Understand the systems:** [packages, events and saves](systems/README.md) ·
   [capabilities](capabilities.md)
4. **Reference:** [API operations and minimum versions](reference/README.md) ·
   [capability keys](capabilities.md) · [manifest and file reference](files.md) ·
   [full Luau reference](lua.md)
5. **Troubleshoot & publish:** [common problems](troubleshoot/README.md) · [publishing](troubleshoot/publish.md)
6. **Advanced:** [raw hooks, memory and table sources](advanced/README.md)

The current release does not yet ship `p4mod`; do not use its planned commands
as if they were available. The [setup page](start/install.md#optional-type-checker)
shows the current source-built checker and its limitations.
