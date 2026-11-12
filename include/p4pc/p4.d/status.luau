--!capability status@1
-- p4.status side ailments: the per-mod gate, protection holds and the held ailment by status-ailment key (docs/modding/lua.md "Side ailments"). p4.status.wearoff is status.wearoff@1's.
--!in p4.status
-- units are live battle DatUnits (else an error); refusals are (false, reason). enable/protect
-- holds are this mod's own and end with its VM (protect also with the battle).
enable: (on: boolean) -> (boolean, string?),
protect: (unit: number, on: boolean) -> (boolean, string?),
protected: (unit: number) -> boolean,
query: (unit: number) -> string?,
landings: (unit: number, key: string) -> number,
key: (leaf: number) -> string?,
set: (unit: number, key: string) -> (boolean, string?),
clear: (unit: number) -> string?,
--!example
local gated: boolean, why: string? = p4.status.enable(true)
local name: string? = p4.status.key(24)
local held: string? = p4.status.query(0x01A2B3C0)
local times: number = p4.status.landings(0x01A2B3C0, "x.mod:freeze")
if held == nil and not p4.status.protected(0x01A2B3C0) then
    local landed: boolean, refused: string? = p4.status.set(0x01A2B3C0, "x.mod:freeze")
end
local cleared: string? = p4.status.clear(0x01A2B3C0)
local guarded: boolean = p4.status.protect(0x01A2B3C0, true)
--!refuse
p4.status.enable(1)
p4.status.set(0x1000, 24)
local s: string = p4.status.query(0x1000)
p4.status.wearoff.on({kind = "add", fn = function(roll) return p4.status.key(24) end})
