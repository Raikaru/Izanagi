--!capability ui.sprite@1
-- p4.ui.sprite: a rectangle of the mod's own power-of-two [resource-image] as a native sprite handle for overlays.
-- A rectangle of this mod's power-of-two [resource-image]; readonly.
export type P4UISprite = { width: number, height: number }
--!in p4.ui
sprite: (image: string, rect: { x: number, y: number, w: number, h: number }) -> (P4UISprite?, string?),
--!example
local badge: P4UISprite?, missing: string? = p4.ui.sprite("badge", {x = 0, y = 0, w = 58, h = 21})
if badge then
    local wide: number = badge.width
    local tall: number = badge.height
end
--!refuse
p4.ui.sprite(7, {x = 0, y = 0, w = 1, h = 1})
p4.ui.sprite("badge", {x = 0, y = 0})
local s: string = p4.ui.sprite("badge", {x = 0, y = 0, w = 1, h = 1})
