--!capability events.revive@1
-- p4.events.on("revive"): a knocked-out unit was revived by a committed effect.
export type P4ReviveEvent = { name: "revive", sequence: number, source: P4BattleUnitRef, unit: P4BattleUnitRef, skill: P4Row? }
--!overload p4.events.on
<T>(name: "revive", callback: (event: P4ReviveEvent) -> ()) -> P4Subscription
--!example
p4.events.on("revive", function(e: P4ReviveEvent) local who: number = e.unit.id end)
--!refuse
p4.events.on("revive", function(e) local s: string = e.unit.id end)
