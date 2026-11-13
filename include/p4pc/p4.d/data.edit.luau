--!capability data.edit@1
-- data.luau and p4.data.edit: typed changes to skills, Personas, enemies and items, applied while the game builds its tables (docs/modding/systems/data-and-edits.md).
-- A row given by name, key or ID; a ref or a full row from p4.data works too.
export type P4DataInput = string | number | P4Ref | P4Row
export type P4SkillEdit = {
    id: number, key: string?, name: string?,
    power: number?, cost: { sp: number?, hp_percent: number? }?, accuracy: number?, critical: number?,
    hits: { min: number?, max: number? }?, element: string?,
    ailment: { status: string?, chance: number? }?,
}
export type P4EnemyEdit = {
    id: number, key: string?, name: string?,
    level: number, hp: number, sp: number, exp: number, yen: number,
    stats: { st: number?, ma: number?, en: number?, ag: number?, lu: number? },
    -- "normal" | "weak" | "resist" | "null" | "repel" | "drain", or a rate in percent
    affinities: { [string]: string | number },
    skills: { [number]: P4DataInput? },
    drops: { [number]: { item: P4DataInput?, chance: number? } },
}
export type P4LearnsetEdit = { { level: number, skill: P4Ref } } & {
    add: (self: P4LearnsetEdit, event: { level: number, skill: P4DataInput }) -> (),
    remove: (self: P4LearnsetEdit, skill: P4DataInput) -> (),
    move: (self: P4LearnsetEdit, skill: P4DataInput, level: number) -> (),
    replace: (self: P4LearnsetEdit, skill: P4DataInput, with: P4DataInput) -> (),
}
export type P4PersonaEdit = {
    id: number, key: string?, name: string?,
    level: number,
    stats: { st: number?, ma: number?, en: number?, ag: number?, lu: number? },
    affinities: { [string]: string | number },
    learnset: P4LearnsetEdit,
}
export type P4ItemEdit = {
    -- name: a retail item's new display name in menus, shops and messages (capability data.item_names).
    id: number, key: string?, name: string?,
    price: number, attack: number?, hit: number?, defense: number?, evasion: number?, skill: P4DataInput?,
    shop: P4ItemShopEdit, -- capability data.shop
}
export type P4DataEdit = {
    skill: (query: P4DataInput) -> P4SkillEdit,
    persona: (query: P4DataInput) -> P4PersonaEdit,
    enemy: (query: P4DataInput) -> P4EnemyEdit,
    item: (query: P4DataInput) -> P4ItemEdit,
    -- Capability data.dungeons.
    floor: (query: P4DataInput) -> P4FloorEdit,
    formation: (query: P4DataInput) -> P4FormationEdit,
    encounters: (query: P4DataInput) -> P4EncountersEdit,
    treasure: (query: P4DataInput) -> P4TreasureEdit,
}
-- One field two or more mods' layers assigned; the last owner won.
export type P4DataConflict = { kind: P4DataKind, id: number, key: string?, field: string, owners: { string }, winner: string }
--!in p4.data
-- In data.luau: runs fn now with views of the tables as the mods before this one left them; assignments
-- change this mod's layer. In main.luau it raises an error saying to move the call to data.luau.
edit: (fn: (d: P4DataEdit) -> ()) -> (),
-- Every field that two or more enabled mods' data layers assigned this boot.
conflicts: () -> { P4DataConflict },
--!example
p4.data.edit(function(d)
    local agilao = d.skill("Agilao")
    agilao.power = 100
    agilao.cost = { sp = 3 }
    local hablerie = d.enemy("p4:enemy:lying-hablerie")
    hablerie.hp = 300
    hablerie.affinities.fire = "weak"
    hablerie.drops[1] = { item = "Medicine", chance = 50 }
    hablerie.skills[2] = p4.data.skill("Bufu")
    d.persona("Tomoe").learnset:add({ level = 12, skill = "Bufula" })
    d.item("Medicine").price = 100
end)
for _, c in p4.data.conflicts() do
    local winner: string = c.winner
    local field: string = c.field
end
--!refuse
p4.data.edit("Agilao")
local n: number = p4.data.conflicts()
p4.data.edit(function(d) d.persona("Tomoe").learnset:add("Bufula") end)
