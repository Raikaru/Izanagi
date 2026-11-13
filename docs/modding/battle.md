# Battle decisions

## Choosing an enemy's turn (capability `battle.decide@1`)

`p4.battle.decide` lets a mod decide what an enemy does on its own turn: use any skill, aimed at
one legal target, or skip the turn. Boss phases, telegraphed attacks and "hit the weakest party
member" rules need no hooks or addresses.

```luau
-- mod.ini: requires = battle.decide@1
p4.battle.decide({ side = "enemy", fn = function(d)
    if d.actor.id ~= 0x2B0 then return nil end          -- not our boss: keep its own choice
    if d.actor.hp_fraction < 0.5 and not d.actor.charged then
        return { skill = 227 }                          -- Power Charge (targets itself)
    end
    local targets = d.candidates(10)                    -- who Agi could hit now
    local weakest = nil
    for _, unit in targets do
        if weakest == nil or unit.hp < weakest.hp then weakest = unit end
    end
    if weakest then return { skill = 10, target = weakest } end
    return { pass = true }
end })
```

**When it runs.** Once per enemy turn, after the enemy's own AI (its table or its script) has
finished choosing and before the turn plays out. Every kind of enemy turn passes here, One More
and extra turns included (`d.one_more`, `d.extra`).

**What `d` holds** (a read-only copy):

| Field | Meaning |
|---|---|
| `actor` | the deciding enemy |
| `units`, `party`, `enemies` | every unit in the battle; `party`/`enemies` are the same unit views split by side |
| `pick` | what the enemy's own AI chose: `kind` (`"skill"`, `"pass"` or `"other"`), `skill`, `targets` |
| `round` | how many base turns the protagonist has had |
| `candidates(skill)` | the units the game would let that skill hit right now, and `true` when it hits all of them |

A unit view has `slot`, `side` (`"party"` or `"enemy"`), `id` (playable character or enemy ID),
`uid`, `alive`, `hp`, `hp_max`, `hp_fraction`, `sp`, `sp_max`, `status` (the ailment word) and
`charged` (Power Charge or Mind Charge is up).

**What `fn` returns:**

- `nil`: keep the enemy's own choice.
- `{ skill = id }`: use that skill. A skill that hits everyone, or only its user, needs no target.
  A single-target skill keeps the enemy's own target when that is still legal, otherwise takes the
  first unit of `d.candidates(skill)`; it never draws a random one.
- `{ skill = id, target = unit }`: aim it at that unit (a unit view from `d`, or its `slot`).
- `{ pass = true }`: the enemy waits this turn.

**Refusals.** The choice must be legal by the game's own rules: an inactive skill, a target that
`d.candidates(skill)` does not list, or a skill nobody can be hit by right now is refused. The
log says why (`p4pc: mod <id>: p4.battle.decide: skill 10 refused: the target is not a legal
target of that skill; the enemy's own choice stands`) and the enemy's own choice plays out. A
malformed return value or a Lua error is a mod error, handled like any other callback error.

**Rules of the callback.** `fn` runs in snapshot mode: it may read `d`, call `d.candidates` and
look rows up in the data catalog (`p4.data`, capability `data.catalog@1`), but not call game
functions, change memory, log, give or take anything, show UI, queue banners or lines
(`p4.battle.announce`/`say`, see [Banners and lines in battle](systems/battle-presentation.md#from-a-decision))
or change settings; each of those is an error, which counts as a failed callback. Keep any state
you need in your own tables. `d.candidates` only works inside the call that received `d`.

**Several mods.** Lower `priority` runs first (default 0, then load order); each later decider
sees the earlier one's result as `d.pick`. `p4.battle.decide` returns a subscription with
`enable`, `disable` and `remove`.

**Scope of revision 1.** Enemies only (`side = "enemy"`); skills by numeric ID (resolve a name
once with `p4.data.skill("Agi").id`). Party AI and summons are not covered. To show or react to
a decision, use the battle events: `skill_used` reports the command that actually left for
execution.
