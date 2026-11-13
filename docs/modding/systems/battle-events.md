# Battle events

Mods can react to what happens in battle: a turn starting, a skill being used, each hit,
ailments, knockdowns, KOs and revivals, One Mores, and the result's rewards, level-ups and
learned skills. Events report what the game has already committed; they do not change an
outcome. To choose what an enemy does, use `p4.battle.decide` (capability `battle.decide@1`,
[Battle decisions](../battle.md)); its callbacks only read, so present or react to a decision
from these events.

```luau
-- mod.ini: requires = events.ailment@1, ui.status@1
p4.events.on("ailment", function(e)
    if e.ailment == "poison" and e.change == "applied" and e.unit.side == "party" then
        p4.ui.status("Poisoned!", "hud")
    end
end)
```

| Event (capability) | Fields besides `name` and `sequence` |
|---|---|
| `turn_start` (`events.turn_start@1`) | `unit`; `follow_up` is true for a follow-up attack (a One More turn is reported as an ordinary turn) |
| `skill_used` (`events.skill_used@1`) | `unit`, `kind` (`"attack"`, `"skill"`, `"item"`), `skill`, `item` for items |
| `damage` (`events.damage@1`) | `source`, `target`, `skill`, `amount` (HP lost; negative when drained), `hp` (the target's HP after), `result` (`"hit"`, `"weak"`, `"critical"`, `"null"`, `"drain"`), `reflected`, `endured` |
| `miss` (`events.miss@1`) | `source`, `target`, `skill` |
| `ailment` (`events.ailment@1`) | `source`, `unit`, `skill`, `status` (the ailment bit), `ailment` (`"dizzy"`, `"rage"`, `"fear"`, `"silence"`, `"confusion"`, `"poison"`, `"exhaustion"`, `"enervation"`, or nil for another bit), `change` (`"applied"`, `"removed"`) |
| `knockdown` (`events.knockdown@1`) | `source`, `unit`, `skill` |
| `ko` (`events.ko@1`) | `source`, `unit`, `skill` |
| `revive` (`events.revive@1`) | `source`, `unit`, `skill` |
| `one_more` (`events.one_more@1`) | `unit` that earned the extra turn, `cause` (`"weakness"`, `"critical"` or `"down"`: what its action's hits did) |
| `reward` (`events.reward@1`) | after a won battle's result gave them: `exp` (the protagonist's), `money` (the yen given, after an Arcana Chance and `p4.reward` modifiers), `items` (`{ item, count }`), `party` (`{ character, exp }` for each member who earned EXP) |
| `level_up` (`events.level_up@1`) | `character`, `persona` (the Persona that levelled up; nil for the protagonist's own level), `level`, `from` |
| `skill_learned` (`events.skill_learned@1`) | `character`, `persona`, `skill`: a skill a result's level-up taught |

A unit (`unit`, `source`, `target`) has `side` (`"party"` or `"enemy"`), `id` (playable
character or enemy ID) and `uid`. Party members also carry `character` (`"protagonist"`,
`"yosuke"`, `"chie"`, ...) when known; enemies carry the data catalog's `key` and current
`name` when available. `skill` and `item` have an `id` and, when available, a catalog `key`
and `name`.

## Order and timing

Events arrive in the order the game committed them, at the next frame boundary, so a reaction
(a message, a status line) shows before the next unit acts. Within one action:

1. `turn_start`
2. `skill_used`
3. for each target and each hit row, in the game's order: `damage` or `miss`, then that hit's
   `ailment` changes (in bit order), `knockdown`, `ko`/`revive`

A `one_more` follows its action's hits, when the game adds the extra turn (at the action's first
hit animation). After a won battle, `reward` comes once the result has given the EXP, yen and
items, followed by the protagonist's own `level_up` if he gained one; each Persona that levels up
then gives its `level_up` and one `skill_learned` per skill it learned. A level-up skill that did
not fit (8 already) is not reported as learned. Characters are `"protagonist"` for him and his
Personas, else the member (`"yosuke"`, ...).

`sequence` numbers events in commit order across the whole session. Callbacks run like
`frame_start` callbacks: they may call game services and UI, but the event itself is a copy and
read-only.

## What revision 1 does not report

- An ailment that was blocked (immune, guarded, failed roll) is not reported yet; only applied
  and removed ailments are.
- Hits carry no hit index or count yet; a multi-hit skill gives one `damage` per row.
- Self effects (regeneration, ailment ticks, guard, a drain skill's own heal) are not `damage`.
- Turn end, Persona switches and world events (fusion, shop, rank up) are separate capabilities.
- Level-ups and skills from anything but a battle result (fusion bonuses, `p4.give.exp`) are not
  reported.
