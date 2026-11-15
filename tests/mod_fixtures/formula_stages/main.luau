--!strict
-- Authored modder example (not shipped; no game data): a small difficulty mod
-- written only against the formula stages, with no raw hooks or addresses.
-- mod_api_check type-checks it against the shipped definitions. Every fn runs
-- in snapshot mode on a frozen query and must answer the same query the same
-- way each time (the results screen asks for money every frame it draws).

-- Shadows are tougher: a quarter more HP. Party values stay retail.
local tougher: P4Subscription = p4.unit.on({
    kind = "multiply",
    fn = function(query: P4UnitQuery): number?
        if query.side == "enemy" and query.stat == "max_hp" then return 1.25 end
        return nil
    end,
})

-- Shadows land a few more criticals on the party.
local sharper: P4Subscription = p4.hit.on({
    kind = "add",
    fn = function(query: P4HitQuery): number?
        if query.roll == "critical" and query.source_side == "enemy" and query.target_side == "party" then return 5 end
        return nil
    end,
})

-- The party's Charge and Concentrate give x2 instead of x2.5.
local softer_charge: P4Subscription = p4.buff.on({
    kind = "multiply",
    fn = function(query: P4BuffQuery): number?
        if query.side == "party" and query.charged then return 2 / 2.5 end
        return nil
    end,
})

-- SMT V-style decay: every ailment that already landed on a Shadow this battle
-- makes the next one less likely, so ailment spam stops working.
local decay: P4Subscription = p4.chance.on({
    kind = "multiply",
    fn = function(hit: P4ChanceHit): number?
        if hit.component ~= "ailment" or hit.target_side ~= "enemy" then return nil end
        return math.max(0.1, 1 - 0.3 * hit.afflictions)
    end,
})

-- In return the party shakes off repeated ailments faster.
local shake_off: P4Subscription = p4.status.wearoff.on({
    kind = "add",
    fn = function(roll: P4WearoffRoll): number?
        if roll.side ~= "party" then return nil end
        return 10 * (roll.afflictions - 1)
    end,
})

-- Harder fights pay more: a quarter more yen and a tenth more EXP for everyone
-- who earns it (stock Personas without Growth still earn nothing).
local pay: P4Subscription = p4.reward.on({
    kind = "multiply",
    fn = function(query: P4RewardQuery): number?
        if query.reward == "money" then return 1.25 end
        return 1.1
    end,
})
