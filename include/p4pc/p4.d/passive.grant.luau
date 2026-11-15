--!capability passive.grant@1
--!covers p4.passive
-- p4.passive: give a party member, a Persona, an equipment item or an enemy kind a passive skill without a skill slot, and take it back (docs/modding/lua.md "Passive grants").
-- Who a grant reaches. party: that member, whatever Persona they have. persona: whoever has that Persona now, and
-- that Persona's own checks in the protagonist's stock (Growth, Camp skill costs). item: whichever member has it
-- equipped. enemy: every enemy of that kind. The protagonist's own and his equipment's grants also reach his
-- stocked Personas.
export type P4PassiveCharacter = "protagonist" | "yosuke" | "chie" | "yukiko" | "rise" | "kanji" | "naoto" | "teddie"
export type P4PassiveTarget = { party: number | P4PassiveCharacter }
    | { persona: number | P4Row }
    | { item: number | P4Row }
    | { enemy: number | P4Row }
export type P4PassiveKind = "party" | "persona" | "item" | "enemy"
-- One held grant: its target (party members by name), the same as kind and numeric id (party members 1..8), the
-- skill ID and the mod (its p4.id) holding it.
export type P4PassiveGrant = { target: P4PassiveTarget, kind: P4PassiveKind, id: number, skill: number, owner: string }
--!in p4
passive: {
    -- Grants a passive skill (an ID or a p4.data skill row): true when this mod did not hold it already. Once the
    -- catalog is loaded a Persona, item or enemy must be one of its rows. The game's next check sees it everywhere it
    -- asks whether the target has that passive: battle formulas, the enemy AI, Endure and Light/Dark survival, max
    -- HP/SP, skill costs, Growth, the field. The skill list is unchanged. A mutation: refused in snapshot callbacks.
    -- Raises when the grant table (256 grants for all mods together) is full. Grants are not saved: they outlive
    -- loads and New Game and end when revoked or with the mod's script.
    grant: (target: P4PassiveTarget, skill: number | P4Row) -> boolean,
    -- Takes back this mod's grant: true when it held one. Another mod's grant of the same skill stays.
    revoke: (target: P4PassiveTarget, skill: number | P4Row) -> boolean,
    -- Takes back every grant of this mod; returns how many.
    clear: () -> number,
    -- Whether any mod grants the skill to exactly that target (not the unit's own slots).
    granted: (target: P4PassiveTarget, skill: number | P4Row) -> boolean,
    -- Every held grant, ordered by target kind (party, persona, item, enemy), target, skill and mod load order.
    list: () -> { P4PassiveGrant },
},
--!example
local counter: number = p4.data.skill("Counter").id
local added: boolean = p4.passive.grant({ party = "yosuke" }, counter)
p4.passive.grant({ persona = p4.data.persona("Izanagi") } :: P4PassiveTarget, p4.data.skill("Arms Master"))
local accessories: { P4Ref } = p4.data.find("item", { kind = "accessory" })
p4.passive.grant({ item = accessories[1].id } :: P4PassiveTarget, counter)
p4.passive.grant({ enemy = p4.data.enemy("Cowardly Maya") } :: P4PassiveTarget, counter)
local held: boolean = p4.passive.granted({ party = 2 } :: P4PassiveTarget, counter)
local removed: boolean = p4.passive.revoke({ party = "yosuke" }, counter)
for _, grant: P4PassiveGrant in p4.passive.list() do
    if grant.owner == p4.id and grant.kind == "party" and grant.id == 2 then
        p4.log("this mod grants Yosuke skill " .. grant.skill)
    end
end
local cleared: number = p4.passive.clear()
-- Grants that depend on the save are dropped and made again whenever a save is loaded or a New Game starts.
p4.save.register({
    schema = 1, max_bytes = 64,
    snapshot = function(): P4SaveValue return nil end,
    load = function(value: P4SaveValue, origin: "load" | "reset")
        p4.passive.clear()
        if origin == "load" then p4.passive.grant({ party = "yosuke" }, counter) end
    end,
})
--!refuse
p4.passive.grant({ party = 1 }, "Counter")
p4.passive.grant("yosuke", 0x1F6)
local n: number = p4.passive.grant({ enemy = 40 }, 0x1F6)
p4.passive.grant({ party = "marie" }, 0x1F6)
local k: number = p4.passive.list()[1].kind
