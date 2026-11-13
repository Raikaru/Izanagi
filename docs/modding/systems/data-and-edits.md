# Game data: skills, Personas, enemies, items and dungeons

`p4.data` lets a mod look up the game's skills, Personas, enemies and items by
name and read their numbers, without knowing a single table offset or ID.

> Requires capability `data.catalog@1` (`requires = data.catalog@1` in
> `mod.ini`).

```luau
local agilao = p4.data.skill("Agilao")
p4.log(agilao.name .. " has power " .. agilao.power .. " and costs " .. agilao.cost.sp .. " SP")

local maya = p4.data.enemy("Cowardly Maya")
if maya.affinities.fire == "weak" then p4.log("Burn it!") end

for _, ref in p4.data.find("skill", { element = "ice" }) do
    p4.log(ref.key .. " " .. ref.name)
end
```

Everything comes from **your own disc** at run time and from the tables the
game has actually loaded, so a Golden table mod, a rebalance or any other
installed mod's changes are what you see. Nothing from the game is shipped
with the port.

## Looking things up

| Function | Returns |
|---|---|
| `p4.data.skill(query)` | a [skill](#skills) |
| `p4.data.persona(query)` | a [Persona](#personas) |
| `p4.data.enemy(query)` | an [enemy](#enemies) |
| `p4.data.item(query)` | an [item](#items) |
| `p4.data.find(kind, filter)` | a list of refs (`{kind, id, key, name}`) |
| `p4.data.all(kind)` | every named row of a kind, in ID order |

`kind` is `"skill"`, `"persona"`, `"enemy"` or `"item"`.

A `query` is one of:

- **A display name.** Case, spaces and punctuation don't matter: `"agilao"`,
  `"Agi Lao"` and `"AGILAO"` all find Agilao. If another mod renamed the row,
  both its new name and the name on your disc still work.
- **A key**, such as `"p4:skill:agilao"` (see [Keys](#keys)).
- **An ID number.** This is the advanced form; prefer names or keys.

When a name isn't found, the error says so and suggests the nearest names:

```
p4.data.skill: no skill named "Agilaoo". Did you mean "Agilao" (p4:skill:agilao), "Agi" (p4:skill:agi)?
```

Some names are shared (several enemies are called the same thing). Asking for
one of those is an error that lists their keys, with a detail to tell them
apart, so you can pick one:

```
p4.data.enemy: 2 enemies are named "Rainy Sister 2"; use a key instead: p4:enemy:rainy-sister-2 (level 40), p4:enemy:rainy-sister-2-2 (level 56)
```

`p4.data.find(kind, filter)` never raises for "nothing found"; it returns an
empty list. The filter is either part of a name (`p4.data.find("item", "bead")`)
or a table of conditions that must all hold:

| Field | Kinds | Example |
|---|---|---|
| `name` | all | `name = "dyne"` (part of the name) |
| `kind` | skill, item | `kind = "heal"`, `kind = "weapon"` |
| `element` | skill | `element = "fire"` |
| `arcana` | persona, enemy | `arcana = "Magician"` |
| `level` | persona, enemy | `level = 20`, `level = { min = 10, max = 20 }` |

Using a field on a kind that doesn't have it (say `element` for enemies) is an
error, not an empty result.

## Keys

Every row has a key that never changes:

- `p4:<kind>:<name>`, made from the name **on your disc**: `p4:skill:agilao`,
  `p4:item:revival-bead`, `p4:skill:gods-hand`. Words are joined with `-`;
  apostrophes and periods are dropped.
- Repeated names get `-2`, `-3`, ... in ID order: `p4:skill:last-resort-2`.
- Rows the game never shows (blank, placeholder or untranslated names such
  as `???`) are `p4:<kind>:#<id>`, and so are formations and groups.
- Rows a mod adds use the mod's own key: `p4pc.fusion-arts:item:storm-blade`.

Keys don't follow renames, so a mod that renames Medicine doesn't change
`p4:item:medicine`. Store keys (in saves, settings or other mods' data), not
names or IDs.

## When you can read

`main.luau` runs after the game has loaded its tables, so you can call
`p4.data` at the top of `main.luau` and in any callback, including
synchronous UI callbacks. Reads never change the game, call game code or use
the random number generator.

The catalog is captured while the game starts, and only when at least one
enabled mod has a `main.luau` or a data edit layer. If something prevents it
(for example the disc's names can't be read), catalog lookups raise an error
saying why.

`p4.data.original_enemy_level(id)` requires `data.original-enemy-level@1`.
It returns `{id, level, source, sha256}` for an original enemy ID in the
selected disc's `battle/UNIT.TBL`, captured before mounting replacements or
applying table edits. `source` is the disc's canonical container-qualified
key; `sha256` hashes the entire packed UNIT file. This identifies the selected
disc bytes, not an independently authenticated retail pressing. An unavailable
or malformed original file, or an added-row ID, raises an error; there is no
fallback to the loaded catalog. It does not read or modify game state.

The provisional Challenge package freezes these original levels and their
source/digest in its run save and rejects older boss-cap saves lacking that
provenance. Its custom continuation still estimates the epoch from the hero's
level; the encounter map, all cap enforcement paths and campaign behavior
remain unqualified. Other legacy Challenge guest access is not migrated.

## Skills

| Field | Type | Meaning |
|---|---|---|
| `id`, `key`, `name` | | as above |
| `kind` | string | `physical`, `magic`, `almighty`, `ailment`, `heal`, `support`, `special`, `passive` or `other` |
| `element` | string? | `physical`, `fire`, `ice`, `electric`, `wind`, `almighty`, `light`, `dark`, an ailment (`poison`, ...), `heal`, `support` or `special`; nil for passives |
| `passive` | boolean | true for passive skills; the fields below are then absent |
| `power` | number | base power of its HP effect |
| `cost` | table? | `{ sp = n }` or `{ hp_percent = n }` |
| `accuracy` | number | percent |
| `critical` | number | critical chance, percent |
| `targets` | string | `one_foe`, `all_foes`, `one_ally`, `all_allies`, `self`, `one_fallen_ally`, `all_fallen_allies`, `everyone` or `other` |
| `hits` | table | `{ min = n, max = n }` |
| `ailment` | table? | for skills that inflict: `{ status = "poison", statuses = {...}, chance = 40 }` (Hama and Mudo inflict `ko`) |
| `cures` | table? | for skills that cure: the statuses cured |
| `usable` | table | `{ field = bool, battle = bool }` |

Statuses are `dizzy`, `rage`, `fear`, `silence`, `confusion`, `poison`,
`exhaustion`, `enervation`, `ko` and `down`.

## Personas

| Field | Type | Meaning |
|---|---|---|
| `arcana` | string | from your disc, e.g. `"Magician"` |
| `level` | number | base level |
| `stats` | table | `{ st, ma, en, ag, lu }` |
| `affinities` | table | element → `normal`, `weak`, `resist`, `null`, `repel` or `drain` |
| `rates` | table | element → percent: damage taken, or the ailment's chance multiplier |
| `learnset` | list | `{ level = n, skill = ref }`, levels are absolute |
| `character` | string? | for a party member's Persona: `yosuke`, `chie`, `yukiko`, `rise`, `kanji`, `naoto` or `teddie` |

`affinities` and `rates` have the eight attack elements (`physical` to
`dark`) and the eight ailments (`confusion`, `poison`, `fear`, `rage`,
`dizzy`, `exhaustion`, `enervation`, `silence`). For attacks the kind is what
the Analyze screen shows (`weak` knocks the target down) and the rate how
much damage it takes: `normal` is usually 100, `resist` 50, but a row can
carry any rate. For ailments the game only has rates: `null` means the
ailment can't land, `weak` a rate above 100 and `resist` one below.

## Enemies

| Field | Type | Meaning |
|---|---|---|
| `arcana`, `level`, `stats`, `affinities`, `rates` | | as for Personas |
| `hp`, `sp` | number | maximum HP and SP |
| `skills` | list | refs of the skills it can use |
| `exp`, `yen` | number | rewards |
| `drops` | list | `{ item = ref, chance = percent }` |
| `special_drop` | table? | `{ item = ref, flag = n, chance = percent }`, a flag-gated item checked before ordinary drops |

An enemy drops at most one item per battle. The special drop is tried first
when its nonzero game flag is set; if it succeeds, ordinary drops are skipped.
Otherwise, with retail totals each ordinary item's `chance` is its own
probability. The catalog reports the configured chances, not the current flag state.

## Items

| Field | Type | Meaning |
|---|---|---|
| `kind` | string | `weapon`, `armor`, `accessory`, `consumable`, `key`, `material` or `other` |
| `price` | number | the shop price |
| `attack`, `hit` | number? | weapons |
| `defense`, `evasion` | number? | armor |
| `skill` | ref? | the skill a consumable uses, or the passive an accessory grants |
| `stats` | table? | gear's stat bonuses `{ st, ma, en, ag, lu }` |
| `users` | list? | who can equip it: `protagonist`, `yosuke`, ... |
| `shop` | table? | gear and consumables: see [Shops](#shops) |

### Shops

The game has no list of what each shop sells. Two stores read every item
row instead (capability `data.shop@1`):

- the **weapon store** (Daidara) lists weapons, armor and accessories once
  their unlock gates pass: each gate names a material and how many of it you
  have sold to the store;
- the **item store** lists consumables from their first day on sale. Other
  stands and counters do not use these stores; their stock is not editable
  here yet.

| Field | Type | Meaning |
|---|---|---|
| `shop.store` | string | `weapon` or `item` |
| `shop.sold` | boolean | `false`: the game never lists it |
| `shop.from` | `{ month, day }`? | consumables: the first day on sale |
| `shop.unlock` | list? | gear: `{ material = ref, count = n }` for each gate |

The buy price is `price` (the item store takes 20% off on its discount days).
Selling pays a fifth of `price`; trading in old gear pays a quarter.

## Dungeons

> Requires capability `data.dungeons@1`.

A dungeon floor decides how many shadows walk around, which battles they
start and what its chests hold. `p4.data.floor` finds a floor by the name the
game shows for it (`"Yukiko's Castle 3F"`), by key or by its step index.

```luau
local floor = p4.data.floor("Yukiko's Castle 3F")
p4.log(floor.name .. ": " .. floor.spawn .. " shadows")
for n, slot in floor.encounters.normal do
    if slot.formation then p4.log(n .. ": " .. #slot.formation.enemies .. " enemies, weight " .. slot.weight) end
end
```

| Field | Type | Meaning |
|---|---|---|
| `spawn` | number | shadows when you arrive |
| `refill` | number | shadows that come back |
| `chests` | number | treasure chests placed |
| `encounters` | encounter group? | which battles its shadows start |
| `treasure` | treasure group? | what its chests hold |

An **encounter group** (`p4.data.encounters(id)`) has `rates = { normal,
special, rare }`: the chance of each kind of battle is its rate out of the
three together. `normal` lists 20 slots, `special` and `rare` 5 each; every
slot is `{ formation, weight }`, and a formation is picked by weight within
its kind. An empty slot has weight 0 and no formation. `alternate` holds a
second set of rates and slots the game uses on some calendar days. Several
floors can share one group.

A **formation** (`p4.data.formation(id)`) is one battle: `enemies` lists up
to five enemies.

A **treasure group** (`p4.data.treasure(id)`) lists 29 records
`{ item, weight, locked }`; `locked` records fill locked chests.

Formations and groups have no names; their keys are `p4:formation:#17`,
`p4:encounters:#1` and `p4:treasure:#2`.

## Changing the data

> Requires capability `data.edit@1` for `data.luau`, `data.xml@1` for
> `data/edits.xml`.

A mod changes skills, Personas, enemies and items with a **`data.luau`**
file next to its `mod.ini` (and `main.luau`, if it has one):

```luau
-- data.luau
p4.data.edit(function(d)
    local agilao = d.skill("Agilao")
    agilao.power = 100
    agilao.cost = { sp = 3 }

    local hablerie = d.enemy("Lying Hablerie")
    hablerie.hp = 300
    hablerie.affinities.fire = "weak"
    hablerie.drops[1] = { item = "Medicine", chance = 50 }

    d.persona("Tomoe").learnset:add({ level = 12, skill = "Bufula" })
    d.item("Medicine").price = 100
end)
```

Or, without any code, a **`data/edits.xml`**:

```xml
<p4-edits schema="1">
  <skill name="Agilao" power="100" sp="3"/>
  <enemy name="Lying Hablerie" hp="300" fire="weak">
    <drop slot="1" item="Medicine" chance="50"/>
  </enemy>
  <persona name="Tomoe"><learn level="12" skill="Bufula"/></persona>
  <item name="Medicine" price="100"/>
</p4-edits>
```

Rows are found exactly as `p4.data` finds them: by name, key (`key="..."` in
XML) or ID (`id="..."`).

[`tests/mod_fixtures/data_rebalance`](../../../tests/mod_fixtures/data_rebalance/data.luau) is a
complete example: its `data.luau` makes these edits plus a rename, a shop date,
a weapon-store gate and an encounter rate, and its `main.luau` reads the
result, lists conflicts and grows an enemy during a run with a persisted
session edit. `mod_api_check` type-checks both files.

[`p4mod check`](../p4mod.md#check) type-checks `data.luau` against the smaller
`p4` it runs with, and runs `data/edits.xml` through the game's own decoder and
field rules (ranges, names of elements, affinities and ailments, learn levels)
without your disc. What needs your disc's tables (that a named row exists, a
row's kind or current values) is left to `p4mod test`, which starts the game and
shows its reason if it refuses the edits.

### When edits happen

The game builds its tables once while it starts, before the title screen.
That's when every enabled mod's `data/edits.xml` and `data.luau` run, in
load order (within one mod, `edits.xml` first). Each mod sees the tables as
the mods before it left them, so "20% more HP than whatever is there" works
on top of another mod's numbers. The result is what the game loads every
time it reads its tables. Disable the mod and the next start is unmodded.

`data.luau` runs in its own small world: `p4.data` (the readers and
`edit`), `p4.log`, `p4.has`, `p4.capabilities`, `p4.settings.get` (your
settings as they were when the game started) and `p4.id`. Hooks, memory,
events and UI don't exist there, because the game hasn't started yet.
`main.luau` runs later, once the game is up; calling `p4.data.edit` there is
an error that tells you to move it to `data.luau`. From `main.luau`,
`p4.data` reads the tables with every mod's edits applied.

### What you can change

| Kind | Field | Range |
|---|---|---|
| skill, enemy, Persona | `name` | printable ASCII, at most 18 (skill), 20 (enemy) or 16 (Persona) characters |
| skill | `power` | 0..9999 |
| skill | `cost = { sp = n }` / `cost = { hp_percent = n }` | 0..999 SP / 0..100 % |
| skill | `accuracy`, `critical` | 0..100 |
| skill | `hits = { min = n, max = n }` | 1..9, min ≤ max (retail uses 1..3) |
| skill | `element` | an attack element, on an attack skill |
| skill | `ailment = { status = "...", chance = n }` | on a skill that already inflicts one |
| enemy | `level` | 1..99 |
| enemy | `hp`, `sp`, `exp`, `yen` | 1..65535 (HP), 0..65535 |
| enemy | `stats.st` .. `stats.lu` | 1..255 |
| enemy | `affinities.<element>` | a kind (`weak`, ...) or a rate in percent (multiple of 5) |
| enemy | `skills[1..8]` | a skill, or `"none"` |
| enemy | `drops[1..4] = { item = ..., chance = n }` | chance 0..100 in 0.5 steps, all four together ≤ 100 |
| Persona | `level`, `stats.st` .. `stats.lu` | 1..99 |
| Persona | `affinities.<element>` | as for enemies |
| Persona | `learnset:add{ level, skill }`, `:remove(skill)`, `:move(skill, level)`, `:replace(skill, with)` | levels 1..99 |
| item | `name` | printable ASCII, at most 31 characters (capability `data.item_names@1`) |
| item | `price` | 0..999999 |
| item | `attack`, `hit` (weapons), `defense`, `evasion` (armor) | 0..9999 |
| item | `skill` | what a consumable uses or an accessory grants |
| item | `shop.sold` | `true` or `false` (capability `data.shop@1`) |
| item | `shop.from = { month = n, day = n }` | consumables: month 1..12, day 1..31 |
| item | `shop.unlock[1..2] = { material = ..., count = n }`, or `{}` for none | gear: a material, count 0..255 (0 turns the gate off) |
| floor | `spawn`, `refill` | 0..15 (capability `data.dungeons@1`) |
| floor | `chests` | 0..8 |
| encounters | `rates.normal`, `rates.special`, `rates.rare` | 0..255, not all three 0 |
| encounters | `normal[1..20]`, `special[1..5]`, `rare[1..5]` `= { formation = ..., weight = n }`, or `{}` to empty | weight 0..65535 |
| encounters | `alternate.<the same>` | as above |
| formation | `enemies[1..5]` | an enemy, or `"none"` |
| treasure | `items[1..29] = { item = ..., weight = n, locked = true }`, or `{}` to empty | weight 0..65535 |

Through a floor, `floor.encounters` and `floor.treasure` edit the group it
uses, so every floor sharing that group changes with it.

In XML the same fields are attributes: `rename` (the new `name`; `name`
finds the row), `power`, `sp`, `hp-percent`, `accuracy`, `critical`,
`hits-min`, `hits-max`, `element`, `ailment`, `ailment-chance` on `<skill>`;
`rename`, `level`, `hp`, `sp`, `st`..`lu`, `exp`, `yen` and an element or
ailment name per affinity on `<enemy>` and `<persona>`; `rename`, `price`,
`attack`, `hit`, `defense`, `evasion`, `skill`, `sold`, `from` (`"4/20"`) and
`unlock="none"` on `<item>`. Enemies hold `<skill slot= name=/>` and
`<drop slot= item= chance=/>`; Personas hold `<learn skill= level=/>`,
`<forget skill=/>`, `<move skill= level=/>` and `<replace skill= with=/>`;
items hold `<unlock slot= material= count=/>`. `<floor>` takes `spawn`,
`refill`, `chests`, `encounters` and `treasure` (group IDs);
`<encounters id=>` takes `normal-rate`, `special-rate`, `rare-rate` and
`alternate-normal-rate` and so on, and holds
`<normal|special|rare slot= formation= weight= alternate="true"/>`;
`<formation id=>` holds `<enemy slot= name=/>`; `<treasure id=>` holds
`<record slot= item= weight= locked=/>`.

`shop.sold = true` puts a never-sold consumable on sale from 4/1 and takes
the never-sold mark off gear; dates and gates it already has stay. `false`
marks it never sold. The game year runs from April to March, so a date is
compared in that order.

A renamed row keeps its key, and every mod can still find it by the name on
the disc. Menus, shops and messages show the new name; a long one may not fit
every menu.

A fusion Persona's learn levels count from its base level, so changing
`level` moves them with it. A party member's Persona (Jiraiya, Tomoe, ...)
learns at fixed character levels.

### Mistakes and conflicts

Every value is checked while the tables are built. A mistake (an unknown
field, a number out of range, a name that isn't found) disables **that mod**
for this start, with a message in the MODS list that names the file, line or
row and what to fix, for example:

```
data/edits.xml: line 3: skill "Agilao" (p4:skill:agilao) power: 10000 is not a whole number from 0 to 9999
```

Nothing of a refused mod is applied; the other mods start normally.

When two mods set the same field, the one later in the load order wins.
That's never silent: `p4.data.conflicts()` (from `main.luau`) lists every
such field with all its owners and the winner:

```luau
for _, c in p4.data.conflicts() do
    p4.log(c.key .. " " .. c.field .. ": " .. table.concat(c.owners, ", ") .. " -> " .. c.winner)
end
```

## Changing the data during a run

> Requires capability `data.session@1`.

`data.luau` decides the tables before the game starts. To change them while
the game runs (a randomizer, a difficulty that grows, a challenge run), call
`p4.data.session.edit` from `main.luau`. It takes the same `d` and the same
fields as `data.luau`, and the change is live at once:

```luau
p4.events.on("battle_end", function()
    p4.data.session.edit(function(d)
        local hablerie = d.enemy("Lying Hablerie")
        hablerie.hp = hablerie.hp + 10
    end)
end)
```

Session edits belong to the current run:

- Loading a save or starting a new game undoes every session edit.
- `p4.data.session.edit(fn, { persist = true })` keeps that call's edits in
  the next save. Loading that save applies them again, by key, after
  undoing the rest; a mod that is no longer enabled has its saved edits left
  off. A new game starts without them.
- An error inside `fn`, or a value out of range, undoes the whole call.
- Item names, and item rows when another mod added items, change only in
  `data.luau`.

## Not yet

Descriptions and new content are on their way. Until then, the table-source
layer in [Native game-table expansion](../tables.md) remains available for
advanced mods.
