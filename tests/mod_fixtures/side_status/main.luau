--!strict
-- Authored mod-test input for p4.status (capability status@1) in a real battle, run as a
-- declared battle-sim mod cohort. The gate is enabled before the battle; once
-- the battle's units exist the fixture lands its own side ailment on the first
-- enemy, checks the refusals and misuse, and logs every status event (the
-- landing, the lost turn, the recovery). Not shipped; not a retail asset.
local KEY = "witness.side-status:numb-ailment"
local BATTLE_STATE = 0x0076449C -- gp slot 0xb3ac: the live battle state
local landed: number? = nil
local fighting = false

local function log(text: string) p4.log("SIDESTATUS " .. text) end

local function enemies(): {number}
    local out = {}
    local state = p4.mem.read("u32", BATTLE_STATE)
    if state == 0 then return out end
    local action = p4.mem.read("u32", state + 0x174)
    local n = 0
    while action ~= 0 and n < 64 do
        local unit = p4.mem.read("u32", action + 0x30)
        if unit ~= 0 then
            local dat = p4.mem.read("u32", unit + 0xa64)
            if dat ~= 0 and bit32.band(p4.mem.read("u16", dat), 4) ~= 0 then table.insert(out, dat) end
        end
        action = p4.mem.read("u32", action + 0x450)
        n += 1
    end
    return out
end

local enabled, why = p4.status.enable(true)
log("enable ok=" .. tostring(enabled) .. " why=" .. tostring(why))

p4.events.on("status", function(e: P4StatusEvent)
    log(string.format("EVENT kind=%d unit=%x key=%s reason=%d held=%s", e.kind, e.unit,
        tostring(p4.status.key(e.leaf)), e.reason, tostring(p4.status.query(e.unit))))
end)

p4.events.on("battle_start", function()
    landed = nil
    fighting = true
    log("battle_start")
end)

p4.events.on("frame_start", function()
    if not fighting or landed ~= nil then return end
    local list = enemies()
    if #list == 0 then return end
    local target = list[1]
    local ok, reason = p4.status.set(target, KEY)
    if not ok then return end -- the battle is not running its units yet
    landed = target
    local again, again_why = p4.status.set(target, KEY)
    local off, off_why = p4.status.enable(false)
    log(string.format("LANDED unit=%x query=%s landings=%d again=%s/%s disable=%s/%s misuse=%s",
        target, tostring(p4.status.query(target)), p4.status.landings(target, KEY), tostring(again),
        tostring(again_why), tostring(off), tostring(off_why), tostring(pcall(p4.status.set, 0x1234, KEY))))
    if #list > 1 then
        local guarded = p4.status.protect(list[2], true)
        local refused, refused_why = p4.status.set(list[2], KEY)
        log(string.format("PROTECT unit=%x ok=%s set=%s/%s", list[2], tostring(guarded), tostring(refused),
            tostring(refused_why)))
    end
end)

p4.events.on("battle_end", function(e: P4BattleEndEvent)
    fighting = false
    log("battle_end outcome=" .. e.outcome .. " held=" .. tostring(if landed then p4.status.query(landed) else nil))
end)
