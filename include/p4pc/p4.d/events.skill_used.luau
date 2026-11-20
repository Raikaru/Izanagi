--!capability events.skill_used@1
-- p4.events.on("skill_used"): an attack, skill or item command leaves READY for execution (not proof it hit).
export type P4SkillUsedEvent = {
    name: "skill_used", sequence: number, unit: P4BattleUnitRef,
    kind: "attack" | "skill" | "item", skill: P4Row, item: P4Row?,
}
--!overload p4.events.on
<T>(name: "skill_used", callback: (event: P4SkillUsedEvent) -> ()) -> P4Subscription
--!example
p4.events.on("skill_used", function(e: P4SkillUsedEvent)
    local label: string = e.skill.name or tostring(e.skill.id)
end)
--!refuse
p4.events.on("skill_used", function(e) local n: number = e.kind end)
