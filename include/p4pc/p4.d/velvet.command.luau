--!capability velvet.command@1
-- p4.velvet.command: one mod command in the Velvet Room, opened with Square from the idle root menu into the mod's hosted page.
--!in p4.velvet
-- One mod command in the Velvet Room: Square on the idle root opens page
-- (a page handle or its global key); label is RAW text; with operation the
-- command is offered only when that authorizer allows it.
command: (specification: {page: P4UIPage | string, label: string, operation: "weapon_fusion"?}) -> P4Subscription,
--!example
local forgeRow: P4Subscription = p4.velvet.command({page = "fusion.forge", label = "WEAPON FUSION", operation = "weapon_fusion"})
p4.velvet.command({page = "fusion.forge", label = "WEAPON FUSION"}):disable()
--!refuse
p4.velvet.command({label = "WEAPON FUSION"})
p4.velvet.command({page = "fusion.forge", label = "WEAPON FUSION", operation = "fusion"})
p4.velvet.command({page = "fusion.forge", label = 7})
