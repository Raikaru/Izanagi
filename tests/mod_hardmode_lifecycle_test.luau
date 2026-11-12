-- Headless check of mods/p4pc.hardmode's run lifecycle (the third chunk run by
-- luau_script_runner, after tests/mod_hardmode_fixture.luau and main.luau).
-- Drives the save block main.luau registered the way the game does: a reset at
-- boot and at the start of every LOAD GAME, then the load. Emits one line per
-- fact for tests/mod_hardmode_lifecycle_test.py.
local F = FIXTURE
local save = F.save
emit("READY " .. tostring(save ~= nil and F.logs[#F.logs] == "HARDMODE_READY"))

local function refused(): number
    local n = 0
    for _, line in F.logs do
        if string.find(line, "^HARDMODE_BADGE refused") then n += 1 end
    end
    return n
end

-- Boot: no run, no badge.
save.load(nil, "reset")
emit("BOOT_BADGE " .. tostring(F.camp ~= nil))

-- LOAD GAME of a save without a hard-mode block, with START FROM THE NEXT
-- LOADED SAVE on: the run is adopted, the badge registered, the setting cleared.
save.load(nil, "reset")
save.load(nil, "load")
local value = save.snapshot()
emit(string.format("ADOPTED %s %s %s", tostring(F.camp ~= nil), tostring(value ~= nil),
    tostring(F.settings.adopt_on_load)))

-- The same save loaded again and again in one session, past the shared sprite
-- pool: each LOAD GAME removes the badge at its reset and registers it again at
-- the load, from the one sprite made at the first.
local CYCLES = 40
local held = true
for _ = 1, CYCLES do
    save.load(nil, "reset")
    if F.camp ~= nil then held = false end
    local allowed = save.authorize_load(value)
    if not allowed then held = false end
    save.load(value, "load")
    if F.camp == nil then held = false end
end
emit(string.format("BADGE_CYCLES %d %s", CYCLES, tostring(held)))

-- A load of a save without a block (and nothing to adopt) ends the run.
save.load(nil, "reset")
save.load(nil, "load")
emit("OFF_AFTER_PLAIN_LOAD " .. tostring(F.camp == nil))
emit("SPRITES " .. tostring(F.sprites))
emit("REFUSED " .. tostring(refused()))
