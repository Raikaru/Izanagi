# Understand the systems

[Modding home](../README.md)

## Packages and load order

An installed package is not automatically enabled. Changing the enabled list or
load order in MODS takes a restart. When packages replace the same file, the
later one wins; ordering defaults to `(priority, order, id)` unless the player
sets an explicit mod order. This rule is for file replacement, **not** a general
promise that raw hooks or every other kind of modification use last-wins.
[Package and ordering details](../files.md#shared-native-registry-and-config-discovery).

## Capabilities and compatibility

Capabilities name individual features and their revisions, independent of the
release's API minor. Use `requires` in the manifest for features your mod needs;
admission refuses a missing or too-old key before exposing the mod. For optional
behavior, use `p4.has` to check the player's build. See the
[capability reference](../capabilities.md) for syntax, keys and diagnostics.

## Settings and events

Manifest settings have a declared default and are stored per mod, separately
from saves. Live settings are read when the script asks for them; restart
settings read their admission snapshot until the next launch. A mod can receive
coarse events such as battle start and end, field entry, calendar changes and
save/load. They describe events, not an ailment-applied stream; do not infer
one from battle start. See [settings](../files.md#settings-api-12) and
[event payloads and timing](../lua.md#semantic-events).

## Saves and identity

Settings are not save state. `p4.save.register` lets a mod snapshot its own
state to a sidecar, with explicit load and optional authorization callbacks.
Actual semantic use can mark a save as requiring its provider; simply
registering a callback does not. Removing a used mod can therefore affect
loading. Back up saves before trying mods. The exact load/reset distinction and
provider rules are in [save provenance](../lua.md#save-provenance).

## Game data (named edits)

Named edits run while the game builds its tables, before the title screen:
`data/edits.xml` or `data.luau` in the mod folder, in load order. With
capability `data.session@1`, `p4.data.session.edit` can also change the loaded
tables during a run. See [game data](data-and-edits.md) for readers, edit
fields, conflicts and the session rules.

## Current table-source limits

Advanced table-source packages are prepared at startup. They can describe raw
rows and expanded content, but they are a different layer from the named edit
API above and require format knowledge. Consult
[advanced table sources](../tables.md#declarative-table-source-schema) before
using them. Never distribute disc-derived rows or assets. The planned semantic
data and conflict rules must not be mistaken for already-shipped behavior.
