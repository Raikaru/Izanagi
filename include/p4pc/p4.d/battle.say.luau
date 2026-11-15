--!capability battle.say@1
-- p4.battle.say: a "<speaker>: <line>" message window at a unit's turn start or end.
export type P4BattleSay = {
    unit: { uid: number }, -- the unit whose turn it waits for: an event's e.unit (not from inside p4.battle.decide)
    text: string, -- one line of 1-40 characters
    speaker: string?, -- 1-16 characters; default the unit's own name
    at: ("turn_start" | "turn_end")?, -- default "turn_end"
    frames: number?, -- 16-600: close after that many frames, ignoring buttons; default: wait for confirm
}
--!in p4.battle
-- Queues the line; the unit's turn waits until the window closes, after any
-- message window or boss talk of the game's own. false and a reason when no
-- battle runs, the unit is not in it, or 4 items already wait for that unit and moment.
say: (spec: P4BattleSay) -> (boolean, string?),
--!example
p4.battle.say({ unit = { uid = 7 }, text = "You'll burn!" })
local ok: boolean = p4.battle.say({ unit = { uid = 7 }, text = "Watch out!", speaker = "Rise", at = "turn_start", frames = 120 })
--!refuse
p4.battle.say({ unit = { uid = 7 }, text = "x", speaker = 3 })
p4.battle.say({ unit = 7, text = "x" })
