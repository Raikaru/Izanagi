--!capability data.dungeons@1
-- Dungeon floors, their shadow encounters, the battles (formations) they pick and their treasure chests, from the loaded ENCOUNT.TBL (docs/modding/systems/data-and-edits.md).
export type P4Formation = P4Row & { enemies: { P4Ref } }
-- Every slot is listed; an empty one has weight 0 and no formation.
export type P4EncounterSlot = { formation: P4Formation?, weight: number }
export type P4EncounterVariant = {
    rates: { normal: number, special: number, rare: number },
    normal: { P4EncounterSlot },
    special: { P4EncounterSlot },
    rare: { P4EncounterSlot },
}
-- alternate: the lists the game uses on the calendar days that select them.
export type P4Encounters = P4Row & P4EncounterVariant & { alternate: P4EncounterVariant }
export type P4Treasure = P4Row & { items: { { item: P4Ref?, weight: number, locked: boolean } } }
export type P4Floor = P4Row & {
    spawn: number,
    refill: number,
    chests: number,
    encounters: P4Encounters?,
    treasure: P4Treasure?,
}
-- In data.luau. Slots by position; {} empties one. Floors that share a group share its edits.
export type P4EncounterSlotEdit = { formation: P4DataInput?, weight: number? }
export type P4EncounterVariantEdit = {
    rates: { normal: number, special: number, rare: number },
    normal: { P4EncounterSlotEdit },
    special: { P4EncounterSlotEdit },
    rare: { P4EncounterSlotEdit },
}
export type P4EncountersEdit = P4EncounterVariantEdit & { id: number, key: string?, alternate: P4EncounterVariantEdit }
export type P4FormationEdit = { id: number, key: string?, enemies: { P4DataInput } }
export type P4TreasureEdit = {
    id: number,
    key: string?,
    items: { { item: P4DataInput?, weight: number?, locked: boolean? } },
}
export type P4FloorEdit = {
    id: number,
    key: string?,
    name: string?,
    spawn: number,
    refill: number,
    chests: number,
    encounters: P4EncountersEdit,
    treasure: P4TreasureEdit,
}
--!in p4.data
-- A floor by its name ("Yukiko's Castle 3F"), key or step index.
floor: (query: string | number) -> P4Floor,
-- Formations and groups by ID or key ("p4:formation:#17").
formation: (query: string | number) -> P4Formation,
encounters: (query: string | number) -> P4Encounters,
treasure: (query: string | number) -> P4Treasure,
--!example
local floor: P4Floor = p4.data.floor("Yukiko's Castle 3F")
local spawn: number = floor.spawn
local group = floor.encounters
local first = group and group.normal[1]
local enemies = first and first.formation and first.formation.enemies
p4.data.edit(function(d)
    local castle = d.floor("Yukiko's Castle 3F")
    castle.spawn = 6
    castle.encounters.rates.rare = 5
    castle.encounters.normal[1] = { formation = 17, weight = 50 }
    castle.treasure.items[1] = { item = "Medicine", weight = 10 }
    d.formation(17).enemies[1] = "Lying Hablerie"
end)
--!refuse
p4.data.edit(function(d) d.floor("Yukiko's Castle 3F").spawn = "many" end)
