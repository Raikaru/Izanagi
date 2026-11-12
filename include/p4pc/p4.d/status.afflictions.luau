--!capability status.afflictions@1
--!covers p4.chance.on p4.status.wearoff.on
-- afflictions on p4.chance.on hits and p4.status.wearoff.on rolls: how many ailments landed on the unit this battle, for prior-affliction rules such as SMT V's decay (docs/modding/tables.md "Affliction count").
--!example
local decay: P4Subscription = p4.chance.on({
    kind = "multiply",
    fn = function(hit: P4ChanceHit): number?
        if hit.component ~= "ailment" or hit.target_side ~= "enemy" then return nil end
        return math.max(0.1, 1 - 0.3 * hit.afflictions)
    end,
})
local linger: P4Subscription = p4.status.wearoff.on({
    kind = "add",
    fn = function(roll: P4WearoffRoll): number?
        return 10 * (roll.afflictions - 1)
    end,
})
--!refuse
p4.chance.on({ kind = "multiply", fn = function(hit) local n: string = hit.afflictions; return nil end })
p4.status.wearoff.on({ kind = "add", fn = function(roll) local n: boolean = roll.afflictions; return nil end })
