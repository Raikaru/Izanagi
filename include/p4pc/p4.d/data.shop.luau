--!capability data.shop@1
--!covers p4.data.item p4.data.edit
-- Shops through item rows: retail has no per-shop stock table. The weapon store lists gear once its unlock gates pass; the item store lists consumables from their date (docs/modding/systems/data-and-edits.md).
export type P4ItemUnlock = { material: P4Ref, count: number }
export type P4ItemShop = {
    store: "weapon" | "item",
    -- false: retail never lists it (the 3/31 date, or the Blank gate on gear).
    sold: boolean,
    -- Consumables: the first day the item store sells it.
    from: { month: number, day: number }?,
    -- Gear: how many of each material the weapon store must have bought first.
    unlock: { P4ItemUnlock }?,
}
-- In data.luau: sold = true lists it (a consumable from 4/1, gear without the Blank gate); {} clears unlock.
export type P4ItemShopEdit = {
    sold: boolean,
    from: { month: number, day: number },
    unlock: { { material: P4DataInput, count: number } },
}
--!example
local medicine: P4Item = p4.data.item("Medicine")
local shop = medicine.shop
local sold: boolean = shop ~= nil and shop.sold
p4.data.edit(function(d)
    d.item("Medicine").shop.from = { month = 4, day = 20 }
    d.item("5-Iron").shop.unlock = { { material = "Supple Metal", count = 3 } }
    d.item("Golf Club").shop.sold = true
end)
--!refuse
p4.data.edit(function(d) d.item("Medicine").shop.from = "4/20" end)
