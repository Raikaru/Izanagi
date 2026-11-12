--!capability events.turn_start@1
-- p4.events.on("turn_start"): a battle unit's turn begins; follow_up marks a follow-up attack (a One More is an ordinary turn in revision 1).
export type P4TurnStartEvent = { name: "turn_start", sequence: number, unit: P4BattleUnitRef, follow_up: boolean }
--!overload p4.events.on
<T>(name: "turn_start", callback: (event: P4TurnStartEvent) -> ()) -> P4Subscription
--!example
p4.events.on("turn_start", function(e: P4TurnStartEvent)
    local enemy: boolean = e.unit.side == "enemy" and not e.follow_up
end)
--!refuse
p4.events.on("turn_start", function(e) local s: string = e.follow_up end)
