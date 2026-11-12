--!capability equipment@1
-- p4.equipment: read a party member's equipment and exchange it the way the Camp equipment menu does, applied at the next frame safe point outside battle.
-- An exchange puts one held item on a joined party member; the item it
-- replaces returns to the items. Slots: 0 weapon, 1 armor, 2 accessory.
export type P4EquipRequest = {character: number, slot: number, item: number}
export type P4EquipResult = {
    outcome: "equipped" | "refused", reason: string?,
    character: number, slot: number, item: number, previous: number,
}
--!in p4.equipment
-- get reads the item in a slot (0: none) in every callback. check validates
-- a request now and returns the item it would replace (0: none), or nil and
-- the reason. equip validates and queues; the next frame safe point outside
-- battle revalidates and equips, or refuses whole if the state changed.
-- result takes the outcome once. check and equip are refused in synchronous
-- callbacks and snapshots.
get: (character: number, slot: number) -> number,
check: (request: P4EquipRequest) -> (number?, string?),
equip: (request: P4EquipRequest) -> (boolean, string?),
result: () -> P4EquipResult?,
pending: () -> boolean,
--!example
local weapon: number = p4.equipment.get(1, 0)
local replaces: number?, why: string? = p4.equipment.check({character = 1, slot = 0, item = 2048})
local queued: boolean, refusal: string? = p4.equipment.equip({character = 1, slot = 0, item = 2048})
local done = p4.equipment.result()
if done and done.outcome == "equipped" then local old: number = done.previous end
local waiting: boolean = p4.equipment.pending()
--!refuse
p4.equipment.equip({character = 1, item = 2048})
p4.equipment.equip({character = "yu", slot = 0, item = 2048})
local o: "worn" = p4.equipment.result().outcome
local s: string = p4.equipment.get(1, 0)
