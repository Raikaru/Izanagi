# Actions: changing what the player has

Actions change game state through the game's own routines, so every rule the game keeps (stack
limits, key-item flags, the money cap) still holds. They return what actually changed, and a
refused action changes nothing.

## Items and money

```luau
-- mod.ini: requires = inventory@1, give.item@1, give.money@1
p4.events.on("battle_end", function(e)
    p4.give.item("Medicine", 2)   -- queued: applied as soon as the battle is over
    p4.give.money(500)
end)
```

| Call (capability) | Does | Returns |
|---|---|---|
| `p4.inventory.count(item)` (`inventory@1`) | the stack the player holds; an item the catalog cannot name (a placeholder row, or a weapon another mod renamed to one) is an error, so count weapon fusion vessels with [`p4.forge.vessel`](../lua.md#weapon-fusion-transaction-forge1) (`forge.vessel@1`) | a number |
| `p4.inventory.money()` (`inventory@1`) | the player's yen | a number |
| `p4.give.item(item, count?)` (`give.item@1`) | adds 1..99 (default 1), up to the game's 99 per stack | how many were added |
| `p4.take.item(item, count?)` (`give.item@1`) | removes up to `count` (default 1) | how many were removed |
| `p4.give.money(amount)` (`give.money@1`) | adds yen, up to 9,999,999 | how much was added |
| `p4.take.money(amount)` (`give.money@1`) | removes yen, down to 0 | how much was removed |

A change that cannot apply yet returns the requested amount and `"queued until the battle ends"`
(or `"queued until the field is ready"` between scenes); a refused one returns `0` and the reason.

An `item` is an ID, a row from `p4.data` (`p4.data.item("Medicine")`) or a name or key the data
catalog resolves (`"Medicine"`, `"p4:item:medicine"`). An unknown or ambiguous name is an error
with the catalog's suggestions:
`p4.give.item: no item named "Medecine". Did you mean "Medicine" (p4:item:medicine)?`

**When.** Reads (`count`, `money`) work in every callback. Changes made in the field, a
dungeon or an event apply at once. Changes made during a battle (or between scenes) are checked
at once and applied, in order, at the first frame outside it; they are dropped with a log line if
the player quits to the title first. With no game loaded they are refused (`"no game is
loaded"`). Decisions, authorizers, synchronous UI callbacks and save snapshots cannot change
anything (an error). A change that took effect counts as the mod changing the game (save
provenance), like any other semantic use.

## Skills, EXP and levels

```luau
-- mod.ini: requires = party.persona@1, give.skill@1, give.exp@1
local yosuke = p4.party.persona("Yosuke")      -- nil and a reason before he joins
if yosuke and #yosuke.skills < 8 then
    p4.give.skill("Yosuke", "Garula")
else
    p4.give.skill("Yosuke", "Garula", { replace = "Garu" })
end
p4.give.exp("Yosuke", 500)                     -- his Persona levels up the game's way
p4.give.exp("protagonist", 1000)               -- the protagonist's own level
```

| Call (capability) | Does | Returns |
|---|---|---|
| `p4.party.persona(who)` (`party.persona@1`) | the member's Persona now: `id`, `key`, `name`, `level`, `exp`, `skills` (IDs in slot order) | a table, or `nil` and a reason |
| `p4.party.level(who)` (`party.persona@1`) | the protagonist's own level (from his EXP), or a member's (their Persona's) | a number, or `nil` and a reason |
| `p4.party.hero_level()` (`party.hero_level@1`) | the protagonist's stored level, as the game's own level read returns it; EXP awaiting a level-up does not raise it | a number, or `nil` and a reason |
| `p4.give.skill(who, skill, { replace = skill }?)` (`give.skill@1`) | teaches the skill in the first empty slot, or in place of `replace` | `true`, or `false` and a reason |
| `p4.give.skill(who, skill, { choice = true, done = fn? })` (`give.skill.choice@1`) | as above; with all 8 slots full the player picks a skill to forget on the game's own panel | `true` (or `true, "queued ..."`), or `false` and a reason; the outcome goes to `done` |
| `p4.give.exp(who, amount, { persona = true }?)` (`give.exp@1`) | adds 1..9,999,999 EXP with the game's level-ups | `true`, or `false` and a reason |

`who` is `1`-`8`, a character name (`"protagonist"`, `"Yosuke"`, `"Chie"`, `"Yukiko"`, `"Rise"`,
`"Kanji"`, `"Naoto"`, `"Teddie"`) or a unit table with a `character` field, such as a battle
event's `e.unit`. For the protagonist, `persona` and `give.skill` mean his current Persona; his own
level is separate and is what `give.exp` raises unless `persona = true`. A party member's level is
their Persona's. A `skill` is an ID, a `p4.data` row or a name or key, like an item.

A skill is refused when the Persona already knows it, when all 8 slots are full and no `replace`
(or `choice`) is given, or when `replace` is a skill it does not know. A level-up from `give.exp`
teaches the Persona's own skills for those levels, as after a battle; one that does not fit (8
already) is not learned and the log says so, so the mod can offer it with `give.skill` and
`replace` or `choice`. A level-up of the protagonist's Personas (not the party's, whose growth is
fixed) rolls their stat gains with the game's random numbers, as the battle result does. Neither
refills HP or SP.

Members who have not joined the party, or have no Persona yet, are refused with that reason. The
timing rules are those of items: at once outside battle, queued during one (checked when queued,
applied in order after; one that no longer holds then is logged and skipped).

Changes to one Persona apply in the order they were asked for, together with
`p4.party.set` (`docs/modding/lua.md`, "Party records"). A `set` applies at the next frame
boundary, so a `give.skill`, `give.exp` or `choice` for that Persona asked for after it returns
`true, "queued behind a pending p4.party.set for that Persona"` and applies right after the
record is written; one asked for while an earlier change to that Persona is still queued returns
`true, "queued behind an earlier change to that Persona"`. A `set` waits for the queued
`give.skill` and `give.exp` changes to its Persona asked for before it.

A `give.skill` (or `choice`) asked for behind a pending `set` or queued changes is checked against
the skills they leave: the `set`'s list, then the queued `give.skill` changes after it. So
`set` without Garula followed by `give.skill(who, "Garula")` is accepted and teaches it, and a
`give.skill` the `set`'s list makes invalid (already known, `replace` not in it, 8 skills without
`replace`) is refused at once. Skills a queued `give.exp` level-up would teach, a battle's own
results and the panel's answer cannot be known in advance: every queued change is checked again
when it applies, and one that no longer holds then is logged and skipped. Other Personas and the
protagonist's own level are not held.

Named story flags are a separate capability.

## Letting the player choose a skill to forget

```luau
-- mod.ini: requires = party.persona@1, give.skill@1, give.skill.choice@1
p4.events.on("battle_end", function(e)
    p4.give.skill("Yosuke", "Garula", { choice = true, done = function(outcome)
        if outcome.learned then
            p4.log("Yosuke's Persona learned Garula, forgetting " .. tostring(outcome.forgot))
        else
            p4.log("not learned: " .. outcome.reason) -- "declined": the player kept every skill
        end
    end })
end)
```

With `choice = true` (`give.skill.choice@1`) a Persona with an empty slot learns the skill as with
plain `give.skill`. With all 8 slots full the game's own panel asks, as after a battle: the
Persona's skills with the new one below them, the result screen's "can't learn more than 8
skills, forget which?" line (read from the user's disc), and Yes/No for the skill the player
picks. Picking the new skill itself and answering Yes keeps every skill ("did not learn"); the
panel has no cancel button of its own. `choice` cannot be combined with `replace`.
The field host loads the game's acquisition-banner sprite before opening the choice and keeps it
alive until the native panel has closed.


The panel needs the field to itself. It opens at the first moment the player is free in a field
or dungeon (no battle, event, script, menu, Camp, Config or scene change), holding the field the
way talking to someone does; until then the call returns `true` and `"queued ..."`. Choices open
one at a time, in the order they were asked for. While one is open `p4.scene.state().ready` is
false ("the game's skill choice holds the field"), so warps and encounters wait for it. A warp or
encounter accepted just before the panel would open goes first, and the choice waits for the
field after it. Like an event, the panel ends a mod's camera lease before it opens (an exclusive
input is released once every button is up), and no lease starts while it is about to open.

While the panel shows a Persona, its skill list is the panel's: `give.skill` and `give.exp` for
that Persona return `true, "queued until the skill choice open for that Persona closes"` and
apply, checked again, once the player has answered (a skill the panel just taught is then
refused as already known), and a `p4.party.set` for it waits for the answer too. Other Personas
and the protagonist's own level are not held.

`done` (optional) runs once, at a frame boundary after the outcome is known (also when the skill
was learned at once), with `{ member, skill, learned, forgot?, reason? }`: `forgot` is the skill
given up for it, `reason` says why it was not learned. A choice that is no longer needed when its
turn comes is settled without the panel: learned if a slot freed meanwhile, not learned if the
Persona knows the skill by then. A choice is the one change that does not hold its place in call
order until it comes up: `give.skill`, `give.exp` and `p4.party.set` asked for while it waits for
its turn apply first (that is the "meanwhile" above), and it settles against the Persona they
leave; once the panel shows it, they wait for the answer. A choice asked for after a pending `set`
or queued change comes up only after those apply. Quitting to the title ends every choice still
waiting, unlearned.
A mod that is shut down never hears its outcomes; a panel already on screen still finishes for
the player.

What the player chose is part of the Persona, so the game's own save keeps it. A choice still
waiting is not part of any save: a mod that must offer a skill across a reload checks
`p4.party.persona` after `p4.events.on("load", ...)` and asks again.
