--!capability events.one_more@1
-- p4.events.on("one_more"): a unit earned a One More (the extra turn), when the game adds it to its action.
export type P4OneMoreEvent = {
    name: "one_more", sequence: number, unit: P4BattleUnitRef,
    cause: "weakness" | "critical" | "down", -- the action struck a weakness, else landed a critical, else knocked down
}
--!overload p4.events.on
<T>(name: "one_more", callback: (event: P4OneMoreEvent) -> ()) -> P4Subscription
--!example
p4.events.on("one_more", function(e: P4OneMoreEvent)
    local party: boolean = e.unit.side == "party" and e.cause == "weakness"
end)
--!refuse
p4.events.on("one_more", function(e) local n: number = e.cause end)
