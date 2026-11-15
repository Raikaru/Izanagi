--!capability events.level_up@1
-- p4.events.on("level_up"): the battle result raised a level: the protagonist's own (no persona), or a Persona's.
export type P4LevelUpEvent = {
    name: "level_up", sequence: number,
    character: string?, -- "protagonist" for him and his Personas, else the party member
    persona: P4Row?, -- the Persona that levelled up; nil for the protagonist's own level
    level: number, from: number,
}
--!overload p4.events.on
<T>(name: "level_up", callback: (event: P4LevelUpEvent) -> ()) -> P4Subscription
--!example
p4.events.on("level_up", function(e: P4LevelUpEvent)
    local own: boolean = e.persona == nil and e.character == "protagonist"
    local gained: number = e.level - e.from
end)
--!refuse
p4.events.on("level_up", function(e) local s: string = e.level end)
