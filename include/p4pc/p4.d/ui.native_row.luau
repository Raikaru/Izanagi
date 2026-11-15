--!capability ui.native_row@1
-- p4.ui.native_row: a mod row drawn by a native menu that opens a hosted page; the Velvet fusion card list takes one card.
export type P4UINativeRowSpec = {
    host: "velvet_fusion", key: string, page: P4UIPage,
    symbol: P4UISprite, label: P4UISprite, small: P4UISprite,
}
export type P4UINativeRow = {
    enable: (self: P4UINativeRow, enabled: boolean) -> boolean,
    present: (self: P4UINativeRow) -> boolean,
    remove: (self: P4UINativeRow) -> boolean,
}
--!in p4.ui
native_row: (specification: P4UINativeRowSpec) -> (P4UINativeRow?, string?),
--!example
local page: P4UIPage = p4.ui.detail({key = "card", title = "CARD", detail = function(): string return "A native card." end})
local badge: P4UISprite?, missing: string? = p4.ui.sprite("card", {x = 0, y = 0, w = 72, h = 77})
if badge then
    local card: P4UINativeRow?, refused: string? = p4.ui.native_row({
        host = "velvet_fusion", key = "card", page = page, symbol = badge, label = badge, small = badge,
    })
    if card then
        local shown: boolean = card:enable(false)
        local listed: boolean = card:present()
        card:remove()
    end
end
--!refuse
local s: P4UISprite = (nil :: any); p4.ui.native_row({host = "camp_root", key = "k", page = (nil :: any), symbol = s, label = s, small = s})
p4.ui.native_row({host = "velvet_fusion", key = "k", page = "lua/x/page"})
