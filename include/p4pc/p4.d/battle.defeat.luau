--!capability battle.defeat@1
-- p4.battle.defeat: the one handler that answers a lost field battle before its Game Over: let it run, hold it, or retry the battle from its start.
-- source: "field" for a field script's encounter (story bosses), "collision" for a shadow touched on the field.
-- polls: earlier calls for this defeat. allowed/reason: whether the "retry" authorizers (and a recorded
-- battle start) allow a retry. party: the fighters' battle-start HP, SP and ailment word, which a retry restores.
export type P4DefeatMember = { id: number, hp: number, sp: number, ailments: number }
export type P4DefeatContext = {
    name: "defeat", encounter: number, source: "field" | "collision", polls: number,
    allowed: boolean, reason: string, party: { P4DefeatMember },
}
--!in p4.battle
-- nil lets the Game Over run now; "hold" asks again next frame (at most 18000 times); "retry" restores the battle start and fights it again.
defeat: (callback: (context: P4DefeatContext) -> ("retry" | "hold")?) -> P4Subscription,
--!example
local handler: P4Subscription = p4.battle.defeat(function(context: P4DefeatContext): ("retry" | "hold")?
    if context.allowed and context.source == "field" then
        return if context.polls < 60 then "hold" else "retry"
    end
    return nil
end)
--!refuse
p4.battle.defeat("retry")
p4.battle.defeat(function(context: P4DefeatContext): number return context.polls end)
