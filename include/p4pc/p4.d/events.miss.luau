--!capability events.miss@1
-- p4.events.on("miss"): a hit row that missed or was evaded.
export type P4MissEvent = { name: "miss", sequence: number, source: P4BattleUnitRef, target: P4BattleUnitRef, skill: P4Row? }
--!overload p4.events.on
<T>(name: "miss", callback: (event: P4MissEvent) -> ()) -> P4Subscription
--!example
p4.events.on("miss", function(e: P4MissEvent) local who: number = e.target.id end)
--!refuse
p4.events.on("miss", function(e) local n: number = e.target end)
