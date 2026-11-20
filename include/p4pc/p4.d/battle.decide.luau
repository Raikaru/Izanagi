--!capability battle.decide@1
-- p4.battle.decide: choose an enemy's command on its own turn: any skill with one legal target, or pass.
-- One unit of the battle, as the deciding enemy sees it (copied; read-only).
export type P4BattleDecideUnit = {
    slot: number, -- index in d.units, also accepted as a target
    side: "party" | "enemy",
    id: number, -- playable character ID or enemy ID
    uid: number,
    alive: boolean,
    hp: number, hp_max: number, sp: number, sp_max: number, hp_fraction: number,
    status: number, -- the ailment word
    charged: boolean, -- Power Charge or Mind Charge is up
}
export type P4BattleDecideContext = {
    actor: P4BattleDecideUnit,
    units: { P4BattleDecideUnit },
    party: { P4BattleDecideUnit },
    enemies: { P4BattleDecideUnit },
    -- What the enemy's own AI chose.
    pick: { kind: "skill" | "pass" | "other", skill: number?, targets: { P4BattleDecideUnit } },
    one_more: boolean, -- this turn is a One More
    extra: boolean, -- this turn is a native extra action
    round: number, -- the protagonist's base turns so far
    -- The game's legal targets of a skill for this actor, and whether it hits all of them.
    candidates: (skill: number) -> ({ P4BattleDecideUnit }, boolean),
}
-- nil keeps the enemy's choice; { pass = true } skips the turn.
export type P4BattleDecision = { skill: number?, target: (P4BattleDecideUnit | number)?, pass: boolean? }
export type P4BattleDecideSpec = {
    side: "enemy"?,
    priority: number?, -- lower runs first; each decider sees the previous one's choice
    fn: (d: P4BattleDecideContext) -> P4BattleDecision?,
}
--!in p4.battle
-- Snapshot mode: fn may read d, call d.candidates and read p4.data; game calls, logging,
-- p4.battle.announce/say and every change raise an error there.
decide: (spec: P4BattleDecideSpec) -> P4Subscription,
--!example
local decider: P4Subscription = p4.battle.decide({ side = "enemy", priority = 10, fn = function(d: P4BattleDecideContext): P4BattleDecision?
    if d.actor.hp_fraction < 0.5 and not d.actor.charged then
        return { skill = 227 } -- self-targeted: no target needed
    end
    local targets: { P4BattleDecideUnit }, everyone: boolean = d.candidates(10)
    local weakest: P4BattleDecideUnit? = nil
    for _, unit in targets do
        if weakest == nil or unit.hp < weakest.hp then weakest = unit end
    end
    if weakest and not everyone then return { skill = 10, target = weakest } end
    if d.pick.kind == "skill" and d.round > 3 then return { pass = true } end
    return nil
end })
decider:disable()
--!refuse
p4.battle.decide(function(d) return nil end)
p4.battle.decide({ fn = function(d) return { skill = "Agi" } end })
p4.battle.decide({ side = "party", fn = function(d) return nil end })
p4.battle.decide({ fn = function(d) local s: string = d.actor.hp; return nil end })
