--!capability battle.execution@1
-- p4.battle.execution: synchronous pre-effect execution facts, completion/cancellation and base-turn boundaries.
-- Tokens, epochs and action UIDs are opaque lowercase hex strings, never floating-point counters.
export type P4BattleExecutionUnit = {action: number, data: number, uid: number, id: number, side: "party" | "enemy"}
export type P4BattleExecution = {
    token: string, action_uid: string, actor: P4BattleExecutionUnit,
    command: number, skill: number, committed: boolean, base_turn: boolean,
    pending_packets: number, result_packets: number, uncertainty: number,
    direct_end: boolean, end_seen: boolean, sealed: boolean, base_boundary: boolean,
    outcome: "running" | "resolved" | "hitless" | "direct_end" | "removed" | "flushed" | "battle_release" | "replaced" | "unproven",
}
export type P4BattleExecutionEvent = {
    kind: "admitted" | "committed" | "base_boundary" | "terminal" | "uncertain" | "battle_start" | "battle_end" | "lost",
    epoch: string, battle_active: boolean, reason: number,
    execution: P4BattleExecution?, -- absent on lifetime/global loss events
}
--!in p4.battle.execution
-- Registration/re-enabling is outside battle only. Delivery is synchronous:
-- no native game calls/yields or subscription changes; policy state and guest
-- memory remain accessible. Events, executions and actor copies are read-only.
-- An uncaught listener error/budget failure enters native mod recovery; it
-- cannot silently disable observation while the game continues on stale facts.
on: (callback: (event: P4BattleExecutionEvent) -> ()) -> P4Subscription,
-- nil means missing/expired, never successful completion. Does not access a stale actor.
snapshot: (token: string) -> P4BattleExecution?,
current: (action: number) -> P4BattleExecution?,
-- Pure live DatUnit -> instance identity; nil for absent/ambiguous/stale data.
-- Refused in snapshot callbacks; available in synchronous execution observers.
unit: (data: number) -> P4BattleExecutionUnit?,
--!example
local watch: P4Subscription = p4.battle.execution.on(function(event: P4BattleExecutionEvent)
    local execution = event.execution
    if execution and event.kind == "committed" then
        local token: string = execution.token
        local copy: P4BattleExecution? = p4.battle.execution.snapshot(token)
        local unit: P4BattleExecutionUnit? = p4.battle.execution.unit(execution.actor.data)
    end
end)
watch:disable()
--!refuse
p4.battle.execution.on({fn = function(event) end})
p4.battle.execution.snapshot(1)
p4.battle.execution.unit("311")
p4.battle.execution.on(function(event) local token: number = (event.execution :: P4BattleExecution).token end)
