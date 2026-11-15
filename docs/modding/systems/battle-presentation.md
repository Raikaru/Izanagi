# Banners and lines in battle

Two capabilities show text in battle, timed to a unit's turn:

| Capability | Call | Shows |
|---|---|---|
| `battle.announce@1` | `p4.battle.announce{ unit, text, at?, frames?, hold? }`, `p4.battle.announce_clear(unit?)` | a one-line banner in the box where the game shows skill names, optionally held there until the unit's next turn |
| `battle.say@1` | `p4.battle.say{ unit, text, speaker?, at?, frames? }` | a message window with the speaker's name and one line |

```luau
-- mod.ini: requires = events.turn_start@1, battle.announce@1, battle.say@1
p4.events.on("turn_start", function(e: P4TurnStartEvent)
    if e.unit.side == "enemy" then
        p4.battle.announce({ unit = e.unit, text = "A Shadow is gathering power!" })
        p4.battle.say({ unit = e.unit, text = "You'll burn!", at = "turn_end" })
    end
end)
```

### From a decision

`p4.battle.decide` callbacks (capability `battle.decide@1`, [Battle decisions](../battle.md)) run
in snapshot mode: they only read. `p4.battle.announce`, `p4.battle.announce_clear` and
`p4.battle.say` are errors there, as they are in authorizers and save snapshots, because a line
queued while choosing would show a choice the game may still refuse. Keep what you chose in your
own table, keyed by the unit's `uid`, and present it from an event of that unit:

```luau
-- mod.ini: requires = battle.decide@1, events.skill_used@1, battle.announce@1
local chosen: { [number]: number } = {} -- enemy uid -> skill this mod picked
p4.battle.decide({ side = "enemy", fn = function(d)
    if d.actor.hp_fraction >= 0.5 then return nil end
    chosen[d.actor.uid] = 10 -- Agi; a refused choice is never used, so check it below
    return { skill = 10 }
end })
p4.events.on("skill_used", function(e: P4SkillUsedEvent)
    local skill = chosen[e.unit.uid]
    chosen[e.unit.uid] = nil
    if skill ~= nil and e.skill.id == skill then
        p4.battle.announce({ unit = e.unit, text = "It lashes out in desperation!" })
    end
end)
```

## When it appears

`unit` is a battle unit table with a `uid`, such as an event's `e.unit`. The item waits for
that unit's next turn:

- `at = "turn_end"` (the default): after the unit's action has played, before the next unit
  starts.
- `at = "turn_start"`: when the unit's next turn begins, before its action (and, for a party
  member, before the command menu opens).

The unit's turn pauses while the item is on screen, the way the game pauses for a boss's
scripted talk. Several items for the same unit and moment play one after another, in the order
they were queued.

The game's own text comes first: an item waits while the banner box shows a skill name, while
another message window is open or while a boss talks. An item that cannot show for 240 frames is
dropped with a log line. The battle simulator (`p4pc --battle-sim`) runs the same waits with or
without graphics, so its results include the time the items take.

## Held banners

With `hold = true`, a banner stays after its `frames`: whenever the game's own lines (a skill name,
a turn-start line, the command menu's) leave the box, it comes back, until the same unit's next turn
starts. A banner shown at `turn_start` stays through that turn and ends at that unit's following
turn start. A banner shown at `turn_end` covers every party turn up to the boss's next move.

```luau
-- Inside the turn_start callback above:
p4.battle.announce({ unit = e.unit, text = "A Shadow is gathering power!", hold = true })
```

`p4.battle.announce_clear(unit)` ends your mod's held banners for that unit early, and
`p4.battle.announce_clear()` all of them; it returns how many ended. When several are held, the
newest shows. A held banner of a unit that leaves the battle ends with it.

## Text

`text` is one line of 1-40 characters: letters, digits, punctuation and the game's own two-byte
glyphs. A newline or a control character is an error. A banner stays for `frames` (16-600,
default 60), then fades.

A line waits for the player's confirm button, like the game's dialogue. With `frames`, it closes
by itself after that many frames instead and ignores buttons; use that for lines during enemy
turns that should not stop the battle. `speaker` (1-16 characters) replaces the name shown, which
is otherwise the unit's own: the protagonist's chosen name, a party member's or the enemy's.

## Results

Both return `true` when the item is queued. They return `false` and a reason when no battle is
running, the unit is not in this battle, or four items already wait for that unit at that moment.
Bad arguments (no `unit`, text too long or not drawable, an unknown `at`) are errors.

Queueing changes nothing in the game until the unit's turn reaches that moment. With nothing
queued the game runs exactly as without mods. Items for units that have left the battle are dropped;
a mod that is turned off drops its waiting items, and one already on screen finishes.
