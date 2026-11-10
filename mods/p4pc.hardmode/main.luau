--!strict
-- Bundled hard mode, wave 1: the difficulty chosen in MODS > Hard mode is armed
-- at the next New Game (or once on the next loaded save without hard-mode
-- rules) and saved with the run. Within a run:
--   * enemy maximum HP/SP and stats, and their starting HP/SP, are scaled;
--   * battle EXP and yen are scaled per defeated enemy;
--   * enemies driven by the weighted AI tables stop healing healthy allies and
--     heal a badly hurt ally when their table group allows it.
-- Every multiplier comes from files/DATA/HARDMODE/TUNING.TXT and scales the
-- value the game itself computes, so no table data from the disc is shipped.
--
-- The tuning file is parsed once while the mod initializes: a callback's
-- interpreter budget (4096 safepoints) is too small to parse it, and a malformed
-- file is then reported at start-up. With the difficulty OFF, or outside a
-- hard-mode run, every hook stays disabled and saves carry no hard-mode block.
--
-- Retail addresses are guest addresses from the generated hook catalog; table
-- bases are read from the game's own gp globals at use.

local GP = 0x007690F0
local function gp(offset: number): number
    return GP + offset - 0x10000
end
local SKILL_TABLE = gp(0xb3b8) -- pointer to 0x28-byte skill rows
local ENEMY_AI_TABLE = gp(0xb3d0) -- pointer to 0xA4-byte AICALC rows, by enemy ID
local ENCOUNTER_TABLE = gp(0xb414) -- pointer to 0x18-byte encounter rows, flags word first
local BATTLE = gp(0xb3ac) -- pointer to the battle state
local TUNING_PATH = "DATA/HARDMODE/TUNING.TXT"

-- Every seam must be in this build's hook catalog with the expected signature;
-- a missing one stops the mod instead of leaving a feature half applied.
type Seam = {address: number, signature: string}
local function seam(name: string, address: number, signature: string): Seam
    local info = p4.describe(address)
    if info == nil or info.signature ~= signature then
        error(string.format("hard mode needs %s (0x%08x, %s) in the hook catalog", name, address, signature))
    end
    return {address = address, signature = signature}
end
local A = {
    datUnitCreateEncounterGroup = seam("datUnitCreateEncounterGroup", 0x00231630, "pH"),
    datUnitInit = seam("datUnitInit", 0x00231af0, "IpBH"),
    datCalcMaxHp = seam("datCalcMaxHp", 0x00231f80, "Hp"),
    datCalcMaxSp = seam("datCalcMaxSp", 0x00232290, "Hp"),
    datCalcGetUnitStat = seam("datCalcGetUnitStat", 0x00232b40, "Bpi"),
    datCalcEnemyExp = seam("datCalcEnemyExp", 0x00243950, "Hp"),
    datCalcEnemyMoney = seam("datCalcEnemyMoney", 0x002439c0, "Hp"),
    btlAISelectWeightedEntry = seam("btlAISelectWeightedEntry", 0x001dea90, "ppI"),
}

-- DatUnit: flags u16 +0 (bit 2: enemy), id u16 +2, hp u16 +8, sp u16 +0xA,
-- bad u32 +0xC (bit 0x80000: KO). BtlAction.unit +0x30. BtlUnit: genus u8
-- +0xA2 (1 enemy), charId u16 +0xA4, datUnit +0xA64, nextUnit +0xA6C. The
-- battle state's genus lists are head/tail pairs from +0x178.
local UNIT_ENEMY_FLAG = 4
local UNIT_KO = 0x80000
local ENEMY_LIST_HEAD = 0x178 + 8
local AI_ROW_SIZE = 0xA4
local AI_GROUP_SIZE = 0x28
local AI_ENTRIES = 0x2C
local SKILL_ROW_SIZE = 0x28
local RETAIL_ACTIVE_SKILLS = 0x1B8 -- metadata rows a retail AI table command can name
local SKILL_KIND_HP_RECOVERY = 2 -- skill row +0x11

-- Annotated: unannotated string-literal arrays are inferred as unions of
-- singletons, which the type solver cannot finish once they are concatenated.
local FIELDS: {string} = {"hp", "sp", "st", "ma", "en", "ag", "lu", "exp", "yen"}
local STAT_FIELDS: {string} = {"st", "ma", "en", "ag", "lu"} -- datCalcGetUnitStat index order
local MODES: {string} = {"hard", "harder"}
local MODE_LABELS: {[string]: string} = {hard = "HARD", harder = "HARDER"}
local CLASSES: {string} = {"ordinary", "boss"}

type Fields = {[string]: number}
type Tuning = {
    revision: number,
    bossFlags: number,
    modes: {[string]: {[string]: Fields}},
    skipHealAbove: number,
    rescueBelow: number,
    enemy: {[number]: {[string]: Fields}},
}
type Run = {mode: string, stats: boolean, ai: boolean, rewards: boolean, revision: number, adopted: boolean}

local function log(message: string)
    p4.log(message)
end

-- Tuning file -----------------------------------------------------------------

local function parseNumber(text: string, where: string): number
    local value: number?
    if string.sub(text, 1, 2) == "0x" then
        value = tonumber(string.sub(text, 3), 16)
    else
        value = tonumber(text)
    end
    if value == nil then
        error(where .. ": not a number: " .. text)
    end
    return value :: number
end

local function checkRange(value: number, low: number, high: number, where: string)
    if value < low or value > high then
        error(string.format("%s: %s is outside %s..%s", where, tostring(value), tostring(low), tostring(high)))
    end
end

local function readSections(): {[string]: {[string]: string}}
    local text = buffer.tostring(p4.files.read(TUNING_PATH, 65536))
    local sections: {[string]: {[string]: string}} = {[""] = {}}
    local section = ""
    local lineNo = 0
    for line in string.gmatch(text .. "\n", "([^\n]*)\n") do
        lineNo += 1
        local stripped = (string.gsub(line, "#.*$", ""))
        stripped = (string.gsub(stripped, "\r", ""))
        stripped = string.match(stripped, "^%s*(.-)%s*$") :: string
        local where = string.format("%s:%d", TUNING_PATH, lineNo)
        if stripped ~= "" then
            local name = string.match(stripped, "^%[(.+)%]$")
            if name then
                if sections[name] then
                    error(where .. ": duplicate section [" .. name .. "]")
                end
                section = name
                sections[section] = {}
            else
                local key, value = string.match(stripped, "^([%w_.]+)%s*=%s*(%S+)$")
                if key == nil or value == nil then
                    error(where .. ": expected key = value")
                end
                local keys = sections[section]
                if keys[key :: string] ~= nil then
                    error(where .. ": duplicate key " .. (key :: string))
                end
                keys[key :: string] = value :: string
            end
        end
    end
    return sections
end

local function takeNumber(keys: {[string]: string}, key: string, where: string): number
    local text = keys[key]
    if text == nil then
        error(string.format("%s: [%s] is missing %s", TUNING_PATH, where, key))
    end
    keys[key] = nil
    return parseNumber(text, string.format("%s [%s] %s", TUNING_PATH, where, key))
end

local function noLeftovers(keys: {[string]: string}, where: string)
    for key in keys do
        error(string.format("%s: [%s] has unknown key %s", TUNING_PATH, where, key))
    end
end

local function loadTuning(): Tuning
    local sections = readSections()
    local top = sections[""]
    local revision = takeNumber(top, "revision", "top")
    if revision < 1 or revision % 1 ~= 0 then
        error(TUNING_PATH .. ": revision must be a positive whole number")
    end
    noLeftovers(top, "top")
    sections[""] = nil

    local classes = sections.classes
    if classes == nil then
        error(TUNING_PATH .. ": missing [classes]")
    end
    local bossFlags = takeNumber(classes, "boss_flags", "classes")
    checkRange(bossFlags, 1, 0xffffffff, TUNING_PATH .. " boss_flags")
    noLeftovers(classes, "classes")
    sections.classes = nil

    local modes: {[string]: {[string]: Fields}} = {}
    for _, mode in MODES do
        local keys = sections[mode]
        if keys == nil then
            error(TUNING_PATH .. ": missing [" .. mode .. "]")
        end
        local byClass: {[string]: Fields} = {}
        for _, group in CLASSES do
            local fields: Fields = {}
            for _, field in FIELDS do
                local value: number = takeNumber(keys, group .. "." .. field, mode)
                checkRange(value, 0.25, 4.0, string.format("%s [%s] %s.%s", TUNING_PATH, mode, group, field))
                fields[field] = value
            end
            byClass[group] = fields
        end
        noLeftovers(keys, mode)
        modes[mode] = byClass
        sections[mode] = nil
    end

    local ai = sections.ai
    if ai == nil then
        error(TUNING_PATH .. ": missing [ai]")
    end
    local skipHealAbove = takeNumber(ai, "skip_heal_above", "ai")
    local rescueBelow = takeNumber(ai, "rescue_below", "ai")
    checkRange(skipHealAbove, 0, 1, TUNING_PATH .. " skip_heal_above")
    checkRange(rescueBelow, 0, 1, TUNING_PATH .. " rescue_below")
    if rescueBelow >= skipHealAbove then
        error(TUNING_PATH .. ": rescue_below must be below skip_heal_above")
    end
    noLeftovers(ai, "ai")
    sections.ai = nil

    local enemy: {[number]: {[string]: Fields}} = {}
    for name, keys in sections do
        local idText = string.match(name, "^enemy (%d+)$")
        if idText == nil then
            error(TUNING_PATH .. ": unknown section [" .. name .. "]")
        end
        local id = tonumber(idText) :: number
        checkRange(id, 0, 0xffff, TUNING_PATH .. " [" .. name .. "]")
        local byMode: {[string]: Fields} = {}
        for key, text in keys do
            local mode, field = string.match(key, "^(%a+)%.(%a+)$")
            if mode == nil or field == nil or modes[mode :: string] == nil or table.find(FIELDS, field :: string) == nil then
                error(string.format("%s: [%s] has unknown key %s", TUNING_PATH, name, key))
            end
            local value = parseNumber(text, string.format("%s [%s] %s", TUNING_PATH, name, key))
            checkRange(value, 0.25, 4.0, string.format("%s [%s] %s", TUNING_PATH, name, key))
            local fields = byMode[mode :: string] or {}
            fields[field :: string] = value
            byMode[mode :: string] = fields
        end
        enemy[id] = byMode
    end

    return {revision = revision, bossFlags = bossFlags, modes = modes, skipHealAbove = skipHealAbove,
        rescueBelow = rescueBelow, enemy = enemy}
end

local tuningData: Tuning = loadTuning()
log(string.format("HARDMODE_TUNING revision=%d boss_flags=0x%x", tuningData.revision, tuningData.bossFlags))
local function tuning(): Tuning
    return tuningData
end

-- Run state -------------------------------------------------------------------

local run: Run? = nil
local currentBoss = false -- class of the encounter being built or fought
local unitBoss: {[number]: boolean} = {} -- DatUnit address -> boss class
local rewardLogged: {[string]: boolean} = {}

local function runFromSettings(adopted: boolean): Run?
    local choice = p4.settings.get("mode")
    local mode: string? = nil
    if choice == "Hard" then
        mode = "hard"
    elseif choice == "Harder" then
        mode = "harder"
    end
    if mode == nil then
        return nil
    end
    return {
        mode = mode :: string,
        stats = p4.settings.get("enemy_stats") == true,
        ai = p4.settings.get("smart_ai") == true,
        rewards = p4.settings.get("rewards") == true,
        revision = tuning().revision,
        adopted = adopted,
    }
end

local function encodeRun(current: Run): P4SaveValue
    return {mode = current.mode, stats = current.stats, ai = current.ai, rewards = current.rewards,
        revision = current.revision, adopted = current.adopted}
end

local function decodeRun(value: P4SaveValue): (Run?, string?)
    if type(value) ~= "table" then
        return nil, "the hard-mode block is not a table"
    end
    local t = value :: {[any]: any}
    if MODE_LABELS[t.mode] == nil then
        return nil, "unknown hard-mode difficulty " .. tostring(t.mode)
    end
    for _, key in {"stats", "ai", "rewards", "adopted"} do
        if type(t[key]) ~= "boolean" then
            return nil, "the hard-mode block has no valid " .. key
        end
    end
    if type(t.revision) ~= "number" or t.revision < 1 or t.revision % 1 ~= 0 then
        return nil, "the hard-mode block has no valid tuning revision"
    end
    for key in t do
        if key ~= "mode" and key ~= "stats" and key ~= "ai" and key ~= "rewards" and key ~= "revision"
            and key ~= "adopted" then
            return nil, "the hard-mode block has an unknown field " .. tostring(key)
        end
    end
    return {mode = t.mode, stats = t.stats, ai = t.ai, rewards = t.rewards, revision = t.revision,
        adopted = t.adopted}, nil
end

local function describeRun(current: Run): string
    local features = {}
    if current.stats then table.insert(features, "stats") end
    if current.ai then table.insert(features, "ai") end
    if current.rewards then table.insert(features, "rewards") end
    return string.format("mode=%s features=%s revision=%d adopted=%s", current.mode,
        if #features > 0 then table.concat(features, ",") else "none", current.revision, tostring(current.adopted))
end

-- Enemy scaling ----------------------------------------------------------------

local function isEnemyUnit(unit: number): boolean
    return unit ~= 0 and bit32.band(p4.mem.read("u16", unit), UNIT_ENEMY_FLAG) ~= 0
end

-- The class the unit was created in; summoned units inherit their encounter's.
local function isBoss(unit: number): boolean
    local boss = unitBoss[unit]
    if boss == nil then
        return currentBoss
    end
    return boss
end

local function multiplier(current: Run, unit: number, field: string): number
    local data = tuning()
    local overrides = data.enemy[p4.mem.read("u16", unit + 2)]
    if overrides then
        local fields = overrides[current.mode]
        if fields and fields[field] ~= nil then
            return fields[field]
        end
    end
    return data.modes[current.mode][if isBoss(unit) then "boss" else "ordinary"][field]
end

local function scaled(value: number, factor: number, low: number, high: number): number
    if factor == 1 or value == 0 then
        return value
    end
    local result = math.floor(value * factor + 0.5)
    if result < low then
        return low
    elseif result > high then
        return high
    end
    return result
end

local encounterHook = p4.hook(A.datUnitCreateEncounterGroup.address, A.datUnitCreateEncounterGroup.signature, "before",
    function(context)
    unitBoss = {}
    rewardLogged = {}
    local base = p4.mem.read("u32", ENCOUNTER_TABLE)
    local encounter = context.args[1] :: number
    local flags = if base ~= 0 then p4.mem.read("u32", base + encounter * 0x18) else 0
    currentBoss = bit32.band(flags, tuning().bossFlags) ~= 0
    log(string.format("HARDMODE_ENCOUNTER id=%d boss=%s", encounter, tostring(currentBoss)))
end)

local unitInitHook = p4.hook(A.datUnitInit.address, A.datUnitInit.signature, "after", function(context)
    if (context.args[2] :: number) ~= 1 then
        return -- party units
    end
    local unit = context.args[1] :: number
    unitBoss[unit] = currentBoss
    local current = run
    if current == nil or not current.stats then
        return
    end
    local hp = p4.mem.read("u16", unit + 8)
    local sp = p4.mem.read("u16", unit + 0xA)
    local newHp = scaled(hp, multiplier(current, unit, "hp"), 1, 0xffff)
    local newSp = scaled(sp, multiplier(current, unit, "sp"), 1, 0xffff)
    p4.mem.write("u16", unit + 8, newHp)
    p4.mem.write("u16", unit + 0xA, newSp)
    -- The five stats are scaled where they are read; log the original (unhooked)
    -- and scaled values once per unit as evidence.
    local stats = {}
    for index, field in STAT_FIELDS do
        local native = p4.call(A.datCalcGetUnitStat.address, A.datCalcGetUnitStat.signature, unit, index - 1) :: number
        table.insert(stats, string.format("%s=%d>%d", field, native, scaled(native, multiplier(current, unit, field), 1, 99)))
    end
    log(string.format("HARDMODE_ENEMY id=%d boss=%s hp=%d>%d sp=%d>%d %s", context.args[3] :: number,
        tostring(currentBoss), hp, newHp, sp, newSp, table.concat(stats, " ")))
end)

local function scaleMaximum(field: string): (context: P4HookContext) -> ()
    return function(context)
        local current = run
        if current == nil then
            return
        end
        local unit = context.args[1] :: number
        if not isEnemyUnit(unit) then
            return
        end
        context.result = scaled(context.result :: number, multiplier(current, unit, field), 1, 0xffff)
    end
end
local maxHpHook = p4.hook(A.datCalcMaxHp.address, A.datCalcMaxHp.signature, "after", scaleMaximum("hp"))
local maxSpHook = p4.hook(A.datCalcMaxSp.address, A.datCalcMaxSp.signature, "after", scaleMaximum("sp"))

-- datCalcGetBattleStat reads an enemy's stat through datCalcGetUnitStat, so
-- this one seam covers both (the battle stat's own halving and clamp follow).
local statHook = p4.hook(A.datCalcGetUnitStat.address, A.datCalcGetUnitStat.signature, "after", function(context)
    local current = run
    if current == nil then
        return
    end
    local unit = context.args[1] :: number
    if not isEnemyUnit(unit) then
        return
    end
    local field = STAT_FIELDS[bit32.band(context.args[2] :: number, 0xffff) + 1]
    if field == nil then
        return -- the native body already refused the index
    end
    context.result = scaled(context.result :: number, multiplier(current, unit, field), 1, 99)
end)

local function scaleReward(field: string): (context: P4HookContext) -> ()
    return function(context)
        local current = run
        if current == nil then
            return
        end
        local unit = context.args[1] :: number
        local native = context.result :: number
        local value = scaled(native, multiplier(current, unit, field), 0, 0xffff)
        context.result = value
        local key = string.format("%x:%s", unit, field)
        if not rewardLogged[key] then
            rewardLogged[key] = true
            log(string.format("HARDMODE_REWARD id=%d boss=%s %s=%d>%d", p4.mem.read("u16", unit + 2),
                tostring(isBoss(unit)), field, native, value))
        end
    end
end
local expHook = p4.hook(A.datCalcEnemyExp.address, A.datCalcEnemyExp.signature, "after", scaleReward("exp"))
local moneyHook = p4.hook(A.datCalcEnemyMoney.address, A.datCalcEnemyMoney.signature, "after", scaleReward("yen"))

-- Enemy support discipline --------------------------------------------------------

local function isHpRecovery(command: number): boolean
    if command >= RETAIL_ACTIVE_SKILLS then
        return false -- native actions (0x8000...) and anything outside the retail AI domain
    end
    local rows = p4.mem.read("u32", SKILL_TABLE)
    return rows ~= 0 and p4.mem.read("u8", rows + command * SKILL_ROW_SIZE + 0x11) == SKILL_KIND_HP_RECOVERY
end

local function maxHp(current: Run, unit: number): number
    local native = p4.call(A.datCalcMaxHp.address, A.datCalcMaxHp.signature, unit) :: number -- original, unhooked
    if current.stats then
        return scaled(native, multiplier(current, unit, "hp"), 1, 0xffff)
    end
    return native
end

-- Lowest and highest HP fraction among the living enemy-side units.
local function allyHealth(current: Run): (number, number)
    local battle = p4.mem.read("u32", BATTLE)
    local lowest, highest = 1, 0
    if battle == 0 then
        return lowest, highest
    end
    local node = p4.mem.read("u32", battle + ENEMY_LIST_HEAD)
    local guard = 0
    while node ~= 0 and guard < 16 do
        guard += 1
        local unit = p4.mem.read("u32", node + 0xA64)
        if unit ~= 0 and bit32.band(p4.mem.read("u32", unit + 0xC), UNIT_KO) == 0 then
            local maximum = maxHp(current, unit)
            if maximum > 0 then
                local fraction = p4.mem.read("u16", unit + 8) / maximum
                if fraction < lowest then lowest = fraction end
                if fraction > highest then highest = fraction end
            end
        end
        node = p4.mem.read("u32", node + 0xA6C)
    end
    return lowest, highest
end

local function bestEntry(group: number, wantRecovery: boolean): number?
    local best: number? = nil
    local bestWeight = 0
    for index = 0, 4 do
        local entry = group + index * 8
        local weight = p4.mem.read("u8", entry)
        if weight > bestWeight and isHpRecovery(p4.mem.read("u16", entry + 2)) == wantRecovery then
            best, bestWeight = entry, weight
        end
    end
    return best
end

-- The game's own pick (and its one random draw) always happens; a different
-- entry of the same table group is returned only for the two rules above.
local aiHook = p4.hook(A.btlAISelectWeightedEntry.address, A.btlAISelectWeightedEntry.signature, "replace",
    function(context)
    local entry = context.next() :: number
    context.result = entry
    local current = run
    if current == nil or entry == 0 then
        return
    end
    local action = context.args[1] :: number
    local actor = p4.mem.read("u32", action + 0x30)
    if actor == 0 or p4.mem.read("u8", actor + 0xA2) ~= 1 then
        return
    end
    -- Boss encounters keep the game's own choices, including any table pick a
    -- boss script requests through the same selector.
    if isBoss(p4.mem.read("u32", actor + 0xA64)) then
        return
    end
    local rows = p4.mem.read("u32", ENEMY_AI_TABLE)
    local enemyId = p4.mem.read("u16", actor + 0xA4)
    local group = rows + enemyId * AI_ROW_SIZE + bit32.band(context.args[2] :: number, 0xffff) * AI_GROUP_SIZE
        + AI_ENTRIES
    if rows == 0 or entry < group or entry >= group + 5 * 8 or (entry - group) % 8 ~= 0 then
        return -- not this retail row (another owner supplied the table): keep the game's pick
    end
    local data = tuning()
    local picked = p4.mem.read("u16", entry + 2)
    local pickedRecovery = isHpRecovery(picked)
    local lowest, highest = allyHealth(current)
    local chosen: number? = nil
    local reason = ""
    if pickedRecovery and lowest > data.skipHealAbove then
        chosen, reason = bestEntry(group, false), "skip_heal"
    elseif not pickedRecovery and lowest < data.rescueBelow then
        chosen, reason = bestEntry(group, true), "rescue"
    end
    if chosen ~= nil and chosen ~= entry then
        context.result = chosen
        log(string.format("HARDMODE_AI enemy=%d native=%d chosen=%d reason=%s lowest=%.2f highest=%.2f", enemyId,
            picked, p4.mem.read("u16", (chosen :: number) + 2), reason, lowest, highest))
    end
end)

-- Each feature's hooks run only in a run that has it. Every feature needs the
-- encounter class, recorded per enemy unit as it is created.
local FEATURE_HOOKS: {[string]: {P4Subscription}} = {
    stats = {maxHpHook, maxSpHook, statHook},
    rewards = {expHook, moneyHook},
    ai = {aiHook},
}
local CLASS_HOOKS = {encounterHook, unitInitHook}

local function setEnabled(hooks: {P4Subscription}, enabled: boolean)
    for _, hook in hooks do
        if enabled then hook:enable() else hook:disable() end
    end
end

local function applyFeatures()
    local current = run
    setEnabled(FEATURE_HOOKS.stats, current ~= nil and current.stats)
    setEnabled(FEATURE_HOOKS.rewards, current ~= nil and current.rewards)
    setEnabled(FEATURE_HOOKS.ai, current ~= nil and current.ai)
    setEnabled(CLASS_HOOKS, current ~= nil and (current.stats or current.rewards or current.ai))
end
applyFeatures() -- everything off until a run is armed

-- Run lifecycle -------------------------------------------------------------------

local statusShown = false
local function updateStatus()
    local current = run
    if current == nil then
        if statusShown then
            p4.ui.status(nil, "camp")
            statusShown = false
        end
        return
    end
    local features = {}
    if current.stats then table.insert(features, "ENEMY STATS") end
    if current.ai then table.insert(features, "SUPPORT AI") end
    if current.rewards then table.insert(features, "REWARDS") end
    local title = "HARD MODE: " .. MODE_LABELS[current.mode] .. (if current.adopted then " (ADOPTED)" else "")
    p4.ui.status({title, if #features > 0 then table.concat(features, ", ") else "NO FEATURES"}, "camp")
    statusShown = true
end

local function adopt(next: Run?, why: string)
    run = next
    unitBoss = {}
    rewardLogged = {}
    currentBoss = false
    local current = run
    if current then
        local installed = tuning().revision
        if current.revision ~= installed then
            -- Only the installed tuning file exists; the run continues on it.
            log(string.format("HARDMODE_REVISION saved=%d current=%d", current.revision, installed))
        end
        log("HARDMODE_RUN " .. why .. " " .. describeRun(current))
    else
        log("HARDMODE_RUN " .. why .. " off")
    end
    applyFeatures()
    updateStatus()
end

-- The game resets to the New Game defaults at boot, when LOAD GAME starts
-- restoring a save (a load follows) and before New Game. A reset therefore only
-- marks a pending New Game; the run is armed at the first field or battle that
-- no load preceded. The two arming subscriptions are enabled only while a
-- difficulty is selected, so with hard mode OFF no callback runs at all.
local pendingNewGame = false
local armSubscriptions: {P4Subscription} = {}

local function setPending(pending: boolean)
    pendingNewGame = pending
    setEnabled(armSubscriptions, pending)
end

local function armNewGame()
    if not pendingNewGame then
        return
    end
    setPending(false)
    adopt(runFromSettings(false), "new_game")
end

armSubscriptions = {
    p4.events.on("field_enter", function()
        armNewGame()
    end),
    p4.events.on("battle_start", function()
        armNewGame()
    end),
}
setPending(false)

p4.save.register({
    schema = 1,
    max_bytes = 256,
    snapshot = function(): P4SaveValue
        local current = run
        if current == nil then
            return nil
        end
        return encodeRun(current)
    end,
    authorize_load = function(value: P4SaveValue): (boolean, ...string)
        if value == nil then
            return true
        end
        local decoded, why = decodeRun(value)
        if decoded == nil then
            return false, "Hard mode cannot load this save: " .. tostring(why)
        end
        return true
    end,
    load = function(value: P4SaveValue, origin: "load" | "reset")
        if origin == "reset" then
            run = nil
            unitBoss = {}
            rewardLogged = {}
            currentBoss = false
            applyFeatures()
            updateStatus()
            local choice = p4.settings.get("mode")
            setPending(choice == "Hard" or choice == "Harder")
            log("HARDMODE_RESET pending=" .. tostring(pendingNewGame))
            return
        end
        setPending(false)
        if value ~= nil then
            local decoded, why = decodeRun(value)
            if decoded == nil then
                error("hard-mode save block rejected after authorization: " .. tostring(why))
            end
            adopt(decoded, "load")
            return
        end
        if p4.settings.get("adopt_on_load") == true then
            local adopted = runFromSettings(true)
            if adopted ~= nil then
                local ok, why = p4.settings.set("adopt_on_load", false)
                if not ok then
                    error("hard mode could not clear START FROM THE NEXT LOADED SAVE: " .. tostring(why))
                end
                adopt(adopted, "adopted")
                return
            end
        end
        adopt(nil, "load")
    end,
})

log("HARDMODE_READY")
