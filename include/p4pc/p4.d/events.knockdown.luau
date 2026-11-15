--!capability events.knockdown@1
-- p4.events.on("knockdown"): a unit was knocked Down by a committed hit.
export type P4KnockdownEvent = { name: "knockdown", sequence: number, source: P4BattleUnitRef, unit: P4BattleUnitRef, skill: P4Row? }
--!overload p4.events.on
<T>(name: "knockdown", callback: (event: P4KnockdownEvent) -> ()) -> P4Subscription
--!example
p4.events.on("knockdown", function(e: P4KnockdownEvent) local down: number = e.unit.uid end)
--!refuse
p4.events.on("knockdown", function(e) local s: string = e.unit.uid end)
