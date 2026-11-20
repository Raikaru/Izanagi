--!capability events.skill_learned@1
-- p4.events.on("skill_learned"): a Persona learned a skill on a battle result's level-up.
export type P4SkillLearnedEvent = {
    name: "skill_learned", sequence: number, character: string?, persona: P4Row, skill: P4Row,
}
--!overload p4.events.on
<T>(name: "skill_learned", callback: (event: P4SkillLearnedEvent) -> ()) -> P4Subscription
--!example
p4.events.on("skill_learned", function(e: P4SkillLearnedEvent)
    local label: string = e.skill.name or tostring(e.skill.id)
end)
--!refuse
p4.events.on("skill_learned", function(e) local n: number = e.persona end)
