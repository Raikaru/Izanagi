# Capabilities: what this build of Persona4-PC provides

Every feature a mod can use has a **capability key** with a revision number, such as
`events.battle_end@1`, `audio@1` or `manifest.resource-image@1`. A newer build may add keys or
raise a key's revision; it never takes a revision away. `api=` in `mod.ini` still names the
oldest API minor the mod's manifest understands, but you no longer have to work out which
minor added a feature: name the keys you use instead.

## Declaring what your mod needs

Add `requires` to the `[mod]` section, with the keys separated by commas:

```ini
[mod]
id = example.poison-notice
name = Poison notice
version = 1.0
api = 1.6
priority = 0
order = 0
requires = events.battle_end@1, ui.status@1
```

`events.battle_end` means `events.battle_end@1`. When the player's build lacks a key, or has an
older revision of it, the mod is refused before any of its files or code are used, and the
MODS list says why, for example:

```text
Requires capability events.ailment@1, which this build does not provide. Update Persona4-PC or check the key's spelling.
```

A misspelled list (an empty entry, capitals, `@0`) refuses the manifest itself:
`Invalid requires= list: use comma-separated capability keys such as events.ailment@1`.

## Checking from Luau

`p4.has(key)` is true when the build provides the key at that revision or later, so a mod can
use an optional feature only where it exists:

```luau
if p4.has("ui.status") then
    p4.ui.status("Hard mode", "hud")
end
for key, revision in p4.capabilities() do
    p4.log(key .. "@" .. revision)
end
```

Both are plain queries, legal in every callback. `p4.has` raises an error for something that
is not a key (`p4.has("UI Status")`).

## The list of keys

Each key is one file in [`include/p4pc/p4.d/`](../../include/p4pc/p4.d/), named after the key.
Its first line is `--!capability <key>@<revision>`, the next line says what the key covers, and
the rest declares its Luau types. `python tools/p4_capabilities.py list` prints every key this
source tree provides, and `python tools/p4_capabilities.py compose p4.d.luau` writes the
complete type definitions (the base [`p4.d.luau`](../../include/p4pc/p4.d.luau) plus every key's
declarations) for Luau Analysis or your editor's language server.

Keys in API 1.6:

| Key | Covers |
|---|---|
| `core` | `p4.api_version`, `language`, `id`, `log`, `u64`, `split64` |
| `capabilities` | `p4.has`, `p4.capabilities`, `requires=` |
| `events.frame_start`, `events.field_enter`, `events.field_leave`, `events.battle_start`, `events.battle_end`, `events.menu_open`, `events.menu_close`, `events.calendar_advance`, `events.save`, `events.load`, `events.exit` | one `p4.events.on` event each |
| `input`, `files`, `settings`, `lifecycle`, `save`, `audio`, `elements`, `clock`, `scene`, `encounter`, `camera`, `hud` | the `p4.<name>` namespace |
| `ui.page`, `ui.status` | hosted MODS pages; the Camp/HUD status line |
| `authorize.fusion`, `authorize.encounter` | one `p4.authorize.on` operation each |
| `battle.formation` | the battle-start party projector |
| `manifest.setting`, `manifest.table-source`, `manifest.resource-image`, `manifest.element-ui` | one `mod.ini` section family each |
| `manifest.require`, `mods.required` | `[require <id>]` sections and `p4.mods.required` ([files](files.md#requirements-api-16)) |
| `manifest.companion`, `contract` | `[companion <id>]` sections and `p4.contract.publish`/`read` |
| `advanced` | raw `p4.hook`, `p4.call`, `p4.fn`, `p4.describe`, `p4.mem`: build-specific addresses that may break on port updates |

Features added later list their key where they are documented ("capability `key@rev`").

## Releases

A release raises the API minor once a coherent set of keys has landed, and lists them here.
Keys that land between releases are usable at once through `requires=`; the minor only
matters for `api=`.

- **API 1.6**: the capability registry (`capabilities@1`, `requires=`), requirements
  (`manifest.require@1`, `mods.required@1`), companions and contracts (`manifest.companion@1`,
  `contract@1`), and every API 1.0–1.5 surface registered under its own key. The 1.6–1.19
  minors planned before the registry were never released; their features arrive under
  capability keys instead.
