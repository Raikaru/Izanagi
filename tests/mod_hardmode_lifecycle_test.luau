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
emit("BOOT_FREEZESHOCK " .. tostring(F.statusGate) .. " " .. tostring(F.statusChanges))

-- LOAD GAME of a save without a hard-mode block, with START FROM THE NEXT
-- LOADED SAVE on: the run is adopted, the badge registered, the setting cleared.
save.load(nil, "reset")
save.load(nil, "load")
local value = save.snapshot()
emit(string.format("ADOPTED %s %s %s", tostring(F.camp ~= nil), tostring(value ~= nil),
    tostring(F.settings.adopt_on_load)))
-- The adopted run took FREEZE AND SHOCK from the settings: the gate is on, and
-- the run block carries it.
emit(string.format("ADOPTED_FREEZESHOCK %s %s", tostring(F.statusGate),
    tostring(type(value) == "table" and (value :: any).freeze_shock == true)))

local function fire(name: string)
    F.fire(name, {name = name})
end
-- A hard run's battle start reads the window bosses' loaded rows (their retail
-- kinds, as in tests/mod_hardmode_weakness_test.luau).
F.rows = {
    [310] = {affinities = {physical = "resist", light = "null", dark = "null"}},
    [258] = {affinities = {fire = "drain", light = "null", dark = "null"}},
    [259] = {affinities = {light = "null", dark = "null"}},
}

-- Battle context can exist before the party list is populated. The guard must
-- retry until Yu's data is available, then apply protection exactly once.
F.battleUnits = {party = {}, enemies = {}}
fire("battle_start")
assert(#F.protections == 0, "empty party cannot be protected")
F.battleUnits = {party = {{id = 2, data = 0x2000}, {id = 1, data = 0}}, enemies = {}}
fire("frame_start")
assert(#F.protections == 0, "partial party cannot protect Yu")
F.battleUnits = {party = {{id = 2, data = 0x2000}, {id = 1, data = 0x1000}}, enemies = {}}
for _ = 1, 2 do
    fire("frame_start")
end
assert(#F.protections == 1 and F.protections[1] == 0x1000, "Yu must be protected exactly once")
F.battleUnits = nil

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
-- The gate followed the run: on at the adoption, then off at each LOAD GAME's
-- reset and on again at its load (40 times), and off at the last reset.
emit("FREEZESHOCK_AFTER_PLAIN_LOAD " .. tostring(F.statusGate) .. " " .. tostring(F.statusChanges))
emit("SPRITES " .. tostring(F.sprites))
emit("REFUSED " .. tostring(refused()))

-- A New Game can arm at battle_start after the feature's own start callback
-- was skipped. The same battle must still guard Yu once he appears.
save.load(nil, "reset")
F.battleUnits = {party = {}, enemies = {}}
fire("battle_start")
assert(#F.protections == 1, "empty party on New Game must stay pending")
F.battleUnits = {party = {{id = 1, data = 0x3000}}, enemies = {}}
fire("frame_start")
fire("frame_start")
assert(#F.protections == 2 and F.protections[2] == 0x3000, "New Game battle must guard Yu exactly once")
