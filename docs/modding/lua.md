# Luau API reference

The scripting language is the pinned **Luau 0.744** interpreter and source
compiler, vendored with its MIT licence. There is no Lua 5.4 compatibility layer,
JIT, CodeGen or native module loading. An enabled mod may contain `main.luau`;
data-only mods need no script. Source is limited to 1 MiB; precompiled bytecode is
not accepted. Each mod owns its VM, globals, allocator, scratch and subscriptions.
Installation never enables a mod. [File mods](files.md) describes discovery,
API-version checks, global ordering and the restart-required selection boundary.

Raw hooks and guest memory are documented separately in
[Advanced](advanced/raw-luau.md). Start with the
[first-mod tutorial](start/first-mod.md) for a semantic API example.

The bundled [`p4pc.example`](../../examples/mods/p4pc.example) is inert until
copied to the configured mod root and explicitly enabled. It shows the three
layers together:

- **File:** it ships `files/DATA/MOD_EXAMPLE.TXT`, published through the same
  VFS as the disc, and logs the winning bytes returned by `p4.files.read`.
- **Hook and memory:** it hooks the actual time-slot setter, reads and writes a
  small private numeric guest layout, and logs its counter. It snapshots a plain
  number into its required save-sidecar block and restores that number into
  fresh guest scratch when a save is loaded.
- **Config:** its `[setting counting]` toggle is a native row under MODS, on by
  default. The hook reads it at each call, so turning it off holds the counter
  at once.

`mod_example_game[64|64w]` is an opt-in, real-executable witness, registered
only with `P4PC_MOD_WITNESS_TESTS=ON`. Supply `P4PC_ISO`, `P4PC_ELF` and a
private-readable `P4PC_EXAMPLE_SEED` containing the story0415 card; run it
headlessly through local study admission with `P4PC_INPUT=script`,
`P4PC_MOD_WITNESS=1`, `P4PC_MODS=1` and `P4PC_PACKS=0`. It copies the seed,
drives the actual time-slot hook and two game saves, and reloads in a fresh
process with the toggle turned off in `mod-settings/p4pc.example.ini`. It checks
the file read, the authenticated sidecar, required provider, the restored
counter, and that the hook still runs but holds the restored value while the
toggle is off. On Android, pass `--driver python tools/android_prog.py run p4pc`
to [`tests/mod_example_game_test.py`](../../tests/mod_example_game_test.py) with
the isolated debug app set up by `android_prog.py setup`. It copies the mod
root, settings and card to the device and holds the shared phone lock
(`P4PC_PHONE_LOCK` must name it; set it empty for a private emulator). Outputs
are tagged non-reference witness artifacts; this is not a
retail comparison.

## Sandbox, budgets and lifetime

The sandbox retains Luau's base, coroutine, table, string, math, utf8, bit32 and
buffer facilities, including normal `pcall`, `xpcall`, environments and coroutines.
There is no `os`, `io`, `debug`, `package`, `require`, source/bytecode loader,
filesystem library, `collectgarbage`, vector/integer library or native loader.
`p4.log(message)` emits one namespaced diagnostic, at most 4096 bytes. Public `p4`
namespace tables and native handle metatables are read-only.

VM allocation is capped at 8 MiB per mod. Luau collects only as it allocates,
so while a mod's heap is above half the cap the runtime also takes one bounded
collector step (512 KiB of work, uncharged) at the mod's first entry of each
presentation frame: garbage left by a large finished job is reclaimed over the
following frames instead of surfacing as "not enough memory" in a later
callback burst. A heap at or below half costs nothing. Initialization has 65,536
safepoints and a callback has 4096. Callbacks additionally share 8192 units
per presentation frame per mod: each callback entry and each safepoint consumes
one. These deterministic interpreter budgets are not millisecond CPU deadlines
and cannot preempt a native C builtin.
Budget failure is sticky across script `pcall` and coroutine boundaries.

`p4.budget()` (`budget@1`) returns how many units the current invocation may still
use: the smallest remaining fuel of its nested callback frames and, outside
initialization, of this presentation frame. Long work (a randomizer generating
a run) can run in a coroutine resumed from `frame_start` callbacks and yield
when the value falls below a margin. The number is advisory pacing, not a
measurement: it may differ between builds and platforms, so a result must never
depend on where the work was split.

Callbacks run on the native game thread. Each native coroutine incarnation gets
its own protected Luau callback stack. A native original that schedules another
game coroutine must not cause the second callback to reuse a suspended stack.
Native work is excluded from interpreter CPU accounting, and returning from a
native call restores the correct interpreter segment. Synchronous native service
callbacks prohibit `p4.call` and continuations rather than permitting an unsafe
native yield.

Lua executes under its own nearest-rounding floating-point environment, without
flushing denormals, on every platform, so `string.format` and arithmetic agree
between Windows, Linux and Android. Typed
native calls restore the caller's game environment; native changes to that
environment survive the eventual return to the game, including error paths.
An escaping init/hook/event error preserves its bounded diagnostic for the native
recovery screen. It never invents a successful game result or silently unloads
queued work; the two loading/failure paths are described below.

A mod admits **128 lifetime hook/event subscriptions**. Removed native handles
are tombstones until shutdown, so removing a subscription does not replenish
that limit. Disable/re-enable existing handles instead of allocating one each
frame. Removal is safe from its own callback; registration during native dispatch
is refused. Store reusable buffers and subscriptions in module globals.

This removes direct host filesystem and native-code facilities. It does **not**
make arbitrary calls into an unsafe C game safe for hostile code. Enable only
trusted mods. Public addresses are guest addresses, never host pointers.

## Typed hooks at retail addresses

```luau
-- Choose a researched address from this build's generated catalog.
local info = assert(p4.describe(address))
p4.log(info.signature)
local observer = p4.hook(info.address, info.signature, "after", function(ctx)
    p4.log(string.format("returned from %x", ctx.address))
end)
observer:disable()
observer:enable()
observer:remove()
```

`p4.describe(address)` returns nil for an unsupported address. The generated
`gen/mod_hooks.tsv` records supported signatures and exclusions; do not infer a
function name or ABI from an address spelling or stale decomp alias. Signatures
have a result letter followed by at most 12 parameter letters:

| Letter | Type |
| --- | --- |
| `v` | void result |
| `b`, `B` | signed/unsigned 8-bit |
| `h`, `H` | signed/unsigned 16-bit |
| `i`, `I` | signed/unsigned 32-bit |
| `q`, `Q` | signed/unsigned 64-bit, represented by an eight-byte LE buffer |
| `f`, `d` | float32/float64 |
| `p` | 32-bit guest data address; zero is NULL |

Variadics, by-value aggregates, unknown aliases, function-pointer declarators and
target-dependent scalar widths such as `size_t` and `long` are not guessed.
An incompatible signature fails before registration or native
invocation. Before hooks execute in global package order; after hooks execute in
reverse. A second active replacement is a reported conflict, not last-writer-wins.

```luau
local replacement = p4.hook(address, signature, "replace", function(ctx)
    ctx.args[1] = adjusted_argument
    ctx.next() -- original at most once; updates ctx.result
    ctx.result = adjusted_result
end)
local value = p4.call(address, signature, argument) -- original, bypasses root hooks
```

`ctx.next()` is valid only inside the replacement's current native coroutine and
callback. Retaining a context does not retain a live native stack frame; stale,
cross-coroutine and repeated continuations are refused. Recursive calls made by
the original still observe hooks. `p4.call` does not establish that a selected
operation is pure, legal at this lifecycle point, or free of RNG/cost/turn effects.
Never invoke a damage/accuracy calculator merely to draw a preview.

The generator instruments final mirrored definitions **after** overrides and ABI
corrections. Named calls, aliases and guest function pointers reach that same
entry, so a hook wraps a hand-translated override. Inactive entries perform only
a predictable enabled load/branch, without allocation, lookup, VM entry or argument
marshalling. Active typed bridges are separate. No executable patching, JIT or
platform-specific detour is involved.
An override's includes, defines, comments, types, data and helper functions may
precede its body: splicing leaves its `// FUN_` marker in front of the override's
own body (docs/porting.md), and the generator hooks the definition that marker
names, looking past includes and comments by that name, never by position, so a
helper is never hooked in its place. A conditional directive (`#if`/`#ifdef`/
`#else`/`#endif`) before the body records an `unsupported preamble` row instead of
guessing the active definition; old-style (K&R) definitions remain unhooked.
Bridge invokers honor the same forced CRT aliases as the mirrored unit: a
retail function such as EE `kill` reaches its `func_00421fb8` body, not a host
process API with the same spelling.
The table generator inspects the compiled bridge as a project archive, not a
host library; system-library names remain subject to the existing host guard.
The standalone i386/x64 task comparison links the real bridge and dispatch
objects, retaining only its reachable task hooks with linker section GC.

## Guest memory, function words and declared layouts

```luau
local scratch = p4.mem.alloc(256)
local a = scratch.address
p4.mem.write("u32", a, 123)
assert(p4.mem.read("u32", a) == 123)
local bits = p4.u64(0x89abcdef, 0x01234567)
p4.mem.write("u64", a + 8, bits)
local low, high = p4.split64(p4.mem.read("u64", a + 8))
```

Integer helpers are explicitly little-endian and permit unaligned accesses.
32-bit and narrower values are range-checked Luau numbers; 64-bit values are
exact eight-byte buffers, **not** doubles, signed host integers or decimal strings.
Use Luau's `buffer` operations for binary codecs. `f32/f64`, bounded strings and
byte buffers are also supported. String writes reject embedded NUL and require
capacity for the terminator. All stores validate the complete payload before
changing any bytes.

[Memory policy](memory.md) describes the source-backed write allowlist, executable
and readonly exclusions, real stack/debug-header protection and function-word
classification. Ordinary writable spans require actual image/heap evidence; a
numeric address inside the old RAM window is not authorization. Function cells
accept only nil or a `p4.fn(address)` reference to a known native dispatch entry,
not arbitrary integers. Declared function cells cannot be partially overwritten
through a scalar/string/byte store.

Scratch allocations are zeroed, 1 byte..1 MiB each and at most 4 MiB per mod.
On translated guest models they use the real guarded guest arena; exhaustion is
an error, never a host-pointer fallback. `scratch:release()` revokes that handle.
A declared view holds its own lifetime pin, whereas a naked numeric address does
not retain memory. Other mods cannot use private scratch through memory helpers.

```luau
local Record = p4.mem.layout<<{flags: number, id: number, skills: P4GuestArray<number>}>>({
    name = "ResearchedRecord",
    size = 0x30,
    fields = {
        flags = {offset = 0, type = "u16"},
        id = {offset = 2, type = "u16"},
        skills = {offset = 0x0c, type = "u16", count = 8},
    },
})
local record = Record:view(guest_address, true) -- read-only
p4.log(tostring(record.skills[1]))
```

In strict Analysis, supply the view type explicitly as above: the layout
specification describes runtime fields but cannot infer the generic view type.

Layouts validate bounded field extents, fixed array counts, nested layouts and
`type="string", capacity=N` declarations. Read-only restrictions propagate into
arrays and nested views. Out-of-range array reads return nil; writes and unknown
fields are refused. Layouts pin authorized memory but do not invent native-object
lifetime or relocate fixed retail tables. Internal pin/read/write closures are
not public memory methods.

## Winning VFS reads and delivered input

`p4.files.read(path[, maximum])` returns a Luau **buffer** containing the winning
read-only VFS bytes, bounded by at most 4 MiB. It neither reads host paths nor
bypasses existing developer/mod ordering. The VFS must be mounted. Resource
formats belong in mods using bounded buffer codecs, not private feature-specific
native parsers.

`p4.input.action(name)` returns `down, pressed, released` from the final delivered
game pad packet, after keyboard/gamepad/touch mapping and input fences. Names are
`confirm` (Cross), `cancel` (Circle), `menu` (Triangle), `help` (Square), directions,
`start/select/l1/r1/l2/r2/l3/r3` and the four face-button names.
`p4.input.sticks()` returns left x/y, right x/y, normalized about neutral 128.
Sampling activates on demand; getters never pump host events or resample devices.

## Persisted settings

`p4.settings` (API 1.2) reads and writes the mod's own values in
`mod-settings/<mod id>.ini` beside `p4pc.ini` ([Settings](files.md#settings-api-12)).

```luau
if p4.settings.get("backlog") then ... end           -- declared toggle: boolean
local style = p4.settings.get("description_style")  -- declared choice: its name
local ok, why = p4.settings.set("run.deaths", 3)     -- undeclared data key
```

For a declared key, `get` returns a boolean (toggle) or the choice name; an
`apply = restart` setting returns the value admitted at startup. `set` takes a
boolean or one of the declared choices; `nil` restores the default. Undeclared
keys (same key syntax) store a boolean, a finite number or a printable ASCII
string of at most 200 bytes without edge spaces, and read back as that scalar
or nil; the hub never draws them. A mod stores at most 256 keys.

`set` is a synchronous disk write on the game thread, meant for changes a player
or a game event causes, not per-frame state: it writes the complete file and
atomically replaces it before returning `true`, or returns `false, reason` with
nothing changed (an unwritable folder, or no settings file in this run). Call it
from hook or event callbacks or at load time. Calls from synchronous native UI
or action callbacks and from a save snapshot raise errors, as do invalid keys and
wrong value types. Declared rows are already adjusted by the native hub. Writes
do not mark a save as modded; record a setting in the mod's own save block when
a save must remember it.

A declared setting that an admitted mod names in its
[`[require]`](files.md#requirements-api-16) section is locked for the rest of
the run: `set` returns `false, "locked: required by <name>"` and changes
nothing, because admission checked the value that mod relies on.

`p4.mods.required(id)` (capability `mods.required@1`) describes one of the mod's own declared
requirements:

```luau
local split = p4.mods.required("p4pc.physsplit")
-- { version = "1.2", settings = { split = true } }
```

`version` is the admitted requirement's manifest version and `settings` holds
the current values of the settings this mod's `[require]` section names (toggles
as booleans, choices as names, as `p4.settings.get` reads them). An id the mod
did not declare raises an error rather than returning nil, so a typo fails
loudly. It only reads, so it is legal in every callback, including synchronous
UI callbacks and save snapshots; record the version in the mod's own save block
when a save must remember which requirement it was made with.

## Party records (`party.record@1`)

`p4.party` reads and changes a playable character's combat progression: the
Persona record of party ids 2..10 (Yosuke 2, Chie 3, Yukiko 4, Rise 5, Kanji 6,
Naoto 7, Teddie 8). A character's level is its Persona's level, so its HP and SP
maxima follow it. The protagonist (id 1) is deliberately excluded: his level and
EXP are his own record's, not a Persona's, and his Personas are the roster's,
which fusion, the compendium and Social Links own.

```luau
local r = p4.party.record(2)  -- {persona, level, exp, stats = {5}, skills = {...}} or nil
local ok, why = p4.party.set(2, {persona = r.persona, level = 1, exp = 0,
    stats = {3, 2, 2, 3, 2}, skills = {r.skills[1]}})
```

`set` checks the whole record before anything changes: the Persona stays the
record's own, the level is 1..99, EXP lies in that level's band of this Persona's
own curve, each stat is 1..99, and the skills are unique IDs of the active
(including expanded) skill registry filling the first slots. A refused `set`
returns `false, reason` and changes nothing. An accepted one is queued and
applied at the next frame safe point outside battle, revalidated against the
record as it is then; a write that no longer fits fails into mod recovery,
never a partial write. Current HP/SP are lowered to the new maxima. Calls during
a battle, in synchronous UI/action callbacks or in save snapshots are refused,
and a second mod's write to an id with a pending write is refused; the same
mod's second write replaces its pending one. Writes and `p4.give.skill`/
`p4.give.exp` for one character apply in call order
(`docs/modding/systems/actions.md`): a write waits for the gives to that
Persona asked for before it, and gives asked for after it wait for it. A write
also waits while the game's skill-choice panel shows that Persona (the panel
holds a copy of its skills and writes it back when answered). Writes to other
ids are not held. A `set` is semantic use of the mod for the save.

## Persona stock and Social Links (`persona.stock@1`, `social.rank@1`)

`p4.persona.stock()` returns Yu's carried Personas in roster slot order. Each
entry is a read-only table:

- `slot` is 1-based, and `lead` is true for the equipped Persona.
- `persona` and `arcana` use the game's own numbers (`arcana` as
  `datPersonaGetArcana` 0x00109280 reads it).
- `level`, `exp`, `stats` (St Ma En Ag Lu) and `skills` (learned IDs in slot
  order) are copied from the roster record.
- `registered` is true when the compendium holds that Persona, and
  `registered_level` is the level of the compendium copy (0 when unregistered),
  so a mod can warn before consuming an unregistered or stronger Persona.

`p4.social.rank(arcana)` returns the rank, 0..10, of the Social Link with that
arcana, as the game itself answers (`cmmCommunityValueForArcana` 0x00107890).
It is 0 when that link has no record yet.

Both functions only copy scalars out of the game's records. Nothing is written,
no game RNG is drawn and no guest pointer is kept, so they are legal in every
callback mode, including synchronous UI callbacks, save snapshots and
authorizers.

## Story progression (`story.dungeon_cleared@1`)

`p4.story.dungeon_cleared(dungeon)` reads the current save's rescue-clear flag
for dungeon 1..7. Dungeon 1 is Yukiko's Castle. The argument must be an integer
in that range; other values raise an error. This is the game's saved progression
flag, not the party's level or a test of the current floor. It reads the current
game state without changing it or consuming RNG, and can be used in synchronous
callbacks and save snapshots. Observe it after the relevant save or new game is
loaded. This API does not expose flag writes.

## Weapon fusion transaction (`forge@1`)

`p4.forge.commit{character, persona, vessel, item, fee, signature?}` asks the
game to turn one carried Persona into a weapon. It consumes the Persona, one
unequipped vessel weapon and `fee` yen, and adds one `item`. Everything is
checked before anything is queued:

- `character` is a recruited fighting party member (1 Yu, 2 Yosuke, 3 Chie,
  4 Yukiko, 6 Kanji, 7 Naoto, 8 Teddie).
- `persona` is carried and not the equipped (lead) Persona, and it is not the
  last one.
- `vessel` and `item` are weapons (equipment band 0) that `character` may
  equip, and differ. At least one unequipped `vessel` is owned, and fewer than
  99 of `item`. Expanded item IDs work through the item tables.
- `fee` is at most the money held.

A failed check returns `false, reason` and queues nothing. Otherwise it returns
`true`, and the request (one at a time across all mods) applies at the next
frame safe point outside battle:

1. The `weapon_fusion` authorizer is asked at stage 1 (commit). A denial spends
   nothing.
2. Recruitment is checked again; if the character is no longer recruited, the
   request is refused without spending. The remaining checks also run against
   the current state; a request that no longer fits them is the mod's failure,
   never a partial forge.
3. The Persona is removed the way fusion removes a material that is not the
   lead (`datPersonaRemoveRosterMemberById` 0x0010AD80). Then the vessel and
   the fee are taken and the item is added.

`p4.forge.result()` then returns the outcome once, as `{outcome = "forged" |
"refused", reason?, character, persona, vessel, item, fee, signature}`, and nil
until there is one. `p4.forge.pending()` is true while the request waits.

`commit` is refused in synchronous UI callbacks and save snapshots; call it
from an ordinary event callback such as `frame_start`. A queued forge records
semantic use of the mod. The log line
`forge by owner N: character C persona P vessel V -> item I (fee F)` marks each
forge.

`p4.forge.vessel(character, item)` (capability `forge.vessel@1`) returns how
many unequipped copies of `item` `commit` would accept as the vessel for
`character`: the count when `character` is recruited and `item` is a weapon
that fighting `character` may equip, and 0 for anything else. It is commit's own vessel check, so it never
reads the item's display name: a weapon another mod renamed to a placeholder
still counts, while `p4.inventory.count` refuses such an item. Arguments are
IDs in commit's ranges (character 1-8, item 1-32765); others raise an error.
It changes nothing but, like `commit`, is refused in synchronous UI callbacks
and save snapshots.

## Equipment exchange (`equipment@1`)

`p4.equipment.get(character, slot)` returns the item a party member wears in a
slot (0 weapon, 1 armor, 2 accessory), or 0 for none. It only copies a value
and is legal in every callback.

`p4.equipment.equip{character, slot, item}` puts one held `item` on
`character`, and the item it replaces returns to the items, as the Camp
equipment menu does. Everything is checked before anything is queued:

- `character` is a fighting party member (1 Yu, 2 Yosuke, 3 Chie, 4 Yukiko,
  6 Kanji, 7 Naoto, 8 Teddie) who has joined (`datMemberIsUnlocked`
  0x00353DC0).
- `item` is in the item table and is equipment of `slot` that `character` may
  equip, as the menu lists it: equipment band (`datIdBand`) and character
  permission (`datEntryHasFlag`). An expanded item must also pass the item
  tables' own equipment check and be usable now. It is not already in that
  slot, and at least one is held.
- The replaced item, if any, is in the item table and fewer than 99 are held.
  The menu silently drops a replaced item when 99 are held; the exchange refuses
  instead, so nothing the player owns is lost.

`p4.equipment.check{character, slot, item}` runs the same checks without
queueing and returns the item that would be replaced (0 for none), or `nil,
reason`. A failed `equip` returns `false, reason` and queues nothing.
Otherwise it returns `true`, and the request (one at a time across all mods)
applies at the next frame safe point outside battle, after a weapon fusion:

1. The request is checked again against the state as it is then. A request
   that no longer fits is refused whole and nothing is written.
2. The menu's own commit (0x0013D5D0): the replaced item's count rises by one,
   the item is assigned (`datAssignEquippedItem` 0x00106D40), then its count
   drops by one.

`p4.equipment.result()` then returns the outcome once, as `{outcome =
"equipped" | "refused", reason?, character, slot, item, previous}`, and nil
until there is one; `previous` is the replaced item (0 for none).
`p4.equipment.pending()` is true while the request waits.

`check` and `equip` are refused in synchronous UI callbacks and save
snapshots. A queued exchange records semantic use of the mod. The log line
`equip by owner N: character C slot S item I (replaced P)` marks each exchange.

## Velvet Room command (`velvet.command@1`)

`p4.velvet.command{page, label, operation?}` gives the Velvet Room one mod
command and returns an enable/disable/remove subscription. Only one mod can
hold it at a time; a second registration is refused.

- `page` is a page handle from `p4.ui.page`, or its global key.
- `label` is RAW font text of 1 to 47 bytes.
- `operation` may only be `"weapon_fusion"`. The command is then offered only
  while that authorizer allows stage 0 (offer). It is asked once each time the
  root list settles, so a denial is logged once per visit, not every frame.

When the Velvet root's three-row list is waiting for input, a hint row is drawn
under it: `SQUARE (<key>)  <label>`, where `<key>` is the first keyboard key
bound to Square. Square, which the root list never reads, then opens `page` in
the real Config task created under the Velvet task, in the hosted mode of
`docs/platform/settings.md` "Hosted flows". The root reads no input until the
page closes (Back, Esc, `page_back()` or `page_close()` on its top view), then
takes input as before. The log lines `Velvet command opened page K` and
`Velvet command page closed` mark each visit.

Removing, disabling or replacing the command is refused while its page is open.
With no command registered the Velvet Room is unchanged.

## Session lifecycle and crash markers (API 1.3)

`p4.lifecycle` tells a later launch whether an earlier one ended in a crash
that the port itself recorded, for rules such as STRICT IRONMAN that must tell
a crash from a deliberate exit.

```luau
local session = p4.lifecycle.session()     -- this launch, >= 1
p4.lifecycle.tag("R1:B7")                   -- what a crash now records for this mod
local crashed, tag = p4.lifecycle.crashed(previous)
p4.lifecycle.consume(previous)             -- after the mod durably reconciled it
```

`session()` allocates this launch's identity on first use: a durable counter in
`mod-lifecycle/session` beside `p4pc.ini`, written and atomically replaced
before it returns, and an empty `crash-<session>.marker` opened before any
fault. It returns `nil, reason` without a settings directory or when the
counter cannot be read or written; a corrupt counter is refused, never reused.
`tag(text)` (1..63 printable ASCII bytes; `nil` clears) is what the crash
handler records for this mod; at most eight mods hold a tag at once.

Only the port's crash handler writes a marker, as its first step, with
write/flush calls on the already-open file and no allocation or Lua (POSIX
fatal signals; Windows fatal exceptions). A clean exit, a force-kill, an OS
kill or power loss leave the marker empty, so they are never "crashed".
`crashed(session)` returns `true` plus this mod's tag (or nil when it had
none), `false` when that session recorded no crash (or is this one), and
`nil, reason` when its marker is unreadable or names another session.
`consume(session)` drops this mod's record; the marker goes with its last one.
A later launch deletes empty markers and keeps the newest 16 recorded crashes
until they are consumed. `session`, `tag` and `consume` are disk writes,
refused, like `p4.settings.set`, in synchronous UI/action callbacks and save
snapshots.

## Element queries

`p4.elements` (API 1.5) answers which element a skill or weapon actually uses
once table mods have added or rebound elements ([Tables](tables.md)). It is
game-only and read-only: every answer comes from the frozen active element
snapshot through the same pure resolvers the battle uses, so it never marks a
table row as used and is legal in synchronous UI callbacks.

```luau
if p4.elements.active() then
    local e = p4.elements.of_skill(0x79)   -- {key, name, short, kind} or nil
    if e and e.weapon then ... end         -- Attack-style skill: ask of_weapon
end
local cached = p4.elements.revision()      -- changes when the snapshot changes
```

`active()` is `false` when no mod expands elements; the retail tables then hold
the answer, and `of_skill`/`of_weapon` return `nil`. Otherwise `of_skill(id)`
returns the skill's damage element after every provider's binding as
`{key = "p4:fire", name = "Fire", short = "Fire", kind = "magic"}`, with `kind`
one of `physical`, `magic`, `almighty` or `status`. A skill that takes its
user's weapon element (Attack and its follow-ups, unless a provider bound the
skill itself) returns `{weapon = true}`. `of_weapon(item)` returns the
equipment's element the same way. Unknown, inactive or out-of-range IDs and
skills without an element return `nil`, never an error. `revision()` is `0`
with no active snapshot and changes whenever one is published; the snapshot
is fixed for a session, so labels can be cached against it.

## Audio volumes

`p4.audio` (API 1.5) sets the port mixer's volume per category: `music`,
`effects`, `voice` and `movie`. It is game-only mixer state, not game state:
nothing is written to guest memory, no save is tagged, and the calls are legal
in synchronous UI callbacks.

```luau
local percent = p4.audio.volume("music")            -- 0..100, default 100
p4.audio.set_volume("voice", 60)                    -- true, or false if refused
p4.audio.tag_adxt(handle, "voice")                  -- from a hook on an ADXT start
```

Sample voices (the sound driver's effects) count as `effects`, movie audio as
`movie`, and every ADX stream as `music` until `tag_adxt(handle, category)`
names the category of the stream an ADXT handle (its guest address) plays;
it returns `false` when the handle's renderer word names no live renderer.

Each mod has its own volume per category, and `volume` returns the caller's.
The mixer uses the product of every mod's factor, so one mod never silently
overrides another; the port logs the combined value whenever it is not 100%.
The result multiplies inside the mix, after the game's own volumes and under
the player's master volume in Config, which stays authoritative and scales the
whole output. At 100 every sample is exactly as before. A mod's volumes return
to 100 when its VM is torn down (failure recovery, shutdown) and are process
state: a mod sets them again when it starts and puts them back to 100 when its
feature is turned off. An unknown category or a volume outside 0..100 raises
an error.

## Artifacts (`artifacts@1`)

`p4.artifacts` keeps a mod's own files in `mod-artifacts/<mod id>/` beside
`p4pc.ini`, for output a player may want to keep or share (a spoiler log, a
manifest) and for caches the mod can always rebuild.

```luau
local ok, why = p4.artifacts.write("runs/1f2e/spoiler.txt", text) -- string or buffer
local bytes = p4.artifacts.read("runs/1f2e/layer.bin")           -- buffer, or nil, reason
local where = p4.artifacts.folder()                              -- for display; nil without one
```

A path is 1 to 8 `/`-separated names of `A-Z a-z 0-9 _ . -`, at most 128 bytes:
no empty, `.` or `..` name, no leading or trailing dot, no Windows device name
(`CON`, `PRN`, `AUX`, `NUL`, `COM1`..`COM9`, `LPT1`..`LPT9`, with any
extension) and no name that differs only by case from an entry already in its
folder. Invalid paths are argument errors. `write` creates the folders it needs
without following links or reparse points (an existing one refuses the write),
writes a temporary file and atomically replaces the target before it returns
`true`; anything else returns `false, reason` with nothing changed. Quotas are
checked before writing: 4 MiB per file, 256 files and 64 MiB per mod. `read`
returns the bytes (at most `maximum`, default 4 MiB) of a file in the mod's own
folder, or `nil, reason`. Without a settings file (`P4PC_CONFIG=none`) writes
are refused and reads find nothing. Like `p4.settings.set`, `write` is a
synchronous disk write refused in synchronous UI/action callbacks and save
snapshots; files do not mark saves as modded.

## Published contracts

`p4.contract` (capability `contract@1`) lets a mod hand plain read-only data to the mods that
declared it in `[require]` or [`[companion]`](files.md#companions-api-16):

```luau
-- p4pc.hardmode's main.luau
p4.contract.publish("run-profile", 1, { tier = "harder", physsplit = "1.2" })

-- a mod with [companion p4pc.hardmode]
local profile, version = p4.contract.read("p4pc.hardmode", "run-profile")
if profile and version == 1 then ... end
```

`publish(name, version, value)` is legal only while `main.luau` runs, so every
contract exists before any callback. The name follows the setting key syntax,
`version` is a positive 32-bit integer the publisher chooses for its own data
layout, and `value` follows the [`p4.save`](#save-provenance) value rules
(booleans, finite numbers, strings and acyclic tables without metatables), at
most 64 KiB encoded. The core keeps a frozen copy: a name is published once, a
mod publishes at most 16 contracts, and they disappear when its VM is torn down.

`read(owner, name)` returns a fresh copy whose tables are read-only, plus its
version, or nil when the owner is not admitted (an absent companion) or never
published that name. Reading an owner the mod did not declare raises an error.
Requirements and companions run `main.luau` first, so their contracts are
already there when a dependent's `main.luau` runs. Reads are legal in every
callback, including synchronous UI callbacks and save snapshots; nothing about a
contract is saved, so record what a save must remember in the mod's own block.
## Battle command panel

`p4.battle` (`battle.menu@1`) reads and steers the battle command panel while it waits
for the controlled party member's own input; party AI (tactics) turns, Rush,
enemy turns, all-out attack prompts and the battle simulator never count. Every
request goes through the game's own panel flow: nothing is pressed on the pad
and nothing happens without the game's own next step.

```luau
local menu = p4.battle.menu()        -- nil unless a panel waits for input
if menu and menu.panel == "commands" then
    p4.battle.choose("skill")        -- as if Skill were selected and confirmed
elseif menu and menu.panel == "skills" then
    p4.battle.point(2)               -- move the cursor to the second skill
    p4.battle.aim(enemy.unit)        -- that skill's target selection starts there
end
local units = p4.battle.units()      -- {party = {...}, enemies = {...}} or nil
```

`menu()` returns `{panel, cursor, first, count, page, unit, action, id,
command?, skills?, items?}`: the panel is `commands`, `skills`, `items`,
`tactics` or `persona`; rows are 1-based; `unit`/`action` are the actor's guest
addresses and `id` its character ID; the commands panel names the `command` at
the cursor, the commands and skills panels list the actor's skill IDs in the
skills panel's row order and the items panel its item IDs. `choose(name)` (commands panel: `analyze`, `tactics`,
`guard`, `attack`, `skill`, `persona`, `item`, `escape`) moves the cursor there
and makes the panel's next input read a confirm, so the game's own availability
check, sound and next panel follow; an unavailable command is refused by the
game as if the player had pressed it. `point(row)` moves a list panel's cursor
(and its first visible row). `aim(unit)` makes that unit the default of the
waiting action's next target selection when the game offers it as a
candidate; the player still confirms or moves it. `units()` lists the battle's
units as `{unit, id, data}` (BtlUnit, character or enemy ID, DatUnit).

Reads are legal anywhere. `choose`, `point` and `aim` raise errors in
synchronous UI callbacks and save snapshots, and return `false, reason` when no
matching panel waits. One mod drives a wait: the first whose request is
accepted; others get `false` and the driver's id until that panel's wait ends.
A mod's use is recorded for save provenance only when the player confirms a
panel it changed, never for reads or for requests that changed nothing.
Nothing observes the panel until a mod first calls `menu`, `choose`, `point`
or `aim`: the native observers are created disabled when the first mod VM
initializes, so that first call, from any callback (a hook, or `frame_start`
while `p4.clock.updates` observes `kwlnRunFrame`), only enables them.

### Enemy action substitution

`p4.battle.substitute(action, skill)` (capability `battle.substitute@1`) lets a
mod replace the command an enemy is about to use, from inside a `before` hook on
`btlAICommandSetActionCode` (`0x001de640`, signature `vppH`) for that call's own
action:

```luau
p4.hook(0x1de640, "vppH", "before", function(context)
    if wants_charge then
        local ok, why = p4.battle.substitute(context.args[1], 227) -- Power Charge
    end
end)
```

Only skills that need no individual target qualify: one that targets its user,
or one that hits every unit its sides collect, so the game's own collector
supplies the targets. With Physical Split or other expanded elements active,
the enemy AI carrier authorizes the substitution first
(`p4_game_elements_ai_native_substitute`). That is what lets the next decode of
this action accept a command the enemy's own table group or script did not
offer; the original random draw and decision stand. With no expanded-element
mod active (the usual case), there is no carrier: the call checks only the
skill and its target scope, and the retail decode simply uses the rewritten
`args[3]`. On success the call returns `true` and the hook's `args[3]` already
holds the new skill. An inactive skill or one with another target scope
returns `false` plus the reason, and so does a carrier refusal ("the enemy AI
carrier refused the substitution"). A call outside a `before` hook on
`0x001de640` (an `after` or `replace` hook included), for another action, or
from a synchronous UI/action callback or save snapshot raises an error. Save
provenance is recorded by the hook's own dispatch, not by this call.

## Native Config pages

The game installs the real `p4.ui` provider before source admission. It registers
pages with the existing native Config backend and attaches them under the owning
mod in the **single MODS hub**. It does not add per-mod permanent root rows, draw
an overlay/toolkit, impersonate native SERVICE pages or force a field/title menu.
Core-only VM fixtures intentionally have no UI provider; they never stub a native
settings implementation to make an API test pass.

`p4.ui.page(spec)` requires `key`, `title`, `count()`, `row(index)` and `find(key)`.
Lua row indexes are **one-based**; `find` returns nil when a stable key was
removed. Counts are bounded by the native backend's1048576-row limit.
Rows provide `key`, `label`, `kind` (`info/toggle/value/action/link`) and optional
`value/help/checked/disabled/restart/current`. A link supplies the registered
target's global page key. Toggle/value/action rows require `adjust(key,direction)`
with direction -1/+1 for changes or0 for activation; info/link-only pages may
omit it. Call `page:refresh()` after observable row membership/order/value changes
to advance the native snapshot revision, rather than acting on a stale index.
Optional `opened()`/`closed()` callbacks follow actual native view lifetime.

`p4.ui.detail{key,title,detail=function() return body end}` uses the same native
font's full-body wrapping/paging. It cannot mix row/adjust callbacks. Both
constructors return a readonly page handle with its globally namespaced `key`,
`:open()` and `:refresh()`. Local keys use1–59 ASCII letters/digits/underscore/
hyphen/dot; the global identity is `lua/<real-mod-id>/<local-key>`. With
`listed = false` (API 1.4) a page or detail is registered but not linked from the
MODS hub: only its own mod's link rows may open it (a link row naming another
mod's unlisted page is an error), back returns to the linking page, and it is
validated and torn down with its mod like a listed page. A mod can therefore
offer one hub row with nested sub-pages.
`:open()` returns whether the asynchronous request was accepted, **not**
visibility. `p4.ui.active()` returns the visible registered page key or nil;
`p4.ui.back()`/`close()` return accepted/refused status through the existing menu
gates. A refresh during shutdown returns false.

Text is the native font's **RAW encoding**, not UTF-8, MSG/BMD commands or host
HTML. English ASCII captions follow the existing native settings convention.

Bounds include NUL: page/global row keys128, labels/titles192, values128,
help2048 and detail32768 bytes. Embedded NUL and oversized values are errors;
captions are single-line and help/detail may use the page layer's LF separators.
Native validation visibly refuses malformed/unrenderable fields.

All provider callbacks are synchronous game-thread protected invocations with
the ordinary 4096-safepoint and per-frame limits. Native calls/continuations
that could yield are prohibited, as is registering a new screen from a
synchronous provider callback. Create screens during initialization or an
ordinary game event instead. Errors enter the same controlled native recovery
as hook/event errors, not an invented successful widget result.

The VM privately roots page userdata, callback snapshots and the last returned
row/detail, even if a script drops its public handle. Native code copies borrowed
strings before another provider call. On a proven native teardown boundary the
binding unlinks the owner's hub entries first, successfully unregisters every
owned page outside provider callbacks, then releases Lua roots before destroying
the VM. Failure to establish that lifetime enters controlled recovery, never a
fake unregister or a GC-owned native view. Selection changes remain restart-only;
there is no script-side module-unload shortcut.

The authored [`native_ui` fixture](../../tests/mod_fixtures/native_ui/main.luau)
uses the public API for toggles, stable-key actions, links and a long detail page.
For an admitted mod-test game run, select `witness.native-ui` from the
`tests/mod_fixtures` root with the explicit witness configuration and navigate the
real hub using scripted native input. Its `UI_READY/OPEN/ADJUST/ACTION/CLOSE`
markers are callback evidence only; actual native rendering/input/scroll/paging
still require labelled platform receipts. The Analysis CLI checks this same
fixture source. Source/fixture definitions are not passing game/visual receipts.

The UI flags are presentation metadata, not automatic persistence:
`restart` does not write configuration or request a restart. This page provider
does not expose general host INI/file writes or rich in-game message commands.

The fixture's explicit `Fail callback witness` action throws `UI_CALLBACK_BAD`
inside the real adjust provider. Use it only in the isolated mod-test lane to
exercise native recovery with a live UI callback; the scripted recovery choice
and resulting persisted selector change require their own actual receipts.

### Status rows (Camp: API 1.3; HUD: API 1.4)

`p4.ui.status(text, placement)` (API 1.3 for Camp; API 1.4 for HUD) replaces
(a string) or several (a table of strings); `p4.ui.status(nil)` clears them.
`placement` is `"camp"` (the default: the native Camp root; two rows in a band
above the menu header, then two above the bottom help line) or
`"hud"` (an always-visible layer drawn by its own native seam). There are four
rows in total across all mods and placements, kept in mod load order. A call
that would not fit beside earlier-loaded mods' rows returns false and keeps this
mod's previous rows; a later-loaded mod never displaces an earlier one. Lines
use the same RAW native glyphs as pages, 1 to 47 bytes each, and are validated
and copied when set; drawing never calls Lua, and with no rows nothing is drawn
or allocated. A Camp line wider than its 470-pixel band is narrowed to fit.
Four rows plus the CONFIG corner stay within the native font manager's per-frame
capacity (at most 5 text runs and 194 glyph nodes of its 256 and 1024).
HUD lines (API 1.4) are drawn every presented frame by the mod services'
`mainDrawFrameUpdate` (0x102550) seam into the last ordering-table bucket,
after every gameplay producer and the screen fades, so they stay visible over
fields, battles, menus and fade-outs and are never hidden by `p4.hud.hide`.
They start below any Camp rows the Camp root drew in the same frame and are
plain native text at three quarters of the native size, at most 300 canvas
pixels wide. The seam is enabled only while a HUD row, an overlay HUD item or
an overlay screen item exists; status rows draw first, then overlay HUD items,
then screen items, all in ordering bucket 0xd7, and none is affected by
`p4.hud.hide`. In a frame that draws a row or item the layer opens by clearing
the depth buffer: the battle command phase writes a nearer depth with its Z-only
TV outline before the last bucket, which used to hide every row and item there
(they draw with the depth test on), and nothing in a frame draws after that
bucket. A frame that draws none leaves the render list untouched.
The rows disappear when the mod's VM shuts down.

### Native sprites and overlays

Overlays (capabilities `ui.sprite@1` and `ui.overlay@1`; the `screen` context
is `ui.overlay@2`) put positioned native text and authored sprites where the
native context draws its own, so a mod's marker looks like the game's
([art families](ui_art.md)). They are the native replacement for status rows,
which stay unchanged.

```luau
local badge = assert(p4.ui.sprite("badge", {x = 0, y = 0, w = 111, h = 21}))
local camp = assert(p4.ui.overlay({context = "camp", fade = true, items = {
    {key = "badge", sprite = badge, x = 22, y = 64},
    {key = "cap", text = "CAP 15", x = 30, y = 92, plate = true},
}}))
camp:set("cap", "CAP 16")     -- validated and copied; drawn from the next frame
camp:show("badge", false)
```

`p4.ui.sprite(image, {x, y, w, h})` names this mod's own
`[resource-image <mod id>:<image>]` and a rectangle inside it. The image must be
a power-of-two atlas (as retail sprite packs are): the native sprite path divides
texel rectangles by the raster size, which equals the texture only then. It
returns a readonly handle with `width` and `height`, or nil and a reason (no
such frozen image, not power-of-two, no free sprite). Each handle is a synthetic
one-record `SdkSpritePack` in port-owned guest memory whose raster is the
pinned P4Resources image; retail packs and records are never touched. VM
shutdown removes the sprite immediately, but a pack already in the draw queue
keeps its image pinned until the next ordering-table clear. Retired packs count
against the limit of 32 sprites across all mods until that clear.

`p4.ui.overlay{context, fade?, items}` registers 1 to 8 items in one context:

| Context | Drawn | Queue |
| --- | --- | --- |
| `camp` | the Camp root's idle state (cmpMainTaskStep, beside the status rows) | Camp's centered table 0xb1 |
| `hud` | while the player walks a field or dungeon: hidden under menus, events, messages and battles, like the native field HUD (`mainDrawFrameUpdate` seam, enabled only while a HUD item or row exists) | last ordering bucket 0xd7 |
| `screen` (`ui.overlay@2`) | every presented frame, never hidden and never faded (`fade = true` is an error): for markers that must stay visible over menus, events and battles, as status rows do | last ordering bucket 0xd7, after `hud` |

Each item has a local `key`, canvas `x`/`y` (640x448) and exactly one of `text`
(1 to 47 RAW glyph bytes, font 1) or `sprite`. Optional: `align`
(`left`/`center`/`right` about `x`), `color` (0xRRGGBB text colour or sprite
tint, default white), `alpha` (0..255), `scale` (native size 1, up to 8),
`visible`, and for text `plate` (the status rows' native rounded plate) and
`width` (narrow a wider line to this many canvas pixels). `fade = true` fades
the items in over the native menus' 10 updates whenever the context begins
(the Camp root opening, or the player regaining control in the field). Every item is validated before any is registered; an
invalid item raises an error and nothing is registered. Capacity: 16 items
across all mods, and 4 text items per context, so with four status rows and the
CONFIG corner a field frame (HUD and screen items) stays within 13 text runs and
570 glyph nodes of the font manager's 256 and 1024. A registration that does not fit returns nil and a
reason and registers nothing. A native queue callback invokes `sdkSpriteDrawNow`
(0x25ea20) with the native tint, alpha, scale and depth. No separately allocated
sprite sample is left behind if the frame's queue is cleared without drawing.

The handle's `:set(key, text)`, `:show(key, visible)` and `:remove()` change
only presentation state. Both constructors and the methods are refused in save
snapshot callbacks, allowed in synchronous UI callbacks (they call no native
code) and are not semantic use. Drawing never calls Lua; with no item nothing
is drawn, allocated or queued. Everything a mod registered is removed when its
VM shuts down.

The authored [`ui_overlay` fixture](../../tests/mod_fixtures/ui_overlay/main.luau)
uses a Camp badge from a power-of-two atlas, a plated Camp line, a HUD line and
a plated screen marker. `mod_overlay_game[64|64w]` (registered only with
`P4PC_MOD_WITNESS_TESTS`, run under `P4PC_MOD_WITNESS=1`) captures the field and
the Camp root with the fixture installed but disabled and then enabled. It
requires the overlay rectangles to change, the HUD line to be absent under Camp
and the screen marker to be present in both. It is mod-witness evidence, never a
retail reference.

### Native rows

Some native menus lay out a fixed set of rows with hard-coded positions per row
count. `p4.ui.native_row` (capability `ui.native_row@1`) adds a mod row to such
a menu so it looks, moves, sounds and answers input exactly like the retail
rows. The first host is the Velvet Room's fusion card list (`"velvet_fusion"`).

```luau
local art = "card" -- [resource-image <mod id>:card], a 256x128 atlas
local card = assert(p4.ui.native_row({
    host = "velvet_fusion", key = "weapon", page = forge,
    symbol = assert(p4.ui.sprite(art, {x = 0, y = 0, w = 72, h = 77})),
    label = assert(p4.ui.sprite(art, {x = 80, y = 0, w = 99, h = 31})),
    small = assert(p4.ui.sprite(art, {x = 80, y = 40, w = 72, h = 18})),
}))
card:enable(offered)          -- hide the card while an authorizer denies it
```

The Velvet card list takes one mod card. It sits after the fusion spreads and
before Fusion Forecast, Talk and Back, so the list holds up to ten cards. The
card's art follows the Velvet card families ([UI art](ui_art.md)): a 72x77
symbol, a large label of at most 99x31 and a small label of at most 72x18, from
the mod's own power-of-two atlas. A mod row that does not fit these is
refused (nil and a reason), and so is a second one.

How the card behaves:

- **Layout.** Cards slide in, highlight, take the selection mark and fade like
  the retail cards, from the same layout tables. Ten cards use a wider arc of
  the same shape.
- **Confirm.** Cross opens the row's `page` (a page or detail handle of the
  same mod), hosted in the Velvet task's own Config child. The root reads no
  input until the page closes. The cards then slide back in with the cursor on
  the mod card.
- **Refusal.** A page that cannot open plays the native refusal cue and the
  list stays.
- **Enable.** `card:enable(false)` drops the card the next time the list is
  laid out. `card:present()` tells whether a list currently shows it, and
  `card:remove()` unregisters it.

Natively the list has nine `commandRows`. The tenth is a port-owned side row
that the card-list overrides read through `p4_native_rows_velvet_row`
(`include/p4pc/native_rows.h`). The card draws from a port-owned copy of the
loaded `facility/velvet01.spr` records with the card's records appended,
installed as the fcl sprite pool's pack. Retail records and packs are never
written. With no enabled mod card, every override runs its retail path and the
menu is byte-identical to retail.

Rows are presentation. They are refused in save-snapshot callbacks and are
not semantic use (the hosted page's own callbacks are). They are removed when
the VM shuts down, before the mod's pages and sprites.

## Operation authorizers (API 1.3)

`p4.authorize.on(operation, callback)` returns an enable/disable/remove
subscription. A native seam asks every enabled authorizer of that operation, in
mod load order, before the game commits the player's operation:

```luau
p4.authorize.on("fusion", function(context)
    return false, "NO FUSION challenge"
end)
```

The callback receives a readonly table of copied scalars and returns
`allowed, reason?`. The first denial wins: later authorizers are not asked, the
game takes its own path for an unavailable operation, and the denial is logged
with its reason. A denial records semantic use of the mod. Callbacks run like
save snapshots: no native calls, no guest or settings mutation, the ordinary
callback budget. An error or exhausted budget never authorizes; it enters the
native mod recovery. Without an enabled authorizer a seam costs one mask test
and the game is unchanged.

| Operation | Seam | Context |
| --- | --- | --- |
| `fusion` | Velvet Room fusion command rows (0x2ecfc0): a denied kind is not offered; the root menu's roster pair fusion (flag 0x1305, 0x2e7ac0) is not entered | kind (0 pair, 1 triangle, 2..4 cross spread with 4/5/6 materials, 5 twelve materials, 6 roster pair), materials |
| `encounter` | `p4.encounter.launch` (API 1.4), at the frame boundary before any teardown | encounter, source (1 = `p4.encounter.launch`) |
| `battle_protection_item` | Before a held party item is selected as a Light/Dark death-protection replacement in battle (`authorize.battle_protection_item@1`). A denial skips the item before inventory spending and continues the native passive and social-link checks. It does not restrict ordinary item commands or field item use. | character (party ID), item (native item ID), element (6 Light, 7 Dark) |
| `weapon_fusion` | the Velvet Room WEAPON FUSION command (capability `authorize.weapon_fusion@1`). Stage 0 (offer) is asked before its row is shown; a denial hides the row. Stage 1 (commit) is asked at the forge safe point before revalidation, fee or consumption; a denial spends nothing and the forge page shows the reason. Queueing a forge validates it before this commit-stage decision. The `fusion` operation never covers it. | stage (0 offer, 1 commit), character (party character ID), vessel (item ID; 0 at offer), persona (Persona ID; 0 at offer), signature (0 or 1) |

Unknown operations are refused. Browsing and compendium rows of the Velvet Room
are not fusion and are never gated.

Challenge's initially-off **NO WEAPON FUSION** rule denies both stages while
the run is active. Its value is fixed when the run is armed and restored from
the run's save; older Challenge saves without this rule retain it off. It is
independent of **NO FUSION**, which restricts Persona fusion instead. The
restriction is prepared against the semantic Forge provider and does not
constitute an in-game or hardware qualification.

`p4.battle.formation(callback)` registers the one battle formation projector;
a second registration while one exists is refused. When a battle creates its
player-side units (0x1b1d70), the callback receives `{name = "formation",
party = {ids}}` with the battle's own lineup of playable character IDs in
order (1 Yu, 2 Yosuke, 3 Chie, 4 Yukiko, 6 Kanji, 7 Naoto, 8 Teddie), and
returns nil to keep it or 1 to 4 unique IDs (1..10) to fight instead. The
encounter's party array is not rewritten, so its retail owner still releases
it; a character the battle did not set up uses one unit wrapper made the first
time it is needed and kept for the process. A lineup without Yu gives his
leader role (the battle's first selected action) to its first fighter, logged
as `battle leader role given to unit N`. The ally AI asks for the
protagonist's action without checking he is present (script command 0x207B,
0x1e6d20); in such a lineup that request answers with the leader-role
action, and any other unresolved request answers -1 instead of reading
through NULL. The ally AI also ranks the protagonist's HP and ailments by his
character ID alone (script commands 0x205A, 0x1e1530, and 0x205D, 0x1e1800);
in such a lineup those queries ask about the leader-role fighter instead, so
allies rank, heal and cure the leader by its own HP and ailments, with the
protagonist's cure priority and attack gate, and a dead leader answers as a
dead Yu would. Queries by any other character ID, including the AI's own-unit
identity, are unchanged. The turn-start counter used by the victory Shuffle Time
chance (0x1a06d0, 0x1ef7e0) counts that leader's eligible ordinary turns when
Yu is absent, under the same gates as Yu; a present Yu retains his retail count.
The projection changes only who fights: story party, field units, recruitment and control settings are
untouched, and anything else (who receives commands, defeat handling, EXP) is
the mod's to arrange through hooks. Like authorizers, the callback runs in
snapshot mode; an error or an invalid lineup enters mod recovery.

## Game services (API 1.4)

These namespaces are game-only (the core VM fixtures do not provide them).
Queries work in any callback. Requests, camera leases and HUD hides change the
running game or what it draws: they raise an error in synchronous UI callbacks
and save snapshots, and record semantic use of the mod. A request is validated
when it is made and carried out by the game thread at the next scheduler frame
boundary, before `frame_start` subscribers run, never inside a mod's hook
callback or a task. That boundary is still inside the game's frame
(`kwlnRunFrame`): while `p4.clock.updates` observes that function, the boundary
and every `frame_start` callback run inside its native hook dispatch, where no
native hook can be added.
Nothing here hooks or runs per frame until a mod first uses a service: the
native seams are created disabled when the first mod VM initializes.

### Clock

```luau
local seconds = p4.clock.wall()     -- host monotonic seconds
local updates = p4.clock.updates()  -- completed game updates
```

`wall()` is the host's monotonic clock in seconds since its first query in the
process. It includes loading and menus, follows the host rather than the game,
and is not deterministic: use it for presentation and timers only. `updates()`
counts completed `kwlnRunFrame` (0x101270) task updates since its first call in
the process; the function's reset branch and not-ready frames do not count.
`retail()` is the game's own counter at 0x00764290. Both are deterministic.
Presented frames are the `frame_start` event's `frame`. A save provider's
`load(value, origin)` marks loads and resets (boot, LOAD GAME, New Game), where
a timer or counter should start a new timeline rather than read a jump as
elapsed time.

### Scene state and field requests

`p4.scene.state()` returns a readonly table: `context` (`none`, `title`,
`field`, `dungeon`, `battle`, `event` or `other`), the sequence id, `ready`
with a `reason` when false, `control` (the player-control task exists),
`event` (a message window or event holds the input), `menu` (Camp or the native
Config is open), `pending` (a sequence change or service request is in flight),
the field and room ids, `entry` when the current field's entry record was
observed and `step` on a generated floor. `ready` means an idle field or floor:
field root state 13, the player in control and none of the above in progress.
The first `p4.scene` query starts observing field entries
(`K_SeqFieldRecordStart`, 0x170220) for the rest of the process.

`p4.scene.player_position()` (`scene.player-position@1`) returns the player's
horizontal world coordinates `x, z` in a ready field or floor, or `nil` when
the position is unavailable. Coordinates are local to the current field and
room; check `p4.scene.state()` before interpreting them. This is a read-only
query and never moves the player.

`p4.scene.warp{field=, room=, entry=0, direction=0}` returns `true` when queued
or `false, reason`. It requires a ready fixed field (sequence 9), field and room
0..999 with a `field/pack/fFFF_RRR.p00` resource on the disc, and no other
request in flight. At the frame boundary it repeats `K_Cmd_CALL_FIELD`
(0x176450): it waits for the scene resources, enables the scheduler and
requests sequence 9 with the 0x1C field record, so the game's own teardown,
load and ready path runs. `entry` and `direction` are passed through as that
script command passes them. A warp does not change story progress or flags.
`p4.scene.available(field, room)` answers whether the disc has that field and
room's `field/pack` resource, the same test a warp makes, without a request.

`p4.scene.status()` returns the state of this mod's last request, a detail
string and the battle outcome when there is one: `requested` (queued or
loading), `battle`, `returning`, `done` (with `arrived`, `returned` or why the
game finished it otherwise) or `refused` (with the reason; nothing changed).
Only one request is in flight at a time across all mods. With `scene@2`, each
mod retains its own last accepted result even if another mod starts a request;
it is replaced by that mod's next accepted request or cleared at shutdown.
Serialize requests within the same mod when each result matters: a new
accepted request supersedes its previous one. An immediate refusal does not
replace it. A request whose scene stops being ready before it runs is refused
and discarded, never retried later.

### Passing the time slot (`calendar.pass@2`) and the hold on time (`calendar.hold@1`)

`p4.calendar.pass()` returns `true` when queued or `false, reason`. It ends the
current time slot the way an activity does. The next slot is the one the game
has staged (`datGetPendingFlag`; after every calendar step the game stages the
next one itself); when nothing is staged it is the one `calendarStepTimeSlot`
(`0x00120F70`) would stage (slot 5 rolls over to the next day, days without a
calendar entry step by one, others step 0, 3, 5), and the request stages it
through the game's own setters (`datSetPendingDateIndex/TimeSlot/Flag`, as the
script command at `0x0025C560` does). Refusal reasons: the scene is not a
ready fixed field (the warp gate and its reasons), `a camera lease is active`,
`another scene request is in flight`, `the next time slot is tomorrow` (never
across midnight), a hold (below; with `calendar.pass@2`) and, at the
frame boundary, `the calendar changed before the time could pass` when the
staged slot no longer matches the request, or the hold again, checked in
the same boundary step that stages the slot, so no game script runs between
the check and the pass. Once the
scene resources are prepared it requests sequence 25, CALENDAR MAIN, as the
script command at `0x00176320` does; `calendarStepTimeSlot` applies the slot and
starts that period's scheduler procedure, which picks the next scene. The
request shares the scene queue and `p4.scene.status()`. A `done` detail that
starts with `passed` means sequence 25 ran and the date and slot read back
equal the expected ones: `passed` when the game is back in a ready field, or
`passed; an event followed`, `passed; a dungeon floor followed` or `passed; a
battle followed` when the next scene is not a field. Other `done` details:
`the calendar sequence did not start` (600 frames) or `did not finish` (5400
frames), `the game did not return to a ready field` (5400 frames with no
sequence) or the slot read back. Frames are counted only while the calendar
runs or nothing is happening, so field events and messages after the pass are
waited out. The call logs the slot it expects and records semantic use, like a
warp.

`p4.calendar.hold()` returns `nil`, or why the time is held. Retail has no
single free-time gate; every activity that spends time checks its own
conditions. The hold is what they share. First the story hold, the script
flags on which the optional time-spending activities of the free fields refuse
(Aiya, the shrine, the bus stop and the capsule machines of the shopping
district, the Junes elevator, the library, the desk, sofa and bed at home,
riverbank fishing; `BIT_CHK`, which is `datGetFlag`). Only scheduler procedures
set and clear them. Reasons: `the 12/05 investigation holds the day` (flag
`0x60F`, set by `sdl12_05_AM_A` and cleared by `sdl12_05_PM_D`; "You have to
gather information") and `the last day holds the time` (flag `0x602`,
`sdl03_20_PM_B` to `sdl03_21_AM_A`; only the farewells remain). Then the slot's
owner: `the schedule still runs this time of day` while the `sdlScheduler`
task that the calendar step starts (`cldSchedulerStart`, `0x00260450`) is
still running the slot's `sdlMM_DD_<slot>` procedure. A procedure that ends
without handing the player a scene makes that task request the calendar
sequence itself (`cldSchedulerTaskUpdate`, `0x00260020`), so the slot is not
the player's until the task is gone (it is freed two frames after its
procedure ends). An activity's own availability (weather, weekday, prices,
Social Link state) is not part of the hold.

### Encounter launcher

`p4.encounter.count()` is the number of encounter records in the loaded table
(including expanded banks); `p4.encounter.info(id)` returns
`{id, enemies = {five enemy ids}, formation, variant}` or nil for a record
that is not a valid loaded encounter. `p4.encounter.launch(id)` requires a
ready fixed field whose entry the service observed, a valid record and no other
request. At the frame boundary it asks the `encounter` authorizers, then
starts the battle through the shared form of `btlScriptStartEncounter`
(0x1932f0, `src/platform/battle_sim/entry.c`): the hero and the current party,
the record's formation and sequence 6. When the battle sequence finishes and
the game has not requested another scene, the service returns to the observed
origin field through the same field request as a warp. After a defeat (outcome
2) the knocked-out party members are revived to one HP with status bit 0x80000
cleared first, the recovery `btlScriptResolvePartyDeath` (0x193450) applies to
allies after a scripted battle; there is no game over. Rewards and experience
are the battle's own. Launches from generated floors are refused because a
floor cannot be returned to.

### Camera lease

```luau
local lease, why, owner = p4.camera.acquire("free camera")
if lease then
    lease:input(true)
    lease:set({y = 300, pitch = -30, fov = 60})
end
```

One mod at a time holds the camera; a second `acquire` returns nil, the reason
and the holder's lease name. Acquiring requires a ready fixed field. The lease
is a **render-only** view override: in RenderWare's `CameraBeginUpdate`, before
frame synchronization (the widescreen seam in `src/render/p4gl_camera.c`), the
main perspective camera's frame matrix and view window are replaced, and both
are restored byte for byte at `CameraEndUpdate`. The game's own field camera
and the player's camera-relative movement read the unchanged values between
frames; engine code that reads that camera between `CameraBeginUpdate` and
`CameraEndUpdate` (culling, billboards, anything drawn in that scope) sees the
leased pose. The pose is world `x/y/z`, `yaw` and `pitch` in degrees (pitch
from -89 to 89) and vertical `fov` in degrees (10..120); `get()` returns the
current pose, starting from the game's view at the first drawn frame, and
`set()` changes any subset of it.
Out-of-range values return false. `input(true)` gives the mod the pad: the game
then receives a neutral packet after the mod's own input sample, and after
`input(false)` or release the neutral packet continues until every button is
up, so no held button reaches the game as a new press. The lease ends at
`release()`, a field change, an event, a menu, a `give.skill` choice panel
about to open (`docs/modding/systems/actions.md`; `acquire` is refused until
it has) or the mod's shutdown; `active()` then returns false and why.

### HUD groups

`p4.hud.hide(group, hidden)` hides named gameplay HUD groups **render-only**:
at the ordering-table flush the renderer drops the GL draws inside the group's
buckets or nodes, while every node and its callback still run, so animation,
input, glyph and sprite bookkeeping and game logic are unchanged
(`p4gl_hud_hidden_set`, `src/render/p4gl_im2d.c`).
Groups (`p4.hud.groups()`): `field_banner` (layer 0x4a), `field_map`
(layers 0x4b..0x53), and the edge-anchored node callbacks `calendar`
(0x122a40), `party` (0x2167f0) and `battle_commands` (0x20b1a0..0x20b5b0), the
same producers the widescreen layout anchors. Fades, loading indicators, Config
and Camp, message windows and status rows are not groups and are never hidden.
Hides are per mod and per group, require a field, floor or battle and are
released at the next scene change, load or the mod's shutdown; a mod that
keeps a HUD hidden reapplies it on `field_enter`/`battle_start`.

### Practice speed (`pace@1`)

`p4.pace.set(rate)` asks for 0.25, 0.5, 1, 2 or 4 times normal speed and
returns `true` or `false` and a reason; `p4.pace.state()` returns the requested
rate, the rate in force and, when they differ, why. One mod at a time owns the
speed; `set(1)`, the mod's shutdown or recovery and a return to the title
release it. It changes how fast wall time passes for the game's own scheduler
clock (`src/platform/kernel_thread.c`, `include/p4pc/pace.h`): every VBLANK,
alarm and timed wait keeps its spacing in game time, so each frame runs the
same updates, random draws and counters as at normal speed and only the host
waits less or more between frames. The new rate is taken at a VBLANK from the
current clock, so time never jumps. A run that was never paced uses the host
clock exactly as before.

The requested rate runs only in gameplay: a ready field or floor, or a
battle. Camp and Config menus, scene changes and movies run at 1x, and the
requested rate returns at the next ready field or battle. Above 1x only every
second or fourth frame is drawn and presented, so the display keeps its own
rate. At any rate other than 1x the audio output is muted and the mixer
renders each VBLANK period's samples from the kernel (as on a virtual clock)
instead of the device's real-time callback, so ADX voices, streams and music
sequences advance with game frames and their ends fall on the same frames as
at 1x; at 1x the device plays again. Movies keep their own audio clock, which
is why they run at 1x.

Pacing changes wall time only, so it records no semantic use and never tags a
save, but like other requests it is refused in synchronous UI callbacks and
save snapshots. A virtual-clock, turbo or lockstep run (`P4PC_VIRTUAL_CLOCK`,
`P4PC_TURBO`, the battle simulator) keeps its own time and refuses it.

## Final-damage modifiers

`p4.damage.on{kind, priority, fn}` (capability `stage.damage@1`) registers a
modifier in the native final damage stage
([tables.md "Final damage stage"](tables.md#final-damage-stage), the end of
`datCalcSkillAmount` 0x00235520) and returns an enable/disable/remove
subscription that counts toward the mod's 128 like a hook:

```luau
p4.damage.on({kind = "multiply", priority = 10, fn = function(hit)
    if hit.target_side == "enemy" and hit.hit_kind == "weak" then
        return 1.2
    end
    return nil
end})
```

`kind` is `"multiply"` (fn returns a factor) or `"add"` (an addend); `priority`
is an optional integer, default 0. fn receives one eligible hit as a readonly
table and returns a number, or nil to leave the hit alone:

| Field | Meaning |
| --- | --- |
| `skill` | skill ID |
| `source_side`, `source_id` | `"party"` or `"enemy"`, and the attacker's DatUnit ID |
| `target_side`, `target_id` | the unit taking the damage |
| `source_status`, `target_status` | each unit's DatUnit ailment word for this hit, as the retail equation reads it: committed before this action, never including the ailments the action itself inflicts |
| `origin_side`, `origin_id` | the unit the hit was aimed at; differs from the target only when `reflected` |
| `hit_kind` | `"normal"`, `"critical"`, `"weak"` or `"other"` |
| `element` | the resolved element leaf, or nil |
| `reflected` | a repel delivered the hit back to its source |
| `resolved` | true while the game resolves the hit; false for an estimate (AI target choice) |
| `stock` | the retail equation's value before any modifier |
| `origin` (`stage.damage@2`) | the command the game resolves: `"skill"`, `"item"` or `"attack"` (Attack and its weapon follow-ups); `"unknown"` only for an estimate outside the resolution of an action |
| `source_persona`, `source_persona_level` (`stage.damage@2`) | a party attacker's active persona ID and that persona's level, for a skill or an unknown origin, so an estimate and the resolved hit agree; nil for an enemy, an item or an attack |

The stage orders every mod's modifiers by priority, then multiply before add,
then the mods' load order, then registration order, and converts the composed
value once with the retail truncation and clamp. fn runs synchronously like a
save snapshot: no native calls, no guest or settings mutation and no `p4.log`
(record what you want to report in your own Lua state and log it from a later
callback, such as `frame_start`), the ordinary callback budget. It must be a
pure function of the hit (no RNG, the same answer for an estimate and the
resolved hit). An identity value (factor 1, addend 0)
is the same as nil; any other value records semantic use of the mod. An error,
a non-number result or an exhausted budget never applies a value: the mod
enters native recovery. Registration is a start-up declaration: `p4.damage.on`
inside any callback is refused. A disabled modifier keeps its place and is not
called; `remove` (refused while a hit is composed), VM teardown and session end
unregister it. With no modifier registered the game's damage is unchanged.

## Recovery, skill cost and chance modifiers

`p4.recovery.on` (capability `stage.recovery@1`), `p4.cost.on` (`stage.cost@1`)
and `p4.chance.on` (`stage.chance@1`) take the same
`{kind, priority, fn}` specification as `p4.damage.on` and follow every rule
above: start-up registration, snapshot-mode fn, the same order and identity
rule, enable/disable/remove handles counted like hooks, native recovery on an
error. Each registers in its own native stage ([tables.md "Final damage
stage"](tables.md#final-damage-stage) and the sections after it), and none runs
while it has no modifier.

```luau
p4.cost.on({kind = "multiply", priority = 20, fn = function(cost)
    if cost.persona == JACK_FROST and cost.resource == "sp" then return 0.81 end
    return nil
end})
```

`p4.recovery.on` sees each formula HP heal (the Dia and Media lines) after
Divine Grace and the variance. The result is truncated once and clamped to
1..32550:

| Field | Meaning |
| --- | --- |
| `skill`, `source_side`, `source_id`, `target_side`, `target_id` | as for damage |
| `resolved`, `origin` | as for damage |
| `stock` | the heal before any modifier |
| `source_persona`, `source_persona_level` | as for damage |

`p4.cost.on` sees every nonzero skill cost wherever the game computes it: the
battle and Camp skill lists, availability, the deduction, AI and the Persona
cost rows. The result is rounded half up once and clamped to 1..65535, so a
nonzero cost stays nonzero:

| Field | Meaning |
| --- | --- |
| `skill` | skill ID |
| `side`, `unit` | the paying unit's side and DatUnit ID |
| `resource` | `"hp"` or `"sp"`, the skill row's cost kind |
| `stock` | the cost after Spell Master/Arms Master and the minimum of 1 |
| `persona`, `persona_level` | the persona the cost is priced for: the unit's active persona, or the persona a Persona or Camp cost row shows, which may be a stocked one; nil for an enemy |

`p4.chance.on` sees each rolled ailment or instant-KO attempt: one call per
target, since the game rolls each target of an all-target skill separately.
The result is truncated once, then the game's own clamp to 0..99 and its single
random roll follow. Guard, immunity and null exits, guaranteed rates (100 or
more) and the scripted unconditional KO never reach it:

| Field | Meaning |
| --- | --- |
| `skill`, `source_side`, `source_id`, `target_side`, `target_id` | as for damage |
| `component` | `"ailment"`, or `"instant_ko"` for the KO mask 0x80000 |
| `statuses` | the attempted status mask |
| `item` | an item action |
| `stock` | the game's truncated chance before its clamp |
| `afflictions` | ailments that landed on the target earlier this battle (capability `status.afflictions@1`) |
| `source_persona`, `source_persona_level` | a party attacker's active persona and level; nil for an enemy or an item |

Rage, fear, silence, confusion, poison, exhaustion and enervation count as
afflictions; Dizzy and KO do not. SMT V-style decay, where each prior
affliction in the fight makes the next one less likely:

```luau
p4.chance.on({kind = "multiply", fn = function(hit)
    if hit.component ~= "ailment" or hit.target_side ~= "enemy" then return nil end
    return math.max(0.1, 1 - 0.3 * hit.afflictions) -- 100% -> 70% -> 40% -> 10%
end})
```

## Ailment wear-off

> Requires capability `status.wearoff@1`.

`p4.status.wearoff.on` takes the same `{kind, priority, fn}` specification and
changes the chance that a unit shakes off an ailment on its own turn, for party
members and enemies alike. Confusion, fear, rage and exhaustion wear off
without a roll on a unit's third turn with them, enervation and silence on its
sixth (Fast Heal halves that, Insta-Heal makes it one); each turn in between
rolls. The result is truncated and clamped to 0..100 (0 never cures on that
roll, 100 always does); the game's own random roll then follows as usual:

| Field | Meaning |
| --- | --- |
| `side`, `unit` | the unit whose turn it is and its DatUnit ID |
| `status` | `"confusion"`, `"fear"`, `"rage"`, `"exhaustion"`, `"enervation"` or `"silence"` |
| `level`, `luck` | the unit's level and Luck |
| `turns` | the unit's own turns with the ailment so far (rolls start at 2) |
| `limit` | the turn that cures it without a roll |
| `stock` | the game's chance in percent |
| `afflictions` | ailments that landed on the unit this battle, this one included (capability `status.afflictions@1`) |

```luau
p4.status.wearoff.on({kind = "add", fn = function(roll)
    return 15 * (roll.turns - 1) -- every unit shakes ailments off faster each turn
end})
```

## Buff multipliers

> Requires capability `buff@1`.

`p4.buff.on` takes the same `{kind, priority, fn}` specification and changes
the multiplier a buff rank, Charge or Concentrate gives one stat, for party
members and enemies alike. The game asks for the attacker's attack or magic
and the target's defense when it works out damage, and the attacker's
accuracy and the target's evasion when it works out a hit. The result is
clamped to 0..100:

| Field | Meaning |
| --- | --- |
| `side`, `unit` | the unit and its DatUnit ID |
| `stat` | `"attack"`, `"magic"`, `"defense"` (damage taken), `"accuracy"` or `"evasion"` |
| `rank` | the buff rank: -1, 0 or 1 |
| `charged` | this attack's Charge (attack) or Concentrate (magic) is up |
| `stock` | the game's multiplier: the rank table, x2.5 Charge, Lead boost |

```luau
p4.buff.on({kind = "multiply", fn = function(query)
    if query.charged then return 1.8 / 2.5 end -- Charge and Concentrate give x1.8
    return nil
end})
```

## Battle rewards

> Requires capability `reward@1`.

`p4.reward.on` takes the same `{kind, priority, fn}` specification and changes
the EXP each recipient and the money the team earns from a won battle. The
result is truncated and clamped to 0..999999; 0 pays nothing:

| Field | Meaning |
| --- | --- |
| `reward` | `"exp"` or `"money"` |
| `recipient` | for EXP: `"hero"`, `"party"` (a party member) or `"persona"` (one of the protagonist's Personas); nil for money |
| `id` | the hero's 1, the party member's character ID or the Persona's ID; nil for money |
| `level` | the recipient's level; nil for money |
| `equipped` | for a Persona: true for the one the protagonist carries, false for a stock Persona earning through Growth |
| `stock` | the game's award (EXP after the level-difference scale, Growth, Lead boost and EXP accessories; money after its flags) |

Stock Personas without a Growth skill are asked with stock 0, so a mod can give
them EXP. Money is asked every frame the results screen shows it and once more
for the payment; answer the same every time.

```luau
p4.reward.on({kind = "multiply", fn = function(query)
    if query.reward == "exp" and query.level >= 30 then return 0 end -- level cap 30
    return nil
end})
```

## Unit values

> Requires capability `unit@1`.

`p4.unit.on` takes the same `{kind, priority, fn}` specification and changes a
unit's max HP and SP (an enemy's starting HP and SP with them) and the five
stats battle formulas read, for party members and enemies alike. The result is
truncated and clamped: max HP and SP up to 999 for the party and 65535 for an
enemy, stats to 1..99:

| Field | Meaning |
| --- | --- |
| `side`, `unit` | the unit and its DatUnit ID |
| `stat` | `"max_hp"`, `"max_sp"`, `"strength"`, `"magic"`, `"endurance"`, `"agility"` or `"luck"` |
| `level` | the unit's level |
| `stock` | the game's value (after HP/SP skills, accessories and the party cap; a stat before Enervation halves it) |

Max HP and SP are read across the game, outside battle too, so keep fn cheap.

```luau
p4.unit.on({kind = "multiply", fn = function(query)
    if query.side == "enemy" and query.stat == "max_hp" then return 1.5 end
    return nil
end})
```

## Accuracy and critical hits

> Requires capability `hit@1`.

`p4.hit.on` takes the same `{kind, priority, fn}` specification and changes the
chance that an attack hits and the chance that it lands a critical, for party
members and enemies alike. The result is truncated and clamped to 0..100 (0
never succeeds, 100 always does); the game's own random roll follows:

| Field | Meaning |
| --- | --- |
| `roll` | `"accuracy"` (after the game's 50..99 clamp) or `"critical"` |
| `skill`, `source_side`, `source_id`, `target_side`, `target_id` | as for damage |
| `stock` | the game's chance in percent |

Automatic hits and guaranteed criticals never ask.

```luau
p4.hit.on({kind = "add", fn = function(query)
    if query.roll == "critical" and query.source_side == "party" then return 10 end -- +10% party criticals
    return nil
end})
```

[`tests/mod_fixtures/formula_stages`](../../tests/mod_fixtures/formula_stages/main.luau)
is a complete small difficulty mod built only from these stages (unit, hit,
buff, chance with afflictions, wear-off and reward), with the `requires=` list
its `mod.ini` declares; `mod_api_check` type-checks it.

## Passive grants

> Requires capability `passive.grant@1`.

`p4.passive` gives a passive skill to a party member, a Persona, an equipment
item or an enemy kind without using a skill slot, and takes it back. The skill
is an ID or a `p4.data` skill row and must be a passive of the loaded tables;
a target is one of:

| Target | Reaches |
| --- | --- |
| `{party = "yosuke"}` or `{party = 2}` | that member (1 protagonist .. 8 teddie), whatever Persona they have |
| `{persona = row or ID}` | whoever has that Persona now; in the protagonist's stock, that Persona's own checks (Growth, the Camp skill list's duplicate choice) |
| `{item = row or ID}` | whichever party member has the item equipped, in any slot |
| `{enemy = row or ID}` | every enemy of that kind |

The protagonist's own grants and his equipment's also reach his stocked
Personas' checks. A grant counts wherever the game asks whether a unit has
that passive, exactly as the skill in a slot would: damage, healing, hit,
critical, ailment and affinity formulas, counters, Endure and Light/Dark
survival, automatic buffs at battle start, regeneration, ailment recovery,
victory recovery, max HP and SP, HP and SP cost halving, the enemy AI's avoidance
of Counter, Growth, the field encounter check (0x205), and table effect rows and damage
cover keyed to the skill. A unit that already has the skill, or whose skill two
mods grant, has it once. The skill list itself is unchanged: skill menus,
Analyze, the AI's command lists, fusion and learning see the eight slots only.
Each check happens when the game makes it, so a grant made during a battle
reaches the next check, and a battle-start check (automatic buffs) the next
battle.

`grant` and `revoke` are mutations (refused in snapshot callbacks such as
stage modifiers) and return whether this mod's own grant changed; another mod's
grant of the same skill stays. Once the catalog is loaded, `grant` takes only a
Persona, item or enemy that is a row of it (`revoke` and `granted` take any
in-range target, so a grant can always be taken back). `granted` asks whether
any mod grants exactly that target the skill. `list` returns every held grant,
ordered by target kind (party, persona, item, enemy), target, skill and mod load
order, each with `kind` and numeric `id` beside its `target` and the holding
mod's `p4.id` as `owner`: filter on `owner == p4.id` to see only this mod's.
`clear` takes back every grant of this mod and none of another's.

Every mod together holds at most 256 grants; `grant` of a new one beyond that
raises "the grant table is full" and changes nothing, so a mod that grants in
response to the player catches it with `pcall` and shows the refusal.

Grants are not saved. They outlive a load and a New Game and end only when
revoked, cleared, or with the mod's script. A grant that belongs to one save
(a reward, a story trait) must therefore be dropped when another save is loaded
or a New Game starts: do it in the `load` callback of `p4.save.register`, which
runs for both (`origin` `"load"` or `"reset"`), and grant again from the loaded
value. Grants that hold for every save (a Persona trait, an item's effect) can
simply be made at start-up.

```luau
local counter = p4.data.skill("Counter")
p4.passive.grant({persona = p4.data.persona("Izanagi")}, p4.data.skill("Arms Master")) -- every save
p4.passive.grant({enemy = p4.data.enemy("Cowardly Maya")}, counter)

local trait = false -- earned in this save
p4.save.register({
    schema = 1, max_bytes = 16,
    snapshot = function() return trait end,
    load = function(value, origin)
        p4.passive.revoke({party = "kanji"}, counter) -- the previous save's
        trait = value == true
        if trait then p4.passive.grant({party = "kanji"}, counter) end
    end,
})
```

Navigator skills (the support Persona's escape, healing and analysis) are not
passives and cannot be granted. Which native checks read grants, and the
timing of each, is listed in [tables.md "Passive grants"](tables.md#passive-grants).
The bundled Practice tools mod's P18 PASSIVE SKILLS
([`mods/p4pc.practice`](../../mods/p4pc.practice/main.luau)) grants and takes
back a party member's passives from its MODS page.

## Session bank overlays (`tables.overlay@1`)

`p4.tables` lets a mod give the current run its own copy of a loaded battle
bank: the rows of a `BTL.CVM` file block, keyed by the file label and block of
the game's own loader descriptor (`PERSONA1` is block 1 of
`battle/PERSONA.TBL`). The table service owns the banks and their exact
extents ([tables.md "Session bank overlays"](tables.md#session-bank-overlays)).

```luau
local stock = p4.tables.stock("PERSONA1")       -- the bytes as the game loaded them
-- ... generate a run's rows from stock ...
local ok, why = p4.tables.overlay({PERSONA1 = rows, UNIT2 = affinities})
```

`banks()` lists every bank (`key`, `file`, `block`, `bytes` once loaded,
`overlayable`, and the `owner` mod of an overlay); `bank(key)` returns one.
`stock(key)` returns a copy of the bank as loaded, never an overlay's bytes.
`overlay(layer)` replaces this mod's whole layer, all or none: every buffer must
have exactly its bank's loaded size, banks this mod overlaid before but left out
get their stock bytes back, and on any refusal (`false, reason`) no byte
changes. Only `PERSONA0`, `PERSONA1`, `UNIT0`, `UNIT1`, `UNIT2`, `ENCOUNT0`
to `ENCOUNT4` and the static fusion charts `FUSION0` to `FUSION3` can be
overlaid. The charts are the arcana result tables the game's pair, alternate
pair, triangle and alternate triangle fusions read (cells (1,1) to (22,22) of
each 24-byte-row chart, 526 bytes); every byte must be an arcana 1..22 or 0 for
"no fusion". A chart overlay composes with `fusion` authorizers: an authorizer
may still deny a fusion kind whose results the overlaid chart decides. With
`tables.overlay@2`, `ITEMTBL`, the retail item table (1536 rows of 68 bytes, the layout
`init/itemtbl.bin` loads), can be overlaid too, but only in each row's two
unlock-gate words at `+0x3c` and `+0x40` (the material and count the shop lists
and Daidara's unlocks test); every other byte must equal stock, and it exists
only while the game uses the retail table (no item catalog is active). A bank
overlaid by another mod is refused and named. Overlays are refused while a package table source owns the battle banks,
during a battle, and from initialization, hooked functions, save snapshots and
synchronous UI callbacks: call it from an event, a `frame_start` or the save
`load` callback. `clear()` restores this mod's banks (refused the same way,
including during a battle); `active()` names them.

An overlay lasts for the run. Every load and every New Game reset ends it: the
save commit restores every overlaid bank before any mod's `load` callback runs,
so a mod re-applies its run's layer from `load` (its own save block). If the
game re-copies or relocates an overlaid bank, the new bytes become its stock
and the layer is applied again, or dropped (and logged) when the bank's size
changed. VM teardown, which happens only at process shutdown, restores the
mod's banks immediately. The live banks are what every
game reader uses; an overlay records semantic use of the mod.

### Derived table source files (`tables.source@1`)

`p4.tables.source(producer, name)` returns a copy (a `buffer`) of a raw file a
registered derived table producer recorded while composing the active table
catalog ([tables.md "Derived table sources"](tables.md#derived-table-sources)),
or nil when that producer is not selected, did not record that name, was
refused, or the catalog is not active yet. It never returns live guest bytes.
Copies are at most 4 MiB. It is a query: legal in every callback, with no
semantic use. The package's own script parses the bytes as a bounded buffer
codec; for example `p4pc.golden` reads `p4pc:golden`'s `init/cmpTable.bin`
from the player's Golden install for its bike-ride skills.
## Semantic events

`p4.events.on(name, callback)` returns an enable/disable/remove subscription.
Payloads contain copied scalars; guest addresses do not retain native objects.
Routes use typed native hooks and remain inactive without subscribers. A missing
required native seam refuses activation rather than fabricating an event.
For battle semantic events, a failed registration leaves no listener or event
interest and does not consume the lifetime subscription limit; the error is
propagated to the caller.
The pinned Analysis checker resolves literal event-name overloads; branch on a
union of names before registering so each callback receives its specific payload.

| Name | Native seam | Payload |
| --- | --- | --- |
| `frame_start` | completed scheduler VSync/device boundary, before guest updates resume | frame |
| `field_leave` | ready field becomes pending/not-ready or root destruction: 0x155360 before/after, 0x155bf0 before | task, field, room, date, period |
| `field_enter` | 0x155360 returns ready state 12..14, pending clear, requested ids match; once per visit | same |
| `battle_start` | 0x192e90 returns initialized task | task, setup, battle |
| `battle_end` | 0x192cd0 before release | battle, outcome (0 unset/cancelled, 1 victory, 2 defeat, 3 no party) |
| `menu_open/close` | native Camp/Config root creation/destruction: 0x12d000/0x12cc50, 0x35c690/0x35e820 | task, kind (0 Camp, 1 Config) |
| `calendar_advance` | completed date/time setters 0x106100/0x1062f0, actual changes only | old_date, old_period, date, period |
| `save` | 0x10d7c0 returns finalized checksummed image | version, data, size |
| `load` | 0x10e710 finishes applying accepted sections, before field handover | version, data, size |
| `exit` (API 1.3) | window-close request, delivered at the next `frame_start` boundary; the process then exits | none |

The save event does not prove disk publication. Post-image notifications do not
authorize save provenance or pre-commit restrictions such as Ironman; use the
save provider's `authorize_save`, `authorize_load` and `published` callbacks.

`exit` is best effort. Without a subscriber the close request exits at once, as
before. With one, the quit waits for the game thread's next frame boundary,
runs every `exit` callback (an error there is logged; the quit still happens),
then exits normally. A second close request while one is pending exits at once.
It never fires for crashes, nonzero exits, the native mod-recovery Quit/restart,
automation exits or an operating system killing the process (for example an
Android background kill), so treat a missing `exit` as an unclean end.

## Save provenance

Compilation, subscription/page registration and writes to mod-owned scratch do
not tag saves. Actual hook/event/native UI callback execution, permitted writes
to game-owned guest memory and actual native calls record the VM's pinned,
selected provider kind/id/version. This ordinary semantic-use history is
process-sticky across load/New Game; installed or retired metadata is not source
authority. The generic file-use consumer uses the one actual VFS dispatcher;
CPU preparation and validated cosmetic-only reads do not become semantic uses.

Normal player modded saves do not acquire MOD_WITNESS custody. With those mods
later disabled, the native loader lists missing versions and optional blocks
and permits explicit continuation when safe; required bindings remain strict.
Only actual witness custody refuses playback outside the separate witness lane.

Each mod may register one `p4.save.register{schema, max_bytes, snapshot, load}`
provider. `schema` is a positive 32-bit version and `max_bytes` is 9 through
1,048,576, including the value encoding. `snapshot()` returns nil to omit the
block, or a plain number, string, boolean or nested table with boolean, number
or string keys. Tables must not have metatables or cycles; functions, userdata
and handles are not serializable. Snapshots run without native calls or guest
mutation, and encoding or budget errors refuse the save before publication.

An emitted block requires the exact selected provider/version. Loading stages
and validates all blocks before publication; the commit is an infallible native
swap, with no Lua execution. After commit, `load(value, origin)` receives the
decoded value, or nil when no block is present, including a vanilla save or a
reset. `origin` is `"load"` for a real load and `"reset"` when the game resets
its state to the New Game defaults. Retail does that at boot, when LOAD GAME
starts restoring a save (a `"load"` follows), and before New Game, so a reset
is not by itself the player starting a New Game. Use `snapshot` to read mod-owned Lua state and `load` to rebuild any fresh
scratch. A post-commit callback failure enters normal mod failure recovery; it
does not retroactively undo the accepted game state.

Three optional provider fields (API 1.3) authorize and confirm saves:

- `authorize_load(value)` runs for a real load (never a reset) after every
  block decoded and before any commit, with this mod's staged value or nil.
  Returning `false, reason` refuses the whole load through the native
  `Cannot load game` error; the slot and the running game are untouched.
- `authorize_save()` runs when the game submits a full save image. Every mod's
  `authorize_save` runs before any provider snapshots, so the answer cannot
  depend on load order. `false, reason` refuses the save through the native
  `Cannot save game` error before any byte of the slot is written.
- `published(value)` runs at the next frame boundary after a save that carried
  this mod's block has been durably committed (data, icons and the `.p4mods`
  sidecar), with the value that was saved. It never runs for a refused, failed
  or cancelled save. Native calls and `p4.settings` writes are allowed here.

Both authorizers run like `snapshot`: no native calls, no guest mutation, the
same budget. A Lua error or an exhausted budget refuses the operation; it never
authorizes. Keep the reason short plain text; it is shown to the player.

## Native lifetime and shutdown

Loading has two passes. **Before any mod code runs**, the shared admission
transaction checks manifests, API versions, native capabilities/conflicts and
compiles/loads every enabled source into a retained function. It does not run
in-game Analysis: typechecking belongs to the modder CLI/luau-lsp, and runtime
type errors follow the runtime path below. Refusal displays a clear native
message, quarantines that mod's raw files and typed resources, and continues
with valid siblings. Native prepares roll back before a filtered generation is
published; surviving compiled chunks are retained, not compiled again.

The complete CPU decode/validation of mandatory resources/table sources also
precedes raw exposure. Raw replacements publish early; only genuine runtime
GPU/guest-bank/UI binding remains at the real0010d480 completion boundary.
The first mod chunk executes only AFTER that late transaction publishes. A
late failure before Lua still uses post-exposure recovery, not early rollback.


**After exposure**, init/callback errors and budget failures cannot invent a
game return value or pretend queued work was rolled back. The native recovery
owner shows `Mod X failed: … The game must restart`, with `Restart without X`
and `Quit`. One atomic INI write removes X from `mods.enabled` and records
`mods.recovery_mod`/`mods.recovery_choice`, preserving other keys/comments.
Unattributed failures never guess a package/native owner: **Restart with all mods
disabled / Quit** explicitly offers safe mode. Safe mode atomically clears BOTH
mod and pack selectors and records its cause/next-boot notice; Quit leaves those
selections unchanged. Two unclean pre-stable startups offer the same recovery.
An actual other settings-directory owner offers explicit instance-only mods-off
continuation without changing stored selectors/history.
An unwritable INI refuses automatic restart and explains manual repair; a
failed platform restart displays manual-relaunch instructions before controlled
exit. Successful restart goes through `p4_os_request_restart` and ordinary
drained exit, never same-process SDL/game/VM reentry.

A real native task/queue quiescence boundary may own safe teardown. The current
restart-required path keeps VM/provider pins until process shutdown when that
proof is unavailable; no automatic hot-unload/exit cleanup invents one.
The separate `P4PC_MOD_WITNESS=1` lane never reads physical modal input:
`P4PC_MOD_RECOVERY_CHOICE=restart|quit` supplies its scripted choice (default
Quit), and labelled error/message receipts replace interactive acknowledgement.
These noninteractive policy witnesses are not native-dialog rendering receipts.

The native hook/event idle queries describe their actual callback stacks only.
Bulk owner removal and aggregate clear fail closed during dispatch; they never
silently return while a caller frees callback userdata. VM teardown also refuses
active native-context invocation frames. It first tombstones an owner's
callbacks, then destroys its VM while those handles remain valid for userdata
finalizers. Aggregate event/hook clear comes **after all VMs/handle clients**,
followed by memory-policy teardown. Package retirement comes after consumers'
real quiescence/pin release, not after a mask/count heuristic.

Native death witnesses invoke aggregate clear and owner removal from actual
hook/event callbacks and require the explicit refusal. These are unit-witness
definitions, not passing platform/gameplay receipts.

`P4PC_MOD_WITNESS_TESTS=ON` explicitly registers the separate
`mod_loader_recovery[64|64w]` CTests (label `mod-witness`); the option is OFF for
default retail gates. Enable the actual native link profile for every selected
ABI and run this label only through an admitted, isolated mod-test lane.
The driver uses real provider reads, retained compiled functions, the real VM,
native recovery backend, atomic INI writes and the OS fresh-image restart API,
with authored synthetic mods. It checks compile/version/capability/duplicate
refusals before any script-side subscription exists, unchanged survivor source
read counts, removal of the bad raw VFS layer, init/runtime-type/budget errors,
event/hook callback errors, both persisted choices and a fresh entry with the
bad mod absent. Its scripted acknowledgement is explicitly labelled non-retail;
it does not qualify physical dialog rendering or game task/queue unload safety.

## Types and qualification

[`include/p4pc/p4.d.luau`](../../include/p4pc/p4.d.luau) is the shipped public type
surface. The pinned Analysis CLI loads it after builtin definitions and checks
both accepted API usage and rejected unavailable globals. Script `--!nocheck`
can opt out; static checking is developer assistance, not sandbox enforcement.

The VM witness uses real shared manifests/provider pins, the interpreter and the
cooperative scheduler, with explicit synthetic retail descriptors and explicit
unavailable-image/heap inputs. Unregistered raw metadata cannot create a mod VM.
It is not a retail gameplay witness. Required integration qualification separately
covers no-mod state/RNG/walk identity on i386/x64 and unhooked frame overhead on
`story_to_0415`, bathhouse and haze: paired turbo wall time and paced frame CPU
must remain within measured run-to-run noise. The inactive byte/branch cost and
hooked-call dispatch budget are explicit in [budgets](budgets.md#hook-cost-acceptance),
not described as zero cost or an unobserved timing result.
Source implementation or a configured target is not a passing platform receipt.

The real no-loaded-mod identity driver is `mod_identity.py`, a private study tool that is not
part of this repository. It builds
separately from game launches and owns actual admission per build/native case:
Windows uses CPU/native `Machine` cases; POSIX runs on the HOST through
`--container-command` with the accepted `p4pc-run` and identical absolute bind
paths, one container invocation per build or game. See the
[admission example](budgets.md#real-frame-windows-and-paired-guard-study).
There is no claimed `P4PC_SCOPE=mods-no-load` route or acceptance from source alone.
It builds entry guards and Luau together OFF/ON on **both i386 and x64**,
with fixed zero initialization and both selectors forced off. The shared
package/VFS plumbing remains in both builds, as do the mask-tested authorizer
and formation seams in overrides 001b1d70 and 002ecfc0, which take the retail
path at mask 0. The port64 census baseline describes the guarded mirror, so only
the compiled-in side is checked against it. It runs the existing real
`logic64_init.py` checkpoint/walk cohort with private seed-card copies, not
synthetic game state.

Its report binds each actual game binary and CMake cache by SHA-256. Per case it
compares every emitted native lockstep state-word/RNG/walk stream byte, the
independent walk digests, all 624 MT state words and cursor, and the existing
pointer-free endpoint semantics. A missing/empty native stream or a failed
native walk fails the check. It retains the real logs, snapshots and stream
files under its caller-owned build root. Run it only through land/study
admission; `P4PC_MOD_WITNESS` is always refused, including with empty selectors.
The separate frame-overhead/platform qualification above remains required.
The real counter and paired `mod_guard_timing.py` study contract (private tooling) are
documented in [budgets](budgets.md#real-frame-windows-and-paired-guard-study);
their source/fixtures are not unobserved timing receipts.
