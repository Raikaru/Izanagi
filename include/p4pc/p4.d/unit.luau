--!capability unit@1
-- p4.unit.on: change a unit's max HP and SP (an enemy's starting HP and SP with them) and the five stats battle formulas read, for party members and enemies alike (docs/modding/tables.md "Unit stage").
-- One value, frozen: whose (side, DatUnit ID, level), which, and the game's value. Battle stats are the ones every
-- damage, hit and ailment formula reads, before Enervation halves them.
export type P4UnitQuery = {
    side: P4DamageSide,
    unit: number,
    stat: "max_hp" | "max_sp" | "strength" | "magic" | "endurance" | "agility" | "luck",
    level: number,
    stock: number,
}
-- fn returns a factor (multiply) or an addend (add), or nil to leave it alone; the result is truncated and clamped:
-- max HP 1.. and max SP 0.. up to 999 for the party and 65535 for an enemy, a stat to 1..99.
export type P4UnitSpec = { kind: "multiply" | "add", priority: number?, fn: (query: P4UnitQuery) -> number? }
--!in p4
unit: { on: (specification: P4UnitSpec) -> P4Subscription },
--!example
local tougher: P4Subscription = p4.unit.on({
    kind = "multiply",
    fn = function(query: P4UnitQuery): number?
        if query.side ~= "enemy" then return nil end
        if query.stat == "max_hp" then return 1.5 end
        if query.stat == "agility" then return 1.1 end
        return nil
    end,
})
--!refuse
p4.unit.on({ kind = "set", fn = function(query) return 1 end })
p4.unit.on({ kind = "add", fn = function(query) local s: "hp" = query.stat; return nil end })
