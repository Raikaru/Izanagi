--!capability persona.stock@1
-- p4.persona.stock(): read-only copies of Yu's carried Personas in roster slot order, legal in every callback mode.
export type P4StockEntry = {
    slot: number, lead: boolean, persona: number, arcana: number,
    level: number, exp: number,
    stats: {number}, -- St Ma En Ag Lu
    skills: {number}, -- learned skill IDs in slot order
    registered: boolean, registered_level: number,
}
--!in p4.persona
-- Yu's Persona stock in slot order: read-only copies, legal in every
-- callback mode.
stock: () -> {P4StockEntry},
--!example
for _, carried in p4.persona.stock() do
    local id: number, lead: boolean, saved: boolean = carried.persona, carried.lead, carried.registered
    local st: number, first: number? = carried.stats[1], carried.skills[1]
end
--!refuse
local s: string = p4.persona.stock()[1].persona
p4.persona.stock(1)
