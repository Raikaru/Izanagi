--!capability stage.chance@1
-- p4.chance.on: Luau modifiers of rolled ailment and instant-KO chances in the native chance stage (datCalcInflictStatus 0x00238940).
-- One rolled ailment or instant-KO attempt (chance stage), frozen. Sides are stage.damage's type.
export type P4ChanceHit = {
    skill: number,
    source_side: P4DamageSide, source_id: number,
    target_side: P4DamageSide, target_id: number,
    component: "ailment" | "instant_ko",
    statuses: number, -- the attempted status mask (0x80000 for instant KO)
    item: boolean,
    stock: number, -- the native truncated chance; the 0..99 clamp follows the stage
    afflictions: number, -- status.afflictions@1: ailments landed on the target earlier this battle
    source_persona: number?, source_persona_level: number?,
}
export type P4ChanceSpec = {kind: "multiply" | "add", priority: number?, fn: (hit: P4ChanceHit) -> number?}
--!in p4.chance
-- Start-up declarations; fn runs in snapshot mode (no native calls/mutation).
on: (specification: P4ChanceSpec) -> P4Subscription,
--!example
p4.chance.on({kind = "add", fn = function(attempt: P4ChanceHit): number?
    local ko: boolean = attempt.component == "instant_ko"
    return if ko and not attempt.item and attempt.statuses == 0x80000 then 5 else nil
end})
--!refuse
p4.chance.on({kind = "add", fn = function(attempt) local c: "status" = attempt.component; return nil end})
p4.chance.on({kind = "multiply"})
