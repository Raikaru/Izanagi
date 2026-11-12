--!capability events.damage@1
-- p4.events.on("damage"): one landed, nulled or drained hit row on a unit, per target and hit in commit order.
-- A battle unit as an event names it: party members their character ("yosuke"),
-- enemies the catalog's key and current name.
export type P4BattleUnitRef = { side: "party" | "enemy", id: number, uid: number, character: string?, key: string?, name: string? }
export type P4DamageEvent = {
    name: "damage", sequence: number, source: P4BattleUnitRef, target: P4BattleUnitRef, skill: P4Row?,
    amount: number, -- HP lost; negative when the hit was drained
    hp: number, -- the target's HP after the hit
    result: "hit" | "weak" | "critical" | "null" | "drain",
    reflected: boolean, -- repelled back at the attacker (source and target are the attacker)
    endured: boolean, -- the target endured at 1 HP
}
--!overload p4.events.on
<T>(name: "damage", callback: (event: P4DamageEvent) -> ()) -> P4Subscription
--!example
local total = 0
p4.events.on("damage", function(e: P4DamageEvent)
    if e.target.side == "enemy" and e.result == "weak" then total += e.amount end
end)
--!refuse
p4.events.on("damage", function(e) local s: string = e.amount end)
p4.events.on("damage", function(e) local r: "miss" = e.result end)
