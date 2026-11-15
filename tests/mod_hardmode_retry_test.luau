-- Headless checks of hard mode's one-way step-down and in-place boss retry
-- (tests/mod_hardmode_retry_test.py). Third chunk after the fixture and
-- main.luau. Drives the save block, the event handlers, the reward hooks and
-- the p4.battle.defeat handler the game would call: load a HARDER save, step it
-- down to Hard then Off, refuse upward, persist across save/load; then lose boss
-- encounter 811 and decline, accept (the retried battle starts with the
-- battle-start vitals and pays its rewards once), be refused by an Ironman
-- authorizer, and lose an ordinary encounter. Emits facts for the runner.
local F = FIXTURE
local save = F.save
-- A hard run's battle start reads the window bosses' loaded rows (their retail
-- kinds, as in tests/mod_hardmode_weakness_test.luau).
F.rows = {
    [310] = {affinities = {physical = "resist", light = "null", dark = "null"}},
    [258] = {affinities = {fire = "drain", light = "null", dark = "null"}},
    [259] = {affinities = {light = "null", dark = "null"}},
}
emit("READY " .. tostring(save ~= nil and F.logs[#F.logs] == "HARDMODE_READY"))

local fire = F.fire

local function badge(): string
    if F.camp == nil then
        return "none"
    end
    local word = F.camp.items[2]
    return tostring(word.text) .. "|" .. tostring(word.visible)
end

local function count(needle: string): number
    local n = 0
    for _, line in F.logs do
        if string.find(line, needle, 1, true) then
            n += 1
        end
    end
    return n
end

local function findLog(needle: string): boolean
    return count(needle) > 0
end

local function screen(key: string): string
    if F.screen == nil then
        return "none"
    end
    for _, item in F.screen.items do
        if item.key == key then
            return tostring(item.text)
        end
    end
    return "missing"
end

local function closeMenu()
    fire("menu_close", {name = "menu_close", task = 0, kind = 1})
end

-- Step-down ------------------------------------------------------------------

-- New runs are HARD only; a HARDER run exists only in a save made before (its
-- block says mode "harder"). Make one from a Hard run's block.
F.settings.mode = "Hard"
F.settings.battle_retry = true
F.settings.adopt_on_load = true
save.load(nil, "reset")
save.load(nil, "load")
local harder = table.clone(save.snapshot() :: any)
harder.mode = "harder"

-- A HARDER save loads with its own rules, whatever DIFFICULTY says (a HARDER
-- choice saved before reads as Off).
F.settings.mode = "Off"
save.load(nil, "reset")
save.load(harder, "load")
emit(string.format("ADOPTED %s %s %s", badge(), tostring((save.snapshot() :: any).mode == "harder"),
    tostring(F.defeatSub ~= nil and F.defeatSub.enabled)))

-- Loaded runs keep their rules: a setting that disagrees but was not
-- changed after the load steps nothing and refuses nothing.
F.settings.mode = "Hard"
save.load(nil, "reset")
save.load(harder, "load")
emit(string.format("KEPT %s %s", badge(), tostring((save.snapshot() :: any).mode)))

-- Choosing Hard after loading the HARDER save: one step down, persisted.
F.settings.mode = "Off"
save.load(nil, "reset")
save.load(harder, "load")
F.settings.mode = "Hard"
closeMenu()
local hard = save.snapshot()
emit(string.format("STEP_HARD %s %s %s %s", badge(), tostring((hard :: any).mode),
    tostring((hard :: any).stepped), tostring(findLog("HARDMODE_STEPDOWN mode=harder>hard via=menu_close"))))
save.load(nil, "reset")
save.load(hard, "load")
emit(string.format("STEP_PERSIST %s %s %s", badge(), tostring((save.snapshot() :: any).stepped),
    tostring((save.snapshot() :: any).stepped_from)))

-- Step to Off leaves a tombstone that is the run for the save slot; Off is
-- the retail game, so the battle retry stops with it.
F.settings.mode = "Off"
closeMenu()
local off = save.snapshot()
emit(string.format("STEP_OFF %s %s %s %s", badge(), tostring((off :: any).mode),
    tostring(findLog("HARDMODE_STEPDOWN mode=hard>off")), tostring(F.defeatSub.enabled)))
save.load(nil, "reset")
save.load(off, "load")
emit(string.format("OFF_PERSIST %s %s", badge(), tostring((save.snapshot() :: any).mode)))
F.settings.mode = "Hard"
closeMenu()
emit(string.format("OFF_UP_REFUSED %s %s %s", badge(), tostring(findLog("HARDMODE_STEPDOWN refused upward setting=hard run=off")),
    tostring((save.snapshot() :: any).mode)))

-- Battle retry -----------------------------------------------------------------

-- A fresh Hard run with BATTLE RETRY. The encounter table at 0x3000:
-- encounter 811 (Avenger Knight) carries the boss bit 0x02, encounter 99 none.
-- Yu's DatUnit record 0x007973C4 and Yosuke's 0x00796E54 hold the values the
-- native restore wrote back (80/40, 60/30).
F.settings.mode = "Hard"
F.settings.battle_retry = true
F.settings.adopt_on_load = true
save.load(nil, "reset")
save.load(nil, "load")
local retryRun = save.snapshot()
F.memMap[0x764504] = 0x3000
F.memMap[0x3000 + 811 * 0x18] = 0x02
F.memMap[0x3000 + 99 * 0x18] = 0
F.memMap[0x007973C4 + 8] = 80
F.memMap[0x007973C4 + 0xA] = 40
F.memMap[0x00796E54 + 8] = 60
F.memMap[0x00796E54 + 0xA] = 30
local START = {{id = 1, hp = 80, sp = 40, ailments = 0}, {id = 2, hp = 60, sp = 30, ailments = 0}}
emit(string.format("RETRY_RUN %s %s", tostring((retryRun :: any).retry), tostring(F.defeatSub.enabled)))

local function defeat(encounter: number, polls: number, allowed: boolean, reason: string): any
    local context = {name = "defeat", encounter = encounter, source = "field", polls = polls, allowed = allowed,
        reason = reason, party = START}
    return F.defeat(context)
end
local function press(action: string?)
    table.clear(F.input)
    if action ~= nil then
        F.input[action] = true
    end
end

-- Lost to the Knight: the prompt is offered before the Game Over and holds it.
local first = defeat(811, 0, true, "")
emit(string.format("OFFERED %s %s|%s|%s %s", tostring(first), screen("title"), screen("retry"), screen("giveup"),
    tostring(findLog("HARDMODE_RETRY offered encounter=811 mode=hard source=field party=1:80/40,2:60/30"))))
-- A Cross still held from the battle answers nothing for the first polls.
press("confirm")
local early = defeat(811, 1, true, "")
emit(string.format("EARLY %s %s", tostring(early), screen("retry")))
-- Declined: Down moves to GIVE UP, Cross confirms; the retail Game Over runs.
press("down")
local moved = defeat(811, 30, true, "")
press("confirm")
local declined = defeat(811, 31, true, "")
emit(string.format("DECLINED %s %s %s %s", tostring(moved), tostring(declined), screen("title"),
    tostring(findLog("HARDMODE_RETRY declined encounter=811"))))

-- Lost again and accepted: Down then Up keeps RETRY BATTLE; Cross retries.
press(nil)
defeat(811, 0, true, "")
press("down")
defeat(811, 30, true, "")
press("up")
local back = defeat(811, 31, true, "")
local choice = screen("retry")
press("confirm")
local accepted = defeat(811, 32, true, "")
press(nil)
emit(string.format("ACCEPTED %s %s %s %s %s", tostring(back), choice, tostring(accepted), screen("title"),
    tostring(findLog("HARDMODE_RETRY accepted encounter=811 party=1:80/40,2:60/30"))))
-- Nothing paid out for the lost battle.
emit("NO_REWARD " .. tostring(count("HARDMODE_REWARD") == 0))
-- The retried battle starts with the battle-start values in the records.
fire("battle_start", {name = "battle_start", task = 0, setup = 0, battle = 0x10000})
emit("RESTARTED " .. tostring(findLog("HARDMODE_RETRY restarted encounter=811 party=1:80/40,2:60/30 matches=true")))
-- It is won: the game computes each enemy's EXP and yen once, and each is
-- paid (and logged) once. The run block never carries a retry state.
F.memMap[0x5000 + 2] = 310
fire("battle_start", {name = "battle_start", task = 0, setup = 0, battle = 0x10000})
local expHook, yenHook = F.hookFns[0x00243950], F.hookFns[0x002439c0]
local expContext = {args = {0x5000}, result = 100}
local yenContext = {args = {0x5000}, result = 50}
expHook(expContext)
yenHook(yenContext)
fire("battle_end", {name = "battle_end", battle = 0x10000, outcome = 1})
emit(string.format("REWARDS %d %d %s", count("HARDMODE_REWARD id=310 boss=false exp="),
    count("HARDMODE_REWARD id=310 boss=false yen="), tostring(count("HARDMODE_RETRY restarted") == 1)))
local block = save.snapshot() :: any
emit(string.format("BLOCK %s %s %s", tostring(block.mode), tostring(block.retry), tostring(block.stepped)))

-- Challenge IRONMAN's "retry" authorizer refused it: the reason is shown,
-- then the Game Over runs.
local refused = defeat(811, 0, false, "IRONMAN: a lost battle ends this run")
local refusedTitle, refusedReason = screen("title"), screen("reason")
local stillHeld = defeat(811, 149, false, "IRONMAN: a lost battle ends this run")
local released = defeat(811, 150, false, "IRONMAN: a lost battle ends this run")
emit(string.format("IRONMAN %s %s|%s %s %s %s %s", tostring(refused), refusedTitle, refusedReason, tostring(stillHeld),
    tostring(released), screen("title"),
    tostring(findLog("HARDMODE_RETRY refused encounter=811 reason=IRONMAN: a lost battle ends this run"))))

-- An ordinary encounter is never retried.
local ordinary = defeat(99, 0, true, "")
emit(string.format("ORDINARY %s %s %s", tostring(ordinary), screen("title"),
    tostring(findLog("HARDMODE_RETRY refused encounter=99 reason=ordinary encounter"))))

-- A run started with BATTLE RETRY off never enables the handler.
F.settings.battle_retry = false
F.settings.adopt_on_load = true
save.load(nil, "reset")
save.load(nil, "load")
emit(string.format("RETRY_OFF %s %s", tostring((save.snapshot() :: any).retry), tostring(F.defeatSub.enabled)))
