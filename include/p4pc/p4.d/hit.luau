--!capability hit@1
-- p4.hit.on: change the chance that an attack hits and the chance that it lands a critical, for party members and enemies alike (docs/modding/tables.md "Hit stage").
-- One rolled chance, frozen: "accuracy" (after the game's 50..99 clamp) or "critical" (after its 0..100 clamp),
-- the skill, the attacker and the target, and the game's chance in percent. Automatic hits and guaranteed
-- criticals never ask.
export type P4HitQuery = {
    roll: "accuracy" | "critical",
    skill: number,
    source_side: P4DamageSide, source_id: number,
    target_side: P4DamageSide, target_id: number,
    stock: number,
}
-- fn returns a factor (multiply) or an addend (add), or nil to leave it alone; the result is truncated and clamped
-- to 0..100 (0 never succeeds, 100 always does).
export type P4HitSpec = { kind: "multiply" | "add", priority: number?, fn: (query: P4HitQuery) -> number? }
--!in p4
hit: { on: (specification: P4HitSpec) -> P4Subscription },
--!example
local focus: P4Subscription = p4.hit.on({
    kind = "add",
    fn = function(query: P4HitQuery): number?
        if query.roll == "critical" and query.source_side == "party" then return 10 end
        return nil
    end,
})
--!refuse
p4.hit.on({ kind = "set", fn = function(query) return 100 end })
p4.hit.on({ kind = "add", fn = function(query) local r: "evade" = query.roll; return nil end })
