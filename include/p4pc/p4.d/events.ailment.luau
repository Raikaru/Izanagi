--!capability events.ailment@1
-- p4.events.on("ailment"): an ailment bit was applied to or removed from a unit by a committed hit (blocked ailments are not reported in revision 1).
export type P4AilmentEvent = {
    name: "ailment", sequence: number, source: P4BattleUnitRef, unit: P4BattleUnitRef, skill: P4Row?,
    status: number, -- the one DatUnit ailment bit
    ailment: ("dizzy" | "rage" | "fear" | "silence" | "confusion" | "poison" | "exhaustion" | "enervation")?,
    change: "applied" | "removed",
}
--!overload p4.events.on
<T>(name: "ailment", callback: (event: P4AilmentEvent) -> ()) -> P4Subscription
--!example
p4.events.on("ailment", function(e: P4AilmentEvent)
    if e.ailment == "poison" and e.change == "applied" and e.unit.side == "party" then
        p4.ui.status("Poisoned!", "hud")
    end
end)
--!refuse
p4.events.on("ailment", function(e) local b: boolean = e.change end)
