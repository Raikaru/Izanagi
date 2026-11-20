--!capability buff@1
-- p4.buff.on: change the multiplier a unit's buff rank, Charge or Concentrate gives one stat, for party members and enemies alike (docs/modding/tables.md "Buff stage").
-- One stat multiplier, frozen. stat: attack and magic (the source's, in damage), defense (the target's damage
-- taken), accuracy and evasion (the hit chance). rank is -1, 0 or 1; charged is this attack's Charge or
-- Concentrate; stock is the game's multiplier (rank table, x2.5 Charge, Lead boost).
export type P4BuffQuery = {
    side: P4DamageSide,
    unit: number,
    stat: "attack" | "magic" | "defense" | "accuracy" | "evasion",
    rank: number,
    charged: boolean,
    stock: number,
}
-- fn returns a factor (multiply) or an addend (add), or nil to leave it alone; the result is clamped to 0..100.
export type P4BuffSpec = { kind: "multiply" | "add", priority: number?, fn: (query: P4BuffQuery) -> number? }
--!in p4
buff: { on: (specification: P4BuffSpec) -> P4Subscription },
--!example
local charge: P4Subscription = p4.buff.on({
    kind = "multiply",
    fn = function(query: P4BuffQuery): number?
        if query.charged then return 1.8 / 2.5 end
        return nil
    end,
})
--!refuse
p4.buff.on({ kind = "set", fn = function(query) return 1 end })
p4.buff.on({ kind = "multiply", fn = function(query) local s: "luck" = query.stat; return nil end })
