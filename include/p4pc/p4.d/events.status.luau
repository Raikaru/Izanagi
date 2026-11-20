--!capability events.status@1
-- p4.events.on("status"): a side ailment (status@1) landed, was cleared or cost a turn (docs/modding/lua.md "Side ailments").
-- kind: 1 landed, 2 cleared, 3 turn lost. reason: 0 none, 1 rider, 2 cure, 3 mod, 4 recovered,
-- 5 defeat, 6 battle end, 7 reset, 8 broken, 9 transfer. unit is a guest DatUnit address.
export type P4StatusEvent = { name: "status", unit: number, leaf: number, kind: number, reason: number }
--!overload p4.events.on
<T>(name: "status", callback: (event: P4StatusEvent) -> ()) -> P4Subscription
--!example
p4.events.on("status", function(e: P4StatusEvent)
    local lost: boolean = e.kind == 3
    local key: string? = p4.status.key(e.leaf)
end)
--!refuse
p4.events.on("status", function(e) local s: string = e.kind end)
