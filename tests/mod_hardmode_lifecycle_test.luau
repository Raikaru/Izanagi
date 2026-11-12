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
emit("BOOT_POLICY " .. tostring(F.policy.enabled))

-- LOAD GAME of a save without a hard-mode block, with START FROM THE NEXT
-- LOADED SAVE on: the run is adopted, the badge registered, the setting cleared.
save.load(nil, "reset")
save.load(nil, "load")
local value = save.snapshot()
emit("ADOPT_POLICY " .. tostring(F.policy.enabled))
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
    [304] = {affinities = {fire = "drain", wind = "null", light = "null", dark = "null"}},
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
    if F.camp ~= nil or F.policy.enabled then held = false end
    local allowed = save.authorize_load(value)
    if not allowed then held = false end
    save.load(value, "load")
    if F.camp == nil or not F.policy.enabled then held = false end
end
emit(string.format("BADGE_CYCLES %d %s", CYCLES, tostring(held)))

-- A load of a save without a block (and nothing to adopt) ends the run.
save.load(nil, "reset")
save.load(nil, "load")
emit("OFF_AFTER_PLAIN_LOAD " .. tostring(F.camp == nil))
-- The gate followed the run: on at the adoption, then off at each LOAD GAME's
-- reset and on again at its load (40 times), and off at the last reset.
emit("FREEZESHOCK_AFTER_PLAIN_LOAD " .. tostring(F.statusGate) .. " " .. tostring(F.statusChanges))

emit("PLAIN_POLICY " .. tostring(F.policy.enabled))

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

-- These assertions characterize the existing run/save boundary, not the
-- boot-composed SKILL rows. The synthetic dispatcher respects subscriptions so
-- both pending-New-Game entry points can be exercised.
local function dispatch(name: string)
    F.fire(name, {name = name})
end

local function sameRun(a: any, b: any): boolean
    if type(a) ~= "table" or type(b) ~= "table" then return a == b end
    for key, value in a do
        if b[key] ~= value then return false end
    end
    for key, value in b do
        if a[key] ~= value then return false end
    end
    return true
end

F.settings.mode = "Off"
F.settings.party_kit = true
F.settings.adopt_on_load = true
save.load(nil, "reset")
dispatch("field_enter")
dispatch("battle_start")
local offReset = save.snapshot() == nil
save.load(nil, "load")
emit("OFF_NO_ARM " .. tostring(offReset and save.snapshot() == nil and F.settings.adopt_on_load))

-- Arm a run, then restore it with different current settings.
F.settings.mode = "Hard"
F.settings.adopt_on_load = false
F.settings.party_kit = false
save.load(nil, "reset")
dispatch("battle_start")
local offRun = save.snapshot()

-- A saved hard run restores independently of the currently selected mode and
-- restart toggle. A bad block is rejected by authorization before notification.
F.settings.mode = "Off"
F.settings.party_kit = true
save.load(nil, "reset")
local accepted = save.authorize_load(offRun)
if accepted then save.load(offRun, "load") end
local restored = sameRun(save.snapshot(), offRun)
local bad = table.clone(offRun)
bad.mode = "invalid"
local allowed = save.authorize_load(bad)
emit("SAVED_OFF_AND_REFUSAL " .. tostring(accepted and restored and not allowed
    and sameRun(save.snapshot(), offRun)))

-- The reset preceding a plain load arms a pending New Game only temporarily:
-- a plain load without adoption cancels it, even if a field event follows.
F.settings.mode = "Hard"
F.settings.adopt_on_load = false
save.load(nil, "reset")
save.load(nil, "load")
dispatch("field_enter")
emit("PLAIN_CANCELS_PENDING " .. tostring(save.snapshot() == nil))

-- Settings alone do not arm the policy; the first New Game entry does.
F.settings.mode = "Hard"
save.load(nil, "reset")
local pending = not F.policy.enabled
F.fire("field_enter", {name = "field_enter"})
local armed = F.policy.enabled
F.settings.mode = "Off"
local persisted = F.policy.enabled and save.snapshot() ~= nil
local newValue = save.snapshot()
save.load(nil, "reset")
local reset = not F.policy.enabled
save.load(newValue, "load")
emit("NEW_GAME_POLICY " .. tostring(pending and armed and persisted and reset and F.policy.enabled))

-- Synthetic boss callbacks under the v1 window rules, not a game or balance
-- witness. Enemy 304's approved window has no skill lock: even a shared
-- guarded skill like White Wall must not suppress it. A landing costs it one
-- turn and is cured at its next own turn; the once-per-phase guard then holds
-- (the Contrarian King has one phase), and an encounter resets it.
local hit: any = {target_side = "enemy", target_id = 304, component = "ailment", statuses = 0x08}
local function chance(): any return F.chance.fn(hit) end
save.load(nil, "reset")
assert(chance() == 0) -- table rows may be installed when no run is armed
F.settings.mode = "Hard"
F.settings.boss_play = false
save.load(nil, "reset")
dispatch("field_enter")
assert(chance() == 0)
F.settings.boss_play = true -- settings cannot change this run
assert(chance() == 0)
save.load(nil, "reset")
dispatch("field_enter")
assert(chance() == nil)
hit.stock = 60
assert(math.abs(chance() - 0.5) < 1e-9) -- the designed 30% over the roll's own 60%
hit.stock = nil
local savedRun = save.snapshot()
save.load(nil, "reset")
assert(chance() == 0)
save.load(savedRun, "load")
assert(chance() == nil) -- the armed run restores its BOSS PLAY choice
hit.target_id = 305
assert(chance() == nil) -- unrelated bosses do not acquire this window
hit.target_id = 304

local encounterBase, encounter = 0x10000, 1
local action, actor, unit = 0x20000, 0x21000, 0x22000
local mem = F.memMap
mem[0x007690F0 + 0xb414 - 0x10000] = encounterBase
mem[encounterBase + encounter * 0x18] = 2
mem[action + 0x30], mem[actor + 0xA2], mem[actor + 0xA64] = actor, 1, unit
mem[unit + 2], mem[unit + 0xC] = 304, 0
local encounterHook = F.hookHandlers[0x00231630]
local turnHook = F.hookHandlers[0x001de640]
local function startEncounter()
    assert(encounterHook.enabled)
    encounterHook.fn({args = {encounter}})
end
local function turn(skill: number)
    assert(turnHook.enabled)
    turnHook.fn({args = {action, 0, skill}})
end
startEncounter()
turn(0xf1) -- White Wall: a shared skill guard for other bosses, not this one
assert(chance() == nil)
mem[unit + 0xC] = 0x08
turn(1) -- landed: it holds through this turn, and the phase's window is spent
assert(mem[unit + 0xC] == 0x08 and chance() == 0)
turn(1) -- cured at its next own turn
assert(mem[unit + 0xC] == 0 and chance() == 0)
for _ = 1, 3 do turn(1); assert(chance() == 0) end -- once per phase
startEncounter()
turn(1)
assert(chance() == nil) -- encounter reset clears the window

local technical = F.damageHandlers[100]
assert(technical.enabled)
local electric = {element = 3, hit_kind = "normal", reflected = false, target_status = 0x08, resolved = false,
    source_side = "party"}
assert(technical.fn(electric) == 1.5)
electric.source_side = "enemy"
assert(technical.fn(electric) == 1.25)
electric.hit_kind = "weak"
assert(technical.fn(electric) == nil)
mem[unit + 2] = 310
hit.target_id, hit.statuses = 310, 0x10
startEncounter()
turn(0xe3) -- the principal boss's existing charge guard remains
assert(chance() == 0)
save.load(nil, "reset")
assert(chance() == 0)
emit("BOSS_WINDOW true")

-- Check the real consumer across save transitions, not just its arithmetic.
-- v1 (2026-11-10): an ordinary enemy's EXP is its epoch's hp factor and its
-- yen 1.00 on HARD and on a HARDER save alike; the synthetic tuning adds a
-- per-enemy override (enemy 42) so class defaults cannot mask a regression to
-- aggregate or encounter-wide rewards.
local function reward(kind: string, unit: number, enemy: number, amount: number): number
    local handler = F.reward[kind]
    return if handler.subscription.enabled then handler.fn({unit = unit, enemy = enemy, amount = amount}) or amount else amount
end
local function unchanged()
    assert(reward("exp", 0x100100, 42, 10) == 10)
    assert(reward("yen", 0x100100, 42, 10) == 10)
end
local function ordinary(unit: number)
    assert(reward("exp", unit, 43, 10) == 12) -- epoch 1 (the Castle): hp 1.20, half up
    assert(reward("yen", unit, 43, 20) == 20)
    assert(reward("exp", unit, 43, 65535) == 65535)
end
unchanged() -- legacy unarmed load
-- Arming happens on the first field/battle, not during reset. OFF never arms.
F.settings.mode = "Off"
save.load(nil, "reset")
dispatch("field_enter")
unchanged()
F.settings.mode = "Hard"
F.settings.enemy_rules = true
save.load(nil, "reset")
unchanged()
dispatch("field_enter")
ordinary(0x100120)
F.settings.mode = "Off" -- saved run controls rewards despite current settings
save.load(nil, "reset")
unchanged()
save.load(nil, "load")
unchanged()
value.mode = "harder"
value.stats = false
value.rewards = false
value.epoch = 1
save.load(value, "load")
unchanged()
value.rewards = true
local encounterTable = 0x200000
F.memMap[0x007690f0 + 0xb414 - 0x10000] = encounterTable
F.memMap[encounterTable + 0x18] = 2
for _, mode in {"hard", "harder"} do
    value.mode = mode
    save.load(nil, "reset")
    save.load(value, "load")
    F.hookHandlers[0x00231630].fn({args = {1}})
    F.hookHandlers[0x00231af0].fn({args = {0x100100, 1, 42}})
    F.hookHandlers[0x00231af0].fn({args = {0x100110, 1, 43}})
    if mode == "harder" then
        assert(reward("exp", 0x100100, 42, 3) == 5) -- the override: x1.50, half up
        assert(reward("yen", 0x100100, 42, 40000) == 65535)
        assert(reward("exp", 0x100100, 42, 3) == 5) -- no accumulation across calls
    end
    assert(reward("yen", 0x100100, 42, 0) == 0)
    assert(reward("exp", 0x100110, 43, 13) == 13) -- boss default 1.00
    assert(reward("yen", 0x100110, 43, 13) == 13)
    F.hookHandlers[0x00231630].fn({args = {0}})
    F.hookHandlers[0x00231af0].fn({args = {0x100120, 1, 43}})
    ordinary(0x100120)
end
local saved = save.snapshot()
save.load(nil, "reset")
unchanged()
assert(save.authorize_load(saved))
save.load(saved, "load")
ordinary(0x100120)
save.load(nil, "reset")
save.load(nil, "load")
unchanged()
emit("REWARD_CONSUMER true")
