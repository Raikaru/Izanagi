--!capability status.wearoff@1
-- p4.status.wearoff.on: change the chance that a unit shakes off an ailment on its own turn, for party members and enemies alike (docs/modding/tables.md "Ailment wear-off stage").
-- One recovery roll, frozen: the unit whose turn it is, the ailment, and the native chance in percent. turns counts
-- the unit's own turns with the ailment so far (rolls start at 2); limit is the turn that cures it without a roll.
export type P4WearoffRoll = {
    side: P4DamageSide,
    unit: number,
    status: "confusion" | "fear" | "rage" | "exhaustion" | "enervation" | "silence",
    level: number,
    luck: number,
    turns: number,
    limit: number,
    stock: number,
    afflictions: number, -- status.afflictions@1: ailments landed on the unit this battle, this one included
}
-- fn returns a factor (multiply) or an addend (add), or nil to leave the roll alone; the result is clamped to 0..100.
export type P4WearoffSpec = { kind: "multiply" | "add", priority: number?, fn: (roll: P4WearoffRoll) -> number? }
--!in p4.status
wearoff: { on: (specification: P4WearoffSpec) -> P4Subscription },
--!example
local fade: P4Subscription = p4.status.wearoff.on({
    kind = "add",
    fn = function(roll: P4WearoffRoll): number?
        if roll.side == "enemy" then return nil end
        return 15 * (roll.turns - 1)
    end,
})
--!refuse
p4.status.wearoff.on({ kind = "divide", fn = function(roll) return 1 end })
p4.status.wearoff.on({ kind = "add", fn = function(roll) local s: "poison" = roll.status; return nil end })
