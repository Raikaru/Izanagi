--!strict
-- Authored mod-test input for p4.ui.native_row (capability ui.native_row@1):
-- one Velvet fusion card from an authored 256x128 atlas (tools/ui_art/ui_art.py:
-- the card symbol and "Witness / Card" labels in the Velvet card families) that
-- opens a small detail page. Markers are callback evidence;
-- tests/native_rows_velvet_gate.sh captures the card list, the hosted page and
-- the list it returns to.
assert(p4.has("ui.sprite@1") and p4.has("ui.native_row@1"), "this build lacks ui.sprite@1 or ui.native_row@1")
local page = p4.ui.detail({
    key = "card", title = "WITNESS CARD",
    detail = function(): string return "Opened from a native Velvet card." end,
    opened = function() p4.log("ROW_PAGE_OPEN") end,
    closed = function() p4.log("ROW_PAGE_CLOSE") end,
})
local function part(x: number, y: number, w: number, h: number): P4UISprite
    local sprite, missing = p4.ui.sprite("card", {x = x, y = y, w = w, h = h})
    assert(sprite, missing)
    return sprite
end
local card, refused = p4.ui.native_row({
    host = "velvet_fusion", key = "witness", page = page,
    symbol = part(0, 0, 72, 77), label = part(80, 0, 99, 31), small = part(80, 40, 72, 18),
})
assert(card, refused)
p4.log("ROW_READY")
