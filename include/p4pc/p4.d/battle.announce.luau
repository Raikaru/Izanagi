--!capability battle.announce@1
-- p4.battle.announce: a one-line banner in the battle's skill-name box at a unit's turn start or end, optionally held until its next turn.
export type P4BattleAnnounce = {
    unit: { uid: number }, -- the unit whose turn it waits for: an event's e.unit (not from inside p4.battle.decide)
    text: string, -- one line of 1-40 characters
    at: ("turn_start" | "turn_end")?, -- default "turn_end"
    frames: number?, -- 16-600 frames on screen, default 60
    hold: boolean?, -- then keep it in the box (re-shown after the game's own lines) until the unit's next turn starts
}
--!in p4.battle
-- Queues the banner; the unit's turn waits for it at that moment, after any
-- banner or boss talk of the game's own. false and a reason when no battle
-- runs, the unit is not in it, or 4 items already wait for that unit and moment.
announce: (spec: P4BattleAnnounce) -> (boolean, string?),
-- Ends this mod's held banners for the unit (all of them without one); the number ended.
announce_clear: (unit: { uid: number }?) -> number,
--!example
local ok: boolean, why: string? = p4.battle.announce({ unit = { uid = 7 }, text = "Shadow Yukiko is gathering power!" })
p4.battle.announce({ unit = { uid = 7 }, text = "Brace yourselves!", at = "turn_start", frames = 90 })
p4.battle.announce({ unit = { uid = 7 }, text = "It is gathering power!", hold = true })
local ended: number = p4.battle.announce_clear({ uid = 7 }) + p4.battle.announce_clear()
--!refuse
p4.battle.announce({ unit = { uid = 7 }, text = "x", hold = "yes" })
p4.battle.announce({ unit = { uid = 7 }, text = 5 })
p4.battle.announce({ unit = { uid = 7 }, text = "x", at = "now" })
p4.battle.announce({ text = "no unit" })
