--!capability stage.damage@1
-- p4.damage.on: Luau final-damage modifiers in the native damage stage (datCalcSkillAmount 0x00235520).
-- One eligible HP-damage hit (final-damage stage), frozen.
export type P4DamageSide = "party" | "enemy"
export type P4DamageHit = {
    skill: number,
    source_side: P4DamageSide, source_id: number,
    source_status: number, -- the attacker's DatUnit ailment word before this action
    target_side: P4DamageSide, target_id: number,
    target_status: number, -- the target's DatUnit ailment word before this action
    origin_side: P4DamageSide, origin_id: number, -- the unit aimed at; differs from target only when reflected
    hit_kind: "normal" | "critical" | "weak" | "other",
    element: number?, reflected: boolean,
    resolved: boolean, -- false for an estimate (AI target choice, previews)
    stock: number, -- the retail equation's value before any modifier
}
export type P4DamageSpec = {
    kind: "multiply" | "add",
    priority: number?, -- lower runs first; default 0
    fn: (hit: P4DamageHit) -> number?, -- factor or addend; nil leaves the hit alone
}
--!in p4.damage
-- Start-up declarations; fn runs in snapshot mode (no native calls/mutation).
on: (specification: P4DamageSpec) -> P4Subscription,
--!example
local technical: P4Subscription = p4.damage.on({kind = "multiply", priority = 10, fn = function(hit: P4DamageHit): number?
    local side: P4DamageSide = hit.target_side
    local element: number? = hit.element
    if side == "enemy" and hit.hit_kind == "weak" and hit.resolved and element ~= nil then
        return 1.2
    end
    return nil
end})
technical:disable()
p4.damage.on({kind = "add", fn = function(hit) return if hit.reflected then -hit.stock // 2 else nil end})
--!refuse
p4.damage.on({kind = "divide", fn = function(hit) return 1 end})
p4.damage.on({kind = "multiply", fn = function(hit) return "double" end})
p4.damage.on({kind = "add", fn = function(hit) local s: string = hit.stock; return nil end})
p4.damage.on({kind = "add"})
