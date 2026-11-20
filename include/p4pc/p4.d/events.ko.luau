--!capability events.ko@1
-- p4.events.on("ko"): a unit was knocked out by a committed hit.
export type P4KoEvent = { name: "ko", sequence: number, source: P4BattleUnitRef, unit: P4BattleUnitRef, skill: P4Row? }
--!overload p4.events.on
<T>(name: "ko", callback: (event: P4KoEvent) -> ()) -> P4Subscription
--!example
p4.events.on("ko", function(e: P4KoEvent) local boss: boolean = e.unit.side == "enemy" end)
--!refuse
p4.events.on("ko", function(e) local n: number = e.unit.side end)
