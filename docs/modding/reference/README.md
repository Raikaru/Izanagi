# Luau API reference: minimum versions

[Modding home](../README.md) · [Complete signatures and contracts](../lua.md) ·
[Type definitions](../../../include/p4pc/p4.d.luau)

The table records the historical minimum API for operations present before the
capability registry. A runtime accepts the same major and a minor no newer than
its own. Declare the feature keys your mod needs with `requires` (see the
[capability reference](../capabilities.md) for syntax and the current key list).
Arguments, return values, callback rules, errors and examples are in the linked
full reference. Version 1.0 below means the base API.

| Operation or field | Minimum | Details |
|---|---|---|
| `p4.api_version`, `p4.language`, `p4.language_version`, `p4.id`, `p4.log` | 1.0 | [Sandbox](../lua.md#sandbox-budgets-and-lifetime) |
| `p4.has`, `p4.capabilities` | 1.6 · `capabilities@1` | [Capabilities](../capabilities.md#checking-from-luau) |
| `p4.u64`, `p4.split64` | 1.0 | [Memory and 64-bit values](../advanced/raw-luau.md#guest-memory-function-words-and-declared-layouts) |
| `p4.events.on` for `frame_start`, `field_enter`, `field_leave`, `battle_start`, `battle_end`, `menu_open`, `menu_close`, `calendar_advance`, `save`, `load` | 1.0 | [Events and payloads](../lua.md#semantic-events) |
| `p4.events.on("exit", ...)` | 1.3 | [Events and payloads](../lua.md#semantic-events) |
| `p4.input.action`, `p4.input.sticks`, `p4.files.read` | 1.0 | [Input and files](../lua.md#winning-vfs-reads-and-delivered-input) |
| `p4.settings.get`, `p4.settings.set` | 1.2 | [Settings](../lua.md#persisted-settings) |
| `p4.ui.page`, `p4.ui.detail`, `p4.ui.back`, `p4.ui.close`, `p4.ui.active` | 1.0 | [Pages](../lua.md#native-config-pages) |
| `p4.ui.page`/`detail` with `listed = false` | 1.4 | [Pages](../lua.md#native-config-pages) |
| `p4.ui.status` with Camp placement or clear | 1.3 | [Status](../lua.md#status-rows-camp-api-13-hud-api-14) |
| `p4.ui.status` with HUD placement | 1.4 | [Status](../lua.md#status-rows-camp-api-13-hud-api-14) |
| `p4.lifecycle.session`, `tag`, `crashed`, `consume` | 1.3 | [Lifecycle](../lua.md#session-lifecycle-and-crash-markers-api-13) |
| `p4.save.register`, including `authorize_load`, `authorize_save`, `published` | 1.3 | [Save provider](../lua.md#save-provenance) |
| `p4.authorize.on` for `fusion` or `encounter` | 1.3 | [Authorizers](../lua.md#operation-authorizers-api-13); encounter launch itself is 1.4 |
| `p4.battle.formation` | 1.3 | [Formation](../lua.md#operation-authorizers-api-13) |
| `p4.clock.wall`, `updates`, `retail` | 1.4 | [Game services](../lua.md#game-services-api-14) |
| `p4.scene.state`, `available`, `warp`, `status` | 1.4 | [Game services](../lua.md#game-services-api-14) |
| `p4.encounter.count`, `info`, `launch` | 1.4 | [Game services](../lua.md#game-services-api-14) |
| `p4.camera.acquire`, `owner`; lease `set`, `get`, `active`, `input`, `release` | 1.4 | [Game services](../lua.md#game-services-api-14) |
| `p4.hud.groups`, `hide`, `hidden` | 1.4 | [HUD groups](../lua.md#hud-groups) |
| `p4.audio.volume`, `set_volume`, `tag_adxt` | 1.5 | [Audio](../lua.md#audio-volumes) |
| `p4.elements.active`, `revision`, `of_skill`, `of_weapon` | 1.5 | [Elements](../lua.md#element-queries) |
| `p4.fn`, `p4.describe`, `p4.call`, `p4.hook` | 1.0 | [Advanced hooks](../advanced/raw-luau.md#typed-hooks-at-retail-addresses) |
| `p4.mem.alloc`, `sizeof`, `read`, `write`, `read_string`, `write_string`, `read_bytes`, `write_bytes`, `layout` | 1.0 | [Advanced memory](../advanced/raw-luau.md#guest-memory-function-words-and-declared-layouts) |

Page row and save fields have additional restrictions described in their linked
sections. A version or capability requirement does not validate native context,
callback timing, or the contents of a package. Capability keys and their
composed Luau definitions are described in the
[capability reference](../capabilities.md#the-list-of-keys).
