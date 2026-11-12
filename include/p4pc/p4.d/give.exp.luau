--!capability give.exp@1
-- p4.give.exp: add EXP with the game's own level-ups: to a party member's Persona, or the protagonist's own level.
--!in p4.give
-- 1-9,999,999 EXP. For the protagonist, his own level unless persona = true (then his current
-- Persona's). A level-up teaches the Persona's skills; one that does not fit (8 already) is not
-- learned and is logged. true, true and "queued ..." during a battle, or false and a refusal.
exp: (who: P4PartyWho, amount: number, options: { persona: boolean? }?) -> (boolean, string?),
--!example
local ok: boolean, why: string? = p4.give.exp("Yosuke", 500)
p4.give.exp("protagonist", 1000)
p4.give.exp("protagonist", 1000, { persona = true })
--!refuse
local n: number = p4.give.exp("Yosuke", 500)
p4.give.exp("Yosuke", "lots")
