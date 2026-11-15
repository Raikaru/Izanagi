--!capability stage.cost@1
-- p4.cost.on: Luau modifiers of nonzero skill costs in the native cost stage (end of datCalcSkillCost 0x0023D9B0).
-- One nonzero skill cost (skill cost stage), frozen. The side is stage.damage's type.
export type P4CostHit = {
    skill: number,
    side: P4DamageSide, unit: number,
    resource: "hp" | "sp",
    stock: number, -- after Spell/Arms Master halving and the minimum of 1
    -- the persona this cost is priced for (a Persona/Camp row may show a stocked one); nil for an enemy
    persona: number?, persona_level: number?,
}
export type P4CostSpec = {kind: "multiply" | "add", priority: number?, fn: (hit: P4CostHit) -> number?}
--!in p4.cost
-- Start-up declarations; fn runs in snapshot mode (no native calls/mutation).
on: (specification: P4CostSpec) -> P4Subscription,
--!example
local priced: P4Subscription = p4.cost.on({kind = "multiply", priority = 20, fn = function(cost: P4CostHit): number?
    local resource: "hp" | "sp" = cost.resource
    return if resource == "sp" and cost.persona ~= nil and cost.side == "party" then 0.81 else nil
end})
priced:disable()
--!refuse
p4.cost.on({kind = "multiply", fn = function(cost) local r: number = cost.resource; return nil end})
p4.cost.on({kind = "multiply", fn = function(cost) return "cheap" end})
