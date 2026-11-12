--!capability stage.damage@3
-- p4.damage.on: Luau final-damage modifiers in the native damage stage (datCalcSkillAmount 0x00235520).
-- One eligible HP-damage hit (final-damage stage), frozen.
export type P4DamageSide = "party" | "enemy"
-- The command 001f14f0 resolves; "unknown" for an unbracketed estimate (revision 2).
export type P4StageOrigin = "skill" | "item" | "attack" | "unknown"
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
    origin: P4StageOrigin, -- revision 2
    -- revision 2: a party source's active persona and its level for a skill or an
    -- unknown origin; nil for an enemy source, an item or an attack.
    source_persona: number?, source_persona_level: number?,
}
export type P4DamageSpec = {
    kind: "multiply" | "add",
    priority: number?, -- lower runs first; default 0
    fn: (hit: P4DamageHit) -> number?, -- factor or addend; nil leaves the hit alone
}
-- Revision 3: an exclusive early HP elemental factor policy. Initially
-- disabled; enable refuses while another policy is active.
export type P4OffensePolicy = {boost: number, amp: number, cap: number}
--!in p4.damage
-- Start-up declarations; fn runs in snapshot mode (no native calls/mutation).
on: (specification: P4DamageSpec) -> P4Subscription,
offense_policy: (specification: P4OffensePolicy) -> P4Subscription,
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
p4.damage.on({kind = "multiply", priority = 20, fn = function(hit: P4DamageHit): number?
    local origin: P4StageOrigin = hit.origin
    local persona: number? = hit.source_persona
    return if origin == "skill" and persona ~= nil and hit.source_persona_level ~= nil then 1.1 else nil
end})
local policy = p4.damage.offense_policy({boost = 1.2, amp = 1.3, cap = 1.55})
policy:enable()
policy:disable()
--!refuse
p4.damage.on({kind = "divide", fn = function(hit) return 1 end})
p4.damage.on({kind = "multiply", fn = function(hit) return "double" end})
p4.damage.on({kind = "add", fn = function(hit) local s: string = hit.stock; return nil end})
p4.damage.on({kind = "add"})
local o: "estimate" = (nil :: any :: P4DamageHit).origin
p4.damage.offense_policy({boost = "bad", amp = 1.3, cap = 1.55})
