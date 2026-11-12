--!capability ui.overlay@2
-- p4.ui.overlay: positioned native text and ui.sprite handles in the Camp root (camp), the field HUD layer (hud) or, from revision 2, on every frame (screen).
export type P4UIOverlayItem = {
    key: string, x: number, y: number,
    text: string?, sprite: P4UISprite?, -- exactly one
    align: ("left" | "center" | "right")?, color: number?, alpha: number?, scale: number?,
    plate: boolean?, width: number?, visible: boolean?,
}
export type P4UIOverlaySpec = { context: "camp" | "hud" | "screen", fade: boolean?, items: { P4UIOverlayItem } }
export type P4UIOverlay = {
    set: (self: P4UIOverlay, item: string, text: string) -> boolean,
    show: (self: P4UIOverlay, item: string, visible: boolean) -> (),
    remove: (self: P4UIOverlay) -> (),
}
--!in p4.ui
overlay: (specification: P4UIOverlaySpec) -> (P4UIOverlay?, string?),
--!example
local badge: P4UISprite?, missing: string? = p4.ui.sprite("badge", {x = 0, y = 0, w = 58, h = 21})
if badge then
    local overlay: P4UIOverlay?, full: string? = p4.ui.overlay({context = "camp", fade = true, items = {
        {key = "badge", sprite = badge, x = 24, y = 64},
        {key = "line", text = "CAP 15", x = 26, y = 88, align = "left", color = 0xFFFFFF, plate = true, width = 200},
    }})
    if overlay then
        local changed: boolean = overlay:set("line", "CAP 16")
        overlay:show("badge", false)
        overlay:remove()
    end
end
local marker: P4UIOverlay?, busy: string? = p4.ui.overlay({context = "screen", items = {
    {key = "mark", text = "PRACTICE", x = 620, y = 8, align = "right", plate = true},
}})
--!refuse
p4.ui.overlay({context = "sidebar", items = {}})
p4.ui.overlay({context = "camp", items = {{key = "a", x = "left", y = 0, text = "A"}}})
p4.ui.overlay({context = "hud", items = {{key = "a", x = 0, y = 0, align = "middle", text = "A"}}})
p4.ui.overlay({context = "battle", items = {{key = "a", x = 0, y = 0, text = "A"}}})
