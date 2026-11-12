--!capability stage.recovery@1
-- p4.recovery.on: Luau modifiers of formula HP heals in the native recovery stage (end of datCalcSkillAmount 0x00235520).
-- One formula HP heal (recovery stage), frozen. Sides and origin are stage.damage's types.
export type P4RecoveryHit = {
    skill: number,
    source_side: P4DamageSide, source_id: number,
    target_side: P4DamageSide, target_id: number,
    resolved: boolean, origin: P4StageOrigin,
    stock: number, -- the heal after Divine Grace and the variance
    source_persona: number?, source_persona_level: number?,
}
export type P4RecoverySpec = {kind: "multiply" | "add", priority: number?, fn: (hit: P4RecoveryHit) -> number?}
--!in p4.recovery
-- Start-up declarations; fn runs in snapshot mode (no native calls/mutation).
on: (specification: P4RecoverySpec) -> P4Subscription,
--!example
p4.recovery.on({kind = "multiply", priority = 20, fn = function(heal: P4RecoveryHit): number?
    return if heal.source_persona ~= nil and heal.origin ~= "item" then 1.15 else nil
end})
--!refuse
p4.recovery.on({kind = "divide", fn = function(heal) return 1 end})
p4.recovery.on({kind = "add", fn = function(heal) local s: string = heal.stock; return nil end})
