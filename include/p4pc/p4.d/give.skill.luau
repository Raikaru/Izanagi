--!capability give.skill@1
-- p4.give.skill: teach a party member's Persona a skill, in its first empty slot or in place of one it knows.
-- A skill as actions take it: an ID, a p4.data row, or a name or key the data catalog resolves.
export type P4SkillArgument = number | string | { id: number }
-- The outcome of p4.give.skill { choice = true } (give.skill.choice@1), handed to done once.
export type P4SkillChoice = {
    member: number, skill: number, -- as requested, by ID
    learned: boolean,
    forgot: number?, -- the skill forgotten to make room
    reason: string?, -- when not learned: "declined" (the player kept every skill) or why it was not offered
}
--!in p4.give
-- true, true and "queued ..." during a battle, or false and a refusal (already known, all 8 slots
-- full without replace, replace not known, not joined, no Persona). choice (give.skill.choice@1)
-- lets the player pick a skill to forget when all 8 are full; it cannot be combined with replace.
skill: (who: P4PartyWho, skill: P4SkillArgument,
    options: { replace: P4SkillArgument?, choice: boolean?, done: ((outcome: P4SkillChoice) -> ())? }?) -> (boolean, string?),
--!example
local ok: boolean, why: string? = p4.give.skill("Yosuke", "Garu")
p4.give.skill(2, "Garula", { replace = "Garu" })
--!refuse
local n: number = p4.give.skill("Yosuke", "Garu")
p4.give.skill("Yosuke", "Garu", { replace = true })
