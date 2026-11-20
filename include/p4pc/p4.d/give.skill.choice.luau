--!capability give.skill.choice@1
--!covers p4.give.skill
-- p4.give.skill { choice = true, done = fn? }: with all 8 slots full, the game's own panel asks the player which skill to forget, or to keep them all.
--!example
local ok: boolean, why: string? = p4.give.skill("Yosuke", "Garula", { choice = true })
p4.give.skill("protagonist", "Agi", { choice = true, done = function(outcome: P4SkillChoice)
    local learned: boolean = outcome.learned
    local forgot: number? = outcome.forgot
    local reason: string? = outcome.reason
end })
--!refuse
p4.give.skill("Yosuke", "Garula", { choice = "yes" })
p4.give.skill("Yosuke", "Garula", { choice = true, done = 5 })
