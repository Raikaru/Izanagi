--!capability party.persona@1
-- p4.party.persona and p4.party.level: a party member's Persona (ID, level, EXP, skills) and level, as they are now.
-- A party member: 1-8, a character name ("Yosuke", "protagonist"), or a unit table with a character field.
export type P4PartyWho = number | string | { character: string? }
-- Member 1 is the protagonist's current Persona; 2-8 the party members' (their level is their Persona's).
export type P4PartyPersona = {
    id: number, key: string?, name: string?, -- the Persona as p4.data names it
    level: number, exp: number,
    skills: { number }, -- skill IDs in slot order, at most 8
}
--!in p4.party
-- nil and a reason when no game is loaded, the member has not joined or has no Persona yet.
persona: (who: P4PartyWho) -> (P4PartyPersona?, string?),
-- The protagonist's own level, or a member's Persona's.
level: (who: P4PartyWho) -> (number?, string?),
--!example
local yosuke: P4PartyPersona? = p4.party.persona("Yosuke")
local level: number? = p4.party.level("protagonist")
if yosuke then
    local first: number? = yosuke.skills[1]
end
--!refuse
local p: P4PartyPersona = p4.party.persona(2)
p4.party.level(true)
