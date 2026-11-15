--!capability data.catalog@1
-- p4.data: the game's skills, Personas, enemies and items by name, read from the
-- player's own disc and the tables the game has loaded (with every installed mod's
-- changes). docs/modding/systems/data-and-edits.md
-- floor, formation, encounters and treasure need capability data.dungeons.
export type P4DataKind = "skill" | "persona" | "enemy" | "item" | "floor" | "formation" | "encounters" | "treasure"
-- A row by kind, ID, stable key ("p4:skill:agilao", "owner:item:storm-blade") and
-- current display name. Keys come from the unmodded disc names and never change.
export type P4Ref = { kind: P4DataKind, id: number, key: string?, name: string? }
-- The head of a full row; its own `kind` (where it has one) is the row's category.
export type P4Row = { id: number, key: string?, name: string? }
export type P4Stats = { st: number, ma: number, en: number, ag: number, lu: number }
export type P4Affinity = "normal" | "weak" | "resist" | "null" | "repel" | "drain"
-- physical fire ice electric wind almighty light dark, then the ailment columns
-- confusion poison fear rage dizzy exhaustion enervation silence.
export type P4Affinities = { [string]: P4Affinity }
export type P4Skill = P4Row & {
    kind: "physical" | "magic" | "almighty" | "ailment" | "heal" | "support" | "special" | "passive" | "other",
    element: string?,
    passive: boolean,
    power: number?,
    cost: { sp: number?, hp_percent: number? }?,
    accuracy: number?,
    critical: number?,
    targets: ("one_foe" | "all_foes" | "one_ally" | "all_allies" | "self" | "one_fallen_ally"
        | "all_fallen_allies" | "everyone" | "other")?,
    hits: { min: number, max: number }?,
    ailment: { status: string, statuses: { string }, chance: number }?,
    cures: { string }?,
    usable: { field: boolean, battle: boolean }?,
}
export type P4Enemy = P4Row & {
    arcana: string?, level: number, hp: number, sp: number, stats: P4Stats,
    affinities: P4Affinities, rates: { [string]: number }, skills: { P4Ref }, exp: number, yen: number,
    drops: { { item: P4Ref, chance: number } },
    special_drop: { item: P4Ref, flag: number, chance: number }?,
}
export type P4Persona = P4Row & {
    arcana: string?, level: number, stats: P4Stats, affinities: P4Affinities, rates: { [string]: number },
    learnset: { { level: number, skill: P4Ref } },
    character: string?,
}
export type P4Item = P4Row & {
    kind: "weapon" | "armor" | "accessory" | "consumable" | "key" | "material" | "other",
    price: number,
    attack: number?, hit: number?, defense: number?, evasion: number?,
    skill: P4Ref?,
    stats: P4Stats?,
    users: { string }?,
    -- Gear and consumables (capability data.shop).
    shop: P4ItemShop?,
}
export type P4DataFilter = string | {
    name: string?, kind: string?, element: string?, arcana: string?,
    level: (number | { min: number?, max: number? })?,
}
--!in p4.data
-- By display name (case, spaces and punctuation ignored), by key, or by ID
-- (advanced). An unknown name raises an error with the nearest names; a name
-- several rows share raises one listing their keys.
skill: (query: string | number) -> P4Skill,
persona: (query: string | number) -> P4Persona,
enemy: (query: string | number) -> P4Enemy,
item: (query: string | number) -> P4Item,
-- Rows whose name contains the text, or that match every given field.
find: (kind: P4DataKind, filter: P4DataFilter?) -> { P4Ref },
-- Every named row of a kind, in ID order.
all: (kind: P4DataKind) -> { P4Ref },
--!example
local agilao: P4Skill = p4.data.skill("Agilao")
local power: number? = agilao.power
local sp: number? = agilao.cost and agilao.cost.sp
local maya: P4Enemy = p4.data.enemy("p4:enemy:cowardly-maya")
local weak: boolean = maya.affinities.fire == "weak"
local first: P4Ref? = p4.data.persona(1).learnset[1] and p4.data.persona(1).learnset[1].skill
local medicine: P4Item = p4.data.item("Medicine")
local users: { string }? = medicine.users
local fire: { P4Ref } = p4.data.find("skill", { element = "fire", name = "agi" })
local ranked: { P4Ref } = p4.data.find("enemy", { arcana = "Magician", level = { min = 10, max = 20 } })
local everything: { P4Ref } = p4.data.all("item")
--!refuse
local n: number = p4.data.skill("Agi")
p4.data.skill({})
p4.data.find("weapon", "x")
local s: string = p4.data.all("skill")
