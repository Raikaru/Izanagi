--!strict
-- Authored mod-test input for p4.ui.sprite / p4.ui.overlay (capabilities
-- ui.sprite@1, ui.overlay@2): a Camp badge from an authored power-of-two atlas
-- (tools/ui_art/ui_art.py "badge-witness", Camp label family), a Camp line on
-- the native plate, a right-aligned HUD line and a screen marker. Markers are
-- callback evidence only; the Camp and field captures are the rendering receipts.
assert(p4.has("ui.sprite@1") and p4.has("ui.overlay@2"), "this build lacks ui.sprite@1 or ui.overlay@2")
local badge, missing = p4.ui.sprite("badge", {x = 0, y = 0, w = 91, h = 21})
assert(badge, missing)
local camp, full = p4.ui.overlay({context = "camp", fade = true, items = {
    {key = "badge", sprite = badge, x = 22, y = 64},
    {key = "line", text = "OVERLAY WITNESS 0", x = 30, y = 92, plate = true, width = 140},
}})
assert(camp, full)
local hud, refused = p4.ui.overlay({context = "hud", items = {
    {key = "line", text = "HUD OVERLAY", x = 620, y = 24, align = "right", color = 0xFFE040},
}})
assert(hud, refused)
-- ui.overlay@2: an always-visible screen marker, drawn over the field, Camp and
-- the battle command menu alike; beside it a status HUD row of the same layer.
local screen, busy = p4.ui.overlay({context = "screen", items = {
    {key = "mark", text = "SCREEN MARK", x = 620, y = 52, align = "right", plate = true},
}})
assert(screen, busy)
p4.ui.status("STATUS ROW", "hud")
p4.log(string.format("UI_OVERLAY_READY badge=%dx%d", badge.width, badge.height))

local opened = 0
p4.events.on("menu_open", function(event)
    if event.kind ~= 0 then return end
    opened += 1
    local changed = camp:set("line", "OVERLAY WITNESS " .. tostring(opened))
    p4.log("UI_OVERLAY_CAMP " .. tostring(opened) .. " " .. tostring(changed))
end)
