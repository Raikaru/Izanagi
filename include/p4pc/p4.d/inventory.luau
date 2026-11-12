--!capability inventory@1
-- p4.inventory.count and p4.inventory.money: what the player holds now (legal in every callback).
-- An item as actions take it: an ID, a p4.data row, or a name or key the data catalog resolves.
export type P4ItemArgument = number | string | { id: number }
--!in p4.inventory
count: (item: P4ItemArgument) -> number,
money: () -> number,
--!example
local medicine: number = p4.inventory.count("Medicine")
local yen: number = p4.inventory.money()
local same: number = p4.inventory.count(1)
--!refuse
local s: string = p4.inventory.count("Medicine")
p4.inventory.count(true)
