--!capability authorize.weapon_fusion@1
-- p4.authorize.on("weapon_fusion"): the Velvet Room WEAPON FUSION command's offer and commit stages, separate from fusion.
-- stage: 0 offer (before the Velvet WEAPON FUSION row is shown; vessel, persona
-- and signature 0), 1 commit (inside the forge, before anything is spent).
-- character is a party character ID (1 Yu .. 8 Teddie); signature is 0 or 1.
export type P4WeaponFusionContext = {
    name: "weapon_fusion", stage: number, character: number,
    vessel: number, persona: number, signature: number,
}
--!overload p4.authorize.on
(operation: "weapon_fusion", callback: (context: P4WeaponFusionContext) -> (boolean, ...string)) -> P4Subscription
--!example
p4.authorize.on("weapon_fusion", function(context)
    local stage: number, who: number, persona: number = context.stage, context.character, context.persona
    return stage == 0 or (who ~= 0 and persona ~= 0 and context.signature == 0), "signature forging denied"
end)
--!refuse
p4.authorize.on("weapon_fusion", function(c) local s: string = c.stage; return true end)
p4.authorize.on("weapon-fusion", function(c) return true end)
