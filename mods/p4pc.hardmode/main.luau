--!strict
-- Bundled hard mode: the difficulty chosen in MODS > Hard mode is armed at the
-- next New Game (or once on the next loaded save without hard-mode rules) and
-- saved with the run. Within a run:
--   * enemy maximum HP/SP and stats, and their starting HP/SP, are scaled;
--     ordinary enemies by the run's story epoch (skill rebalance family 1);
--   * the HP damage every enemy hit deals is scaled through the final-damage
--     stage (p4.damage, datCalcSkillAmount 0x00235520): ordinary enemies by
--     epoch, the unscripted floor bosses by their own factor;
--   * battle EXP and yen are scaled per defeated enemy;
--   * enemies driven by the weighted AI tables stop healing healthy allies and
--     heal a badly hurt ally when their table group allows it;
--   * Technical: a normal hit of the element paired with an ailment the target
--     already has deals extra damage, for both sides, through the same stage;
--   * boss tactics: Avenger Knight and Shadow Kanji use Power Charge before
--     charged attacks, while Shadow Yukiko uses her own Mind Charge. Their
--     bursts are sized per difficulty through the same damage stage;
--   * boss Technical windows: the principal bosses can suffer one ailment of the
--     Technical set, worn off by SMT V's roll and guarded on their scripted and
--     charged turns, through the status chance stage (p4.chance, 0x00238940).
-- Every multiplier comes from files/DATA/HARDMODE/TUNING.TXT and scales the
-- value the game itself computes, so no table data from the disc is shipped.
--
-- The tuning file is parsed once while the mod initializes: a callback's
-- interpreter budget (4096 safepoints) is too small to parse it, and a malformed
-- file is then reported at start-up. With the difficulty OFF, or outside a
-- hard-mode run, every hook stays disabled and saves carry no hard-mode block;
-- the window guard alone stays on, so a boss whose window rows are loaded keeps
-- its retail immunity outside a run.
--
-- Retail addresses are guest addresses from the generated hook catalog; table
-- bases are read from the game's own gp globals at use.

local GP = 0x007690F0
local function gp(offset: number): number
    return GP + offset - 0x10000
end
local SKILL_TABLE = gp(0xb3b8) -- pointer to 0x28-byte skill rows
local ENEMY_ROWS = gp(0xb3c4) -- pointer to 0x3C-byte enemy rows: level u8 +3, luck u8 +0xC
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
    datCalcGetLevelById = seam("datCalcGetLevelById", 0x00104c70, "ii"),
    btlAISelectWeightedEntry = seam("btlAISelectWeightedEntry", 0x001dea90, "ppI"),
    btlAICommandPickTableAction = seam("btlAICommandPickTableAction", 0x001de800, "Bp"),
    btlAICommandSetActionCode = seam("btlAICommandSetActionCode", 0x001de640, "vppH"),
    -- The boss redesigns' script steering (design §4.A/§4.B): the enemy script's
    -- own random draw, condition queries and flags, read and answered through
    -- the script VM's parameter and return slots.
    scrGetAiState = seam("scrGetAiState", 0x0029d050, "i"),
    scrGetParamInt = seam("scrGetParamInt", 0x0029cc00, "ii"),
    scrSetReturnInt = seam("scrSetReturnInt", 0x0029cf50, "vi"),
    datSetFlag = seam("datSetFlag", 0x00106390, "vii"),
    datSetBitmapBit = seam("datSetBitmapBit", 0x0010f3d0, "vII"),
    datCalcHasBuffMask = seam("datCalcHasBuffMask", 0x002340c0, "ipi"),
    btlAiRandom = seam("btlAiRandom", 0x001eb2a0, "i"),
    queryMyHp = seam("btlAICommandScriptQueryMyHp", 0x001df310, "i"),
    queryMore = seam("btlAICommandScriptQueryMore", 0x001e0f60, "i"),
    queryEnBadAll = seam("btlAICommandScriptQueryEnBadAll", 0x001dfc00, "i"),
    queryMyUseSkil = seam("btlAICommandScriptQueryMyUseSkil", 0x001e0ab0, "i"),
    queryFrHojo = seam("btlAICommandScriptQueryFrHojo", 0x001dfe10, "i"),
    selfHasBuff = seam("btlAICommandScriptSelfHasBuff", 0x001e43d0, "i"),
    sideIdQueryMyHp = seam("btlAICommandSideIdQueryMyHp", 0x001e1530, "I"),
    sideIdQueryMyBad = seam("btlAICommandSideIdQueryMyBad", 0x001e1800, "I"),
    sideIdQueryHasBuff = seam("btlAICommandSideIdQueryHasBuff", 0x001e1940, "i"),
    setActionParams = seam("btlEffectScriptSetActionParams", 0x001ea220, "i"),
    -- Formation cue: the battle's top-centre panel in raw-text mode, raised
    -- once at the end of the formation action.
    btlActionEndInit = seam("btlActionEndInit", 0x001afa50, "vp"),
    btlPanelStartEntry = seam("btlPanelStartEntry", 0x00216b40, "vii"),
    btlPanelMarkEntryStarted = seam("btlPanelMarkEntryStarted", 0x00216d10, "vi"),
    btlPanelSetCountdown = seam("btlPanelSetCountdown", 0x00216c40, "vii"),
}

-- DatUnit: flags u16 +0 (bit 2: enemy), id u16 +2, hp u16 +8, sp u16 +0xA,
-- bad u32 +0xC (bit 0x80000: KO), charges u32 +0x14 (Power Charge 0x40000,
-- Mind Charge 0x80000). BtlAction.unit +0x30. BtlUnit: genus u8 +0xA2 (1
-- enemy), charId u16 +0xA4, datUnit +0xA64, nextUnit +0xA6C. The battle
-- state's genus lists are head/tail pairs from +0x178; its encounter record
-- +0xC68 has the encounter ID at +8.
local UNIT_ENEMY_FLAG = 4
local UNIT_KO = 0x80000
local ENEMY_LIST_HEAD = 0x178 + 8
local BATTLE_RECORD = 0xC68
local AI_ROW_SIZE = 0xA4
local AI_GROUP_SIZE = 0x28
local AI_ENTRIES = 0x2C
local SKILL_ROW_SIZE = 0x28
local ENEMY_ROW_SIZE = 0x3C
local RETAIL_ACTIVE_SKILLS = 0x1B8 -- metadata rows a retail AI table command can name
local SKILL_KIND_HP_RECOVERY = 2 -- skill row +0x11
local PROTAGONIST = 1 -- datCalcGetLevelById's playable ID

-- Technical pairs (design §8): an ailment already on the target and the element
-- of a normal hit. Masks of the DatUnit ailment word, joined from real skills
-- (Pulinpa/Tentarafoo 0x10 Panic, Evil Touch/Smile 0x04 Fear, Makajam/Foolish
-- Whisper 0x08 Silence, Rage 0x02 by its doubling in 0x00235520); element IDs
-- of the retail element catalog (src/platform/mods/game_tables.c).
local AILMENT_MASKS: {[string]: number} = {rage = 0x02, fear = 0x04, silence = 0x08, panic = 0x10}
local ELEMENT_IDS: {[string]: number} = {fire = 1, ice = 2, electric = 3, wind = 4}
local TECHNICAL_PRIORITY = 100 -- multiplicative; challenge rules declare lower priorities
local BOSS_PRIORITY = 50 -- multiplicative, like Technical
local PRESSURE_PRIORITY = 40 -- multiplicative: epoch and floor-boss damage of enemy hits
local WINDOW_PRIORITY = 50 -- the status chance stage: a guarded window boss's roll is x0

-- Boss tactics and redesigns (design §4 bindings, traced from the player's disc:
-- BTL.CVM AICALC block 10). Enemy IDs are DatUnit IDs; skills are native skill
-- IDs every action code names.
local KNIGHT = 310 -- Avenger Knight, encounter 811: AICALC table row only
local YUKIKO = 258 -- Shadow Yukiko, encounter 514 (0x202): enemy script AI_yukiko
local PRINCE = 269 -- Charming Prince, summoned by AI_yukiko: enemy script AI_ouji
local KANJI = 259 -- Shadow Kanji, encounter 515 (0x203): enemy script enemyAI_d_kanji
local NICE_GUY = 272 -- enemyAI_aniki_A: Dia/Diarama, Mutudi, White Wall, Heat Riser, Tarukaja, Dekunda
local TOUGH_GUY = 273 -- enemyAI_aniki_B: Kill Rush, Rampage, Dekaja
local YUKIKO_ENCOUNTER = 0x202
local KANJI_ENCOUNTER = 0x203
local POWER_CHARGE = 227 -- self: the next physical hit is charged (DatUnit +0x14 0x40000)
local GALE_SLASH = 145 -- physical, every party member
local MIND_CHARGE = 228 -- self: the next magic hit is charged
local BURN_TO_ASHES = 354 -- Yukiko's fire burst on the whole party
local MATARUNDA = 212 -- Yukiko's own attack debuff on the whole party (AI_yukiko_25)
local SWIFT_STRIKE = 126 -- Kanji's physical hits on the whole party
local FATAL_END = 128 -- Kanji's single-target physical
local TETRAKARN = 229 -- Kanji's physical-repelling barrier on himself (AI_kanji_25)
local SPIRIT_DRAIN, ROAR_OF_WRATH = 102, 359 -- what AI_kanji_25 asks he used last
local FANATICAL_SPARK = 360 -- Kanji's electric hit on the whole party
local DIA, DIARAMA = 192, 193
local ICE = 2 -- retail element ID: the Prince's weakness
-- The battle AI script flags (BIT 448.. in AICALC block 10), which btlInit
-- clears for every battle. AI_yukiko: first summon done, resummon done.
-- enemyAI_d_kanji: opening Power Charge, Forbidden Murmur at 75%, Fanatical
-- Spark at 50% and at 25%, Spirit Drain pending. AI_ouji: Diarama given.
local FLAG_BIT_448, FLAG_BIT_449, FLAG_BIT_450, FLAG_BIT_451, FLAG_BIT_452 = 0x15C0, 0x15C1, 0x15C2, 0x15C3, 0x15C4
local FLAG_PRINCE_HEAL = 0x15D3 -- BIT 467
-- Mind (Yukiko) and Power (Kanji) Charge at DatUnit +0x14.
local CHARGE_BITS: {[number]: number} = {[YUKIKO] = 0x80000, [KANJI] = 0x40000}
-- A boss under rage, fear, silence or panic is not steered: its turn is the
-- party's window, and the rotation waits (no free catch-up).
local DISABLING = 0x1E

-- Annotated: unannotated string-literal arrays are inferred as unions of
-- singletons, which the type solver cannot finish once they are concatenated.
local FIELDS: {string} = {"hp", "sp", "st", "ma", "en", "ag", "lu", "exp", "yen", "damage"}
local CLASS_FIELDS: {[string]: {string}} = {
    -- An ordinary enemy's hp and damage come from the run's epoch.
    ordinary = {"sp", "st", "ma", "en", "ag", "lu", "exp", "yen"},
    boss = FIELDS,
}
local EPOCH_FIELDS: {string} = {"hp", "damage"}
local STAT_FIELDS: {string} = {"st", "ma", "en", "ag", "lu"} -- datCalcGetUnitStat index order
local MODES: {string} = {"hard", "harder"}
local MODE_LABELS: {[string]: string} = {hard = "HARD", harder = "HARDER"}
local CLASSES: {string} = {"ordinary", "boss"}

type Fields = {[string]: number}
type Technical = {factor: number, masks: {[number]: number}} -- element ID -> ailment mask
type Epoch = {encounter: number, enemy: number, values: {[string]: Fields}} -- mode -> hp/damage
type Windows = {turns: number, immuneTurns: number, cap: number, bosses: {[number]: number}} -- enemy ID -> mask
type Guards = {skills: {[number]: boolean}, bands: {[number]: number}}
type Overrides = {retail: {[string]: Fields}, golden: {[string]: Fields}} -- baseline -> mode -> fields
-- Boss tactics (Knight) and the redesigns' thresholds, cadences and burst sizes.
type Bosses = {
    knightCycle: number, knightSweep: {[string]: number}, yukikoBurst: {[string]: number},
    yukikoPrinceAt: number, yukikoFinalAt: number, princeHealTurns: number,
    kanjiFormationAt: number, kanjiFinalAt: number, kanjiHealBelow: number, kanjiHealTurns: number,
    kanjiStrike: {[string]: number}, kanjiSpark: {[string]: number}, kanjiFinisher: {[string]: number},
}
type Tuning = {
    revision: number,
    bossFlags: number,
    modes: {[string]: {[string]: Fields}},
    epochs: {Epoch},
    skipHealAbove: number,
    rescueBelow: number,
    technical: Technical,
    bosses: Bosses,
    windows: Windows,
    guards: Guards,
    enemy: {[number]: Overrides},
}
type Run = {mode: string, stats: boolean, ai: boolean, rewards: boolean, technical: boolean, bosses: boolean,
    revision: number, adopted: boolean, physsplit: string?, epoch: number}

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

local function checkWhole(value: number, where: string)
    if value % 1 ~= 0 then
        error(where .. ": must be a whole number")
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

local function takeSection(sections: {[string]: {[string]: string}}, name: string): {[string]: string}
    local keys = sections[name]
    if keys == nil then
        error(TUNING_PATH .. ": missing [" .. name .. "]")
    end
    sections[name] = nil
    return keys
end

local function takeFactor(keys: {[string]: string}, key: string, where: string): number
    local value = takeNumber(keys, key, where)
    checkRange(value, 0.25, 4.0, string.format("%s [%s] %s", TUNING_PATH, where, key))
    return value
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

    local classes = takeSection(sections, "classes")
    local bossFlags = takeNumber(classes, "boss_flags", "classes")
    checkRange(bossFlags, 1, 0xffffffff, TUNING_PATH .. " boss_flags")
    noLeftovers(classes, "classes")

    local modes: {[string]: {[string]: Fields}} = {}
    for _, mode in MODES do
        local keys = takeSection(sections, mode)
        local byClass: {[string]: Fields} = {}
        for _, group in CLASSES do
            local fields: Fields = {}
            for _, field in CLASS_FIELDS[group] do
                fields[field] = takeFactor(keys, group .. "." .. field, mode)
            end
            byClass[group] = fields
        end
        noLeftovers(keys, mode)
        modes[mode] = byClass
    end

    -- [epoch 1] .. [epoch N], consecutive.
    local epochs: {Epoch} = {}
    while sections["epoch " .. (#epochs + 1)] ~= nil do
        local name = "epoch " .. (#epochs + 1)
        local keys = takeSection(sections, name)
        local encounter = takeNumber(keys, "encounter", name)
        local enemy = takeNumber(keys, "enemy", name)
        checkRange(encounter, 1, 0xffff, TUNING_PATH .. " [" .. name .. "] encounter")
        checkRange(enemy, 1, 0xffff, TUNING_PATH .. " [" .. name .. "] enemy")
        checkWhole(encounter, TUNING_PATH .. " [" .. name .. "] encounter")
        checkWhole(enemy, TUNING_PATH .. " [" .. name .. "] enemy")
        local values: {[string]: Fields} = {}
        for _, mode in MODES do
            local fields: Fields = {}
            for _, field in EPOCH_FIELDS do
                fields[field] = takeFactor(keys, mode .. "." .. field, name)
            end
            values[mode] = fields
        end
        noLeftovers(keys, name)
        table.insert(epochs, {encounter = encounter, enemy = enemy, values = values})
    end
    if #epochs == 0 then
        error(TUNING_PATH .. ": missing [epoch 1]")
    end

    local ai = takeSection(sections, "ai")
    local skipHealAbove = takeNumber(ai, "skip_heal_above", "ai")
    local rescueBelow = takeNumber(ai, "rescue_below", "ai")
    checkRange(skipHealAbove, 0, 1, TUNING_PATH .. " skip_heal_above")
    checkRange(rescueBelow, 0, 1, TUNING_PATH .. " rescue_below")
    if rescueBelow >= skipHealAbove then
        error(TUNING_PATH .. ": rescue_below must be below skip_heal_above")
    end
    noLeftovers(ai, "ai")

    local technicalKeys = takeSection(sections, "technical")
    local factor = takeNumber(technicalKeys, "factor", "technical")
    checkRange(factor, 1, 2, TUNING_PATH .. " [technical] factor")
    local masks: {[number]: number} = {}
    for ailment, mask in AILMENT_MASKS do
        local name = technicalKeys[ailment]
        if name ~= nil then
            technicalKeys[ailment] = nil
            local element = ELEMENT_IDS[name]
            if element == nil then
                error(string.format("%s: [technical] %s pairs with unknown element %s", TUNING_PATH, ailment, name))
            elseif masks[element] ~= nil then
                error(string.format("%s: [technical] element %s is paired twice", TUNING_PATH, name))
            end
            masks[element] = mask
        end
    end
    noLeftovers(technicalKeys, "technical")

    local bossValues = takeSection(sections, "bosses")
    local knightCycle = takeNumber(bossValues, "knight.cycle", "bosses")
    checkRange(knightCycle, 2, 8, TUNING_PATH .. " [bosses] knight.cycle")
    checkWhole(knightCycle, TUNING_PATH .. " [bosses] knight.cycle")
    local function bossFactor(key: string): number
        local value = takeNumber(bossValues, key, "bosses")
        checkRange(value, 0.25, 2.0, string.format("%s [bosses] %s", TUNING_PATH, key))
        return value
    end
    local function bossFraction(key: string, low: number, high: number): number
        local value = takeNumber(bossValues, key, "bosses")
        checkRange(value, low, high, string.format("%s [bosses] %s", TUNING_PATH, key))
        return value
    end
    local function bossTurns(key: string): number
        local value = takeNumber(bossValues, key, "bosses")
        checkRange(value, 1, 8, string.format("%s [bosses] %s", TUNING_PATH, key))
        checkWhole(value, string.format("%s [bosses] %s", TUNING_PATH, key))
        return value
    end
    local knightSweep: {[string]: number} = {}
    local yukikoBurst: {[string]: number} = {}
    local kanjiStrike: {[string]: number} = {}
    local kanjiSpark: {[string]: number} = {}
    local kanjiFinisher: {[string]: number} = {}
    for _, mode in MODES do
        knightSweep[mode] = bossFactor(mode .. ".knight_sweep")
        yukikoBurst[mode] = bossFactor(mode .. ".yukiko_burst")
        kanjiStrike[mode] = bossFactor(mode .. ".kanji_strike")
        kanjiSpark[mode] = bossFactor(mode .. ".kanji_spark")
        kanjiFinisher[mode] = bossFactor(mode .. ".kanji_finisher")
    end
    local yukikoPrinceAt = bossFraction("yukiko.prince_at", 0.05, 0.95)
    local yukikoFinalAt = bossFraction("yukiko.final_at", 0.05, yukikoPrinceAt)
    local kanjiFormationAt = bossFraction("kanji.formation_at", 0.05, 0.95)
    local kanjiFinalAt = bossFraction("kanji.final_at", 0.05, kanjiFormationAt)
    local bosses: Bosses = {knightCycle = knightCycle, knightSweep = knightSweep, yukikoBurst = yukikoBurst,
        yukikoPrinceAt = yukikoPrinceAt, yukikoFinalAt = yukikoFinalAt,
        princeHealTurns = bossTurns("yukiko.prince_heal_turns"),
        kanjiFormationAt = kanjiFormationAt, kanjiFinalAt = kanjiFinalAt,
        kanjiHealBelow = bossFraction("kanji.heal_below", 0.05, 0.95),
        kanjiHealTurns = bossTurns("kanji.heal_turns"),
        kanjiStrike = kanjiStrike, kanjiSpark = kanjiSpark, kanjiFinisher = kanjiFinisher}
    noLeftovers(bossValues, "bosses")

    local windowKeys = takeSection(sections, "windows")
    local turns = takeNumber(windowKeys, "turns", "windows")
    local immuneTurns = takeNumber(windowKeys, "immune_turns", "windows")
    local cap = takeNumber(windowKeys, "cap", "windows")
    checkRange(turns, 1, 4, TUNING_PATH .. " [windows] turns")
    checkRange(immuneTurns, 0, 8, TUNING_PATH .. " [windows] immune_turns")
    checkRange(cap, 1, 8, TUNING_PATH .. " [windows] cap")
    for _, value in {turns, immuneTurns, cap} do
        checkWhole(value, TUNING_PATH .. " [windows]")
    end
    local windowBosses: {[number]: number} = {}
    for key, text in windowKeys do
        local idText = string.match(key, "^enemy%.(%d+)$")
        local id = if idText then tonumber(idText) else nil
        local mask = parseNumber(text, string.format("%s [windows] %s", TUNING_PATH, key))
        local known = false
        for _, ailment in AILMENT_MASKS do
            known = known or ailment == mask
        end
        if id == nil or not known then
            error(string.format("%s: [windows] %s must be enemy.<ID> = one Technical ailment mask", TUNING_PATH, key))
        end
        windowBosses[id :: number] = mask
    end

    local guardKeys = takeSection(sections, "guards")
    local guardSkills: {[number]: boolean} = {}
    local guardBands: {[number]: number} = {}
    for key, text in guardKeys do
        local kind, idText = string.match(key, "^(%a+)%.(%w+)$")
        local id = if idText then parseNumber(idText :: string, string.format("%s [guards] %s", TUNING_PATH, key)) else nil
        local value = parseNumber(text, string.format("%s [guards] %s", TUNING_PATH, key))
        if kind == "skill" and id ~= nil and value == 1 then
            guardSkills[id] = true
        elseif kind == "band" and id ~= nil and windowBosses[id] ~= nil then
            checkRange(value, 2, 8, string.format("%s [guards] %s", TUNING_PATH, key))
            checkWhole(value, string.format("%s [guards] %s", TUNING_PATH, key))
            guardBands[id] = value
        else
            error(string.format("%s: [guards] %s must be skill.<ID> = 1 or band.<window boss ID> = 2..8", TUNING_PATH, key))
        end
    end

    -- [enemy <ID>]: <mode>.<field> for the retail row, golden.<mode>.<field>
    -- for Golden's.
    local enemy: {[number]: Overrides} = {}
    for name, keys in sections do
        local idText = string.match(name, "^enemy (%d+)$")
        if idText == nil then
            error(TUNING_PATH .. ": unknown section [" .. name .. "]")
        end
        local id = tonumber(idText) :: number
        checkRange(id, 0, 0xffff, TUNING_PATH .. " [" .. name .. "]")
        local overrides: Overrides = {retail = {}, golden = {}}
        for key, text in keys do
            local golden, mode, field = string.match(key, "^(golden%.)(%a+)%.(%a+)$")
            if golden == nil then
                mode, field = string.match(key, "^(%a+)%.(%a+)$")
            end
            if mode == nil or field == nil or MODE_LABELS[mode :: string] == nil or not table.find(FIELDS, field :: string) then
                error(string.format("%s: [%s] has unknown key %s", TUNING_PATH, name, key))
            end
            local value = parseNumber(text, string.format("%s [%s] %s", TUNING_PATH, name, key))
            checkRange(value, 0.25, 4.0, string.format("%s [%s] %s", TUNING_PATH, name, key))
            local byMode = if golden then overrides.golden else overrides.retail
            local fields = byMode[mode :: string] or {}
            fields[field :: string] = value
            byMode[mode :: string] = fields
        end
        enemy[id] = overrides
    end

    return {revision = revision, bossFlags = bossFlags, modes = modes, epochs = epochs, skipHealAbove = skipHealAbove,
        rescueBelow = rescueBelow, technical = {factor = factor, masks = masks},
        bosses = bosses,
        windows = {turns = turns, immuneTurns = immuneTurns, cap = cap, bosses = windowBosses},
        guards = {skills = guardSkills, bands = guardBands}, enemy = enemy}
end

local tuningData: Tuning = loadTuning()
log(string.format("HARDMODE_TUNING revision=%d boss_flags=0x%x epochs=%d", tuningData.revision, tuningData.bossFlags,
    #tuningData.epochs))
local function tuning(): Tuning
    return tuningData
end

-- The baseline the enemy rows come from: p4pc.golden (a companion) publishes
-- its admitted groups; without it, or with its ENEMIES group off, the rows are
-- retail and only the retail-fit overrides apply.
local goldenEnemies = false
do
    local groups = p4.contract.read("p4pc.golden", "groups")
    if type(groups) == "table" and (groups :: {[any]: any}).enemies == true then
        goldenEnemies = true
    end
end
log("HARDMODE_BASELINE enemies=" .. (if goldenEnemies then "golden" else "retail"))

-- Run state -------------------------------------------------------------------

local run: Run? = nil
local currentBoss = false -- class of the encounter being built or fought
local unitBoss: {[number]: boolean} = {} -- DatUnit address -> boss class
local rewardLogged: {[string]: boolean} = {}
local knightTurns: {[number]: number} = {} -- the Knight's base turns, per DatUnit; cleared per encounter
-- Boss redesigns (design §4.A/§4.B), per enemy DatUnit; cleared per encounter.
-- A state is the unit's phase, its position in the phase's base-turn rotation
-- and its once-only entries; a plan is the steering of the one script
-- evaluation in progress, from its first steered query to its action.
-- `armed`: Kanji's P3 Power Charge primes the finisher (not his ordinary strike).
type BossState = {phase: number, step: number, turns: number, summoned: boolean, formed: boolean,
    armed: boolean, finished: boolean, last: string, lastHeal: number?}
-- `update`: the game update (p4.clock.updates) its evaluation began in. A script
-- evaluation runs inside one update; a plan from an earlier one is stale (a
-- query asked after that turn's action was set) and never steers a later turn.
type Plan = {steer: boolean, enemy: number, extra: boolean, beat: string, cyclic: boolean, wants: number?,
    random: number?, hp: number?, more: number?, selfBuff: number?, enBadAll: number?, heal: number?,
    useSkill: {[number]: number}?, update: number?}
local bossStates: {[number]: BossState} = {}
local plans: {[number]: Plan} = {}
local steeringEncounter = 0 -- YUKIKO_ENCOUNTER or KANJI_ENCOUNTER while one is fought
local finisherArmed = false -- Kanji's finisher is the next Fatal End (damage stage)
local kanjiSweptSinceTough = false -- Kanji hit the whole party since Tough Guy's last action
local pendingBanners: {[number]: string} = {} -- BtlAction -> formation cue for its turn's end
-- Boss Technical windows (design §4.D), per boss DatUnit; cleared per encounter.
type Window = {held: number, immune: number, count: number, mask: number, band: number?}
local windows: {[number]: Window} = {}
-- Read by the chance stage (snapshot mode: no memory reads there): whether a
-- window boss, by enemy ID, shrugs its ailment off until its next turn.
local windowProtected: {[number]: boolean} = {}
local windowSeed = 0

local function enemyLevel(enemy: number): number
    local rows = p4.mem.read("u32", ENEMY_ROWS)
    return if rows == 0 then 0 else p4.mem.read("u8", rows + enemy * ENEMY_ROW_SIZE + 3)
end

-- A New Game starts in the first epoch; an adopted save in the first whose
-- principal boss is above the protagonist's level.
local function startingEpoch(adopted: boolean): number
    local epochs = tuning().epochs
    if not adopted then
        return 1
    end
    local level = p4.call(A.datCalcGetLevelById.address, A.datCalcGetLevelById.signature, PROTAGONIST) :: number
    local epoch = 1
    while epochs[epoch] ~= nil and enemyLevel(epochs[epoch].enemy) <= level do
        epoch += 1
    end
    return epoch
end

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
        technical = p4.settings.get("technical") == true,
        bosses = p4.settings.get("boss_rules") == true,
        revision = tuning().revision,
        adopted = adopted,
        -- Hard mode requires Physical Split (mod.ini [require]): the run
        -- records the version it starts with.
        physsplit = p4.mods.required("p4pc.physsplit").version,
        epoch = startingEpoch(adopted),
    }
end

local function encodeRun(current: Run): P4SaveValue
    return {mode = current.mode, stats = current.stats, ai = current.ai, rewards = current.rewards,
        technical = current.technical, bosses = current.bosses, revision = current.revision, adopted = current.adopted,
        physsplit = current.physsplit, epoch = current.epoch}
end

-- An epoch above the last one is the post-game; the last epoch's values stay.
local EPOCH_LIMIT = 64

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
    -- Runs saved before Technical, the boss tactics, the Physical Split
    -- requirement or the epochs existed carry no such field: they stay without
    -- the first ones, and take their epoch from the protagonist's level.
    for _, key in {"technical", "bosses"} do
        if t[key] ~= nil and type(t[key]) ~= "boolean" then
            return nil, "the hard-mode block has no valid " .. key
        end
    end
    if t.physsplit ~= nil and (type(t.physsplit) ~= "string" or #t.physsplit == 0 or #t.physsplit > 32) then
        return nil, "the hard-mode block has no valid physsplit version"
    end
    if t.epoch ~= nil and (type(t.epoch) ~= "number" or t.epoch < 1 or t.epoch > EPOCH_LIMIT or t.epoch % 1 ~= 0) then
        return nil, "the hard-mode block has no valid epoch"
    end
    if type(t.revision) ~= "number" or t.revision < 1 or t.revision % 1 ~= 0 then
        return nil, "the hard-mode block has no valid tuning revision"
    end
    for key in t do
        if key ~= "mode" and key ~= "stats" and key ~= "ai" and key ~= "rewards" and key ~= "technical"
            and key ~= "bosses" and key ~= "revision" and key ~= "adopted" and key ~= "physsplit" and key ~= "epoch" then
            return nil, "the hard-mode block has an unknown field " .. tostring(key)
        end
    end
    return {mode = t.mode, stats = t.stats, ai = t.ai, rewards = t.rewards, technical = t.technical == true,
        bosses = t.bosses == true, revision = t.revision, adopted = t.adopted, physsplit = t.physsplit,
        epoch = if t.epoch ~= nil then t.epoch else 0}, nil
end

local function describeRun(current: Run): string
    local features = {}
    if current.stats then table.insert(features, "stats") end
    if current.ai then table.insert(features, "ai") end
    if current.rewards then table.insert(features, "rewards") end
    if current.technical then table.insert(features, "technical") end
    if current.bosses then table.insert(features, "bosses") end
    return string.format("mode=%s features=%s revision=%d adopted=%s physsplit=%s epoch=%d", current.mode,
        if #features > 0 then table.concat(features, ",") else "none", current.revision, tostring(current.adopted),
        current.physsplit or "none", current.epoch)
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

-- One enemy's multiplier for one field: its own override for the run's
-- difficulty (Golden's row has its own), else its epoch's (an ordinary
-- enemy's hp and damage), else its class's. Pure: also used in snapshot mode.
local function factorFor(current: Run, enemy: number, boss: boolean, field: string): number
    local data = tuning()
    local overrides = data.enemy[enemy]
    if overrides then
        local byMode = if goldenEnemies then overrides.golden else overrides.retail
        local fields = byMode[current.mode]
        if fields and fields[field] ~= nil then
            return fields[field]
        end
    end
    if not boss and (field == "hp" or field == "damage") then
        local epochs = data.epochs
        local epoch = epochs[math.min(current.epoch, #epochs)]
        return epoch.values[current.mode][field]
    end
    return data.modes[current.mode][if boss then "boss" else "ordinary"][field]
end

local function multiplier(current: Run, unit: number, field: string): number
    return factorFor(current, p4.mem.read("u16", unit + 2), isBoss(unit), field)
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
    table.clear(unitBoss)
    table.clear(rewardLogged)
    table.clear(knightTurns)
    table.clear(bossStates)
    table.clear(plans)
    finisherArmed = false
    kanjiSweptSinceTough = false
    table.clear(pendingBanners)
    table.clear(windows)
    table.clear(windowProtected)
    windowSeed = bit32.bor(p4.clock.retail() % 0x7fffffff, 1)
    local base = p4.mem.read("u32", ENCOUNTER_TABLE)
    local encounter = context.args[1] :: number
    local flags = if base ~= 0 then p4.mem.read("u32", base + encounter * 0x18) else 0
    currentBoss = bit32.band(flags, tuning().bossFlags) ~= 0
    steeringEncounter = if encounter == YUKIKO_ENCOUNTER or encounter == KANJI_ENCOUNTER then encounter else 0
    local current = run
    log(string.format("HARDMODE_ENCOUNTER id=%d boss=%s epoch=%d", encounter, tostring(currentBoss),
        if current then current.epoch else 0))
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
    -- The five stats are scaled where they are read: log the original (unhooked)
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

-- Enemy pressure: the HP damage of every hit an enemy deals (not a reflected
-- one, the party's own attack) scales by its epoch (ordinary encounters) or
-- its own factor (the floor bosses). The encounter's class covers every unit
-- in it, summons included. A pure function of the hit and the run.
local pressureModifier = p4.damage.on({kind = "multiply", priority = PRESSURE_PRIORITY,
    fn = function(hit: P4DamageHit): number?
    local current = run
    if current == nil or hit.source_side ~= "enemy" or hit.reflected then
        return nil
    end
    local factor = factorFor(current, hit.source_id, currentBoss, "damage")
    return if factor ~= 1 then factor else nil
end})

-- Enemy support discipline ------------------------------------------------------

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

-- What one table group of an enemy's AI row can do: "heal" when every
-- positive-weight entry is an HP recovery, "mixed" with both, "other" with none.
local function groupKind(row: number, group: number): string
    local heals, others = 0, 0
    for index = 0, 4 do
        local entry = row + AI_ENTRIES + group * AI_GROUP_SIZE + index * 8
        if p4.mem.read("u8", entry) > 0 then
            if isHpRecovery(p4.mem.read("u16", entry + 2)) then heals += 1 else others += 1 end
        end
    end
    if heals > 0 then
        return if others > 0 then "mixed" else "heal"
    end
    return if others > 0 then "other" else "empty"
end

local function firstGroup(kinds: {string}, wanted: string, fallback: string): number?
    for _, kind in {wanted, fallback} do
        for group = 0, 2 do
            if kinds[group + 1] == kind then
                return group
            end
        end
    end
    return nil
end

-- The game chooses which of an enemy's three table groups it acts from through
-- two rows of conditions (btlAICommandPickTableAction 0x001DE800). Retail rows
-- usually keep heals in a heal-only group behind a condition, so the rule acts
-- here too: the conditions always run, and only the group they chose may be
-- exchanged. A heal-only group while every living ally is above
-- skip_heal_above becomes the first group without a heal; a group without a
-- heal while a living ally is below rescue_below becomes the first heal-only
-- (else mixed) group. The weighted pick inside the group then runs natively.
local groupHook = p4.hook(A.btlAICommandPickTableAction.address, A.btlAICommandPickTableAction.signature, "after",
    function(context)
    local current = run
    local native = context.result :: number
    if current == nil or native > 2 then
        return -- not a table group
    end
    local actor = p4.mem.read("u32", (context.args[1] :: number) + 0x30)
    if actor == 0 or p4.mem.read("u8", actor + 0xA2) ~= 1 or isBoss(p4.mem.read("u32", actor + 0xA64)) then
        return
    end
    local rows = p4.mem.read("u32", ENEMY_AI_TABLE)
    if rows == 0 then
        return
    end
    local enemyId = p4.mem.read("u16", actor + 0xA4)
    local row = rows + enemyId * AI_ROW_SIZE
    local kinds: {string} = {groupKind(row, 0), groupKind(row, 1), groupKind(row, 2)}
    local data = tuning()
    local lowest, highest = allyHealth(current)
    local chosen: number? = nil
    local reason = ""
    if kinds[native + 1] == "heal" and lowest > data.skipHealAbove then
        chosen, reason = firstGroup(kinds, "other", "mixed"), "skip_heal"
    elseif kinds[native + 1] == "other" and lowest < data.rescueBelow then
        chosen, reason = firstGroup(kinds, "heal", "mixed"), "rescue"
    end
    if chosen ~= nil and chosen ~= native then
        context.result = chosen
        log(string.format("HARDMODE_AIGROUP enemy=%d native=%d chosen=%d reason=%s lowest=%.2f highest=%.2f",
            enemyId, native, chosen :: number, reason, lowest, highest))
    end
end)

-- Technical damage -------------------------------------------------------------

-- A normal hit (no weakness or critical: the game's own bonus stands instead)
-- of an element paired with an ailment the target already had before this
-- action, from either side. Repelled hits keep their stock value. The stage
-- only offers formula HP damage that lands, so fixed, percentage, scripted,
-- recovery and SP amounts never reach this. The callback runs in snapshot mode,
-- where even p4.log is refused: resolved hits are noted in Lua state and logged
-- at the next frame.
local TECHNICAL_NOTES_MAX = 32
local technicalNotes: {string} = {}
local technicalDropped = 0
local technicalModifier = p4.damage.on({kind = "multiply", priority = TECHNICAL_PRIORITY,
    fn = function(hit: P4DamageHit): number?
    local element = hit.element
    if hit.hit_kind ~= "normal" or hit.reflected or element == nil then
        return nil
    end
    local data = tuning().technical
    local mask = data.masks[element]
    if mask == nil or bit32.band(hit.target_status, mask) == 0 then
        return nil
    end
    if hit.resolved then
        if #technicalNotes < TECHNICAL_NOTES_MAX then
            table.insert(technicalNotes, string.format(
                "HARDMODE_TECHNICAL skill=%d element=%d status=0x%x target=%s:%d stock=%d factor=%.2f",
                hit.skill, element, hit.target_status, hit.target_side, hit.target_id, hit.stock, data.factor))
        else
            technicalDropped += 1
        end
    end
    return data.factor
end})
local technicalLog = p4.events.on("frame_start", function(event)
    if #technicalNotes == 0 and technicalDropped == 0 then
        return
    end
    for _, note in technicalNotes do
        log(note)
    end
    if technicalDropped > 0 then
        log(string.format("HARDMODE_TECHNICAL dropped=%d (more than %d in one frame)", technicalDropped,
            TECHNICAL_NOTES_MAX))
    end
    table.clear(technicalNotes)
    technicalDropped = 0
end)

-- Boss Technical windows (design §4.D) -------------------------------------------

-- data/windows.xml (table source, restart setting `windows`) makes one
-- Technical-set ailment slot per window boss susceptible; [windows] in the
-- tuning names the same masks. Every roll of that ailment on such a boss passes
-- the chance stage below: outside a hard-mode run with Technical the roll is x0
-- (the boss keeps its retail immunity), and inside one it is x0 while the boss
-- is guarded. The guard state is computed at each of the boss's own turns,
-- where the landing is also detected and worn off.

-- SMT V's ailment recovery Base for this enemy (15 x Luck / (4 + (Level - 1) / 2),
-- clamped 7.5..85), from its own row.
local function wearOffBase(enemy: number): number
    local rows = p4.mem.read("u32", ENEMY_ROWS)
    if rows == 0 then
        return 7.5
    end
    local level = p4.mem.read("u8", rows + enemy * ENEMY_ROW_SIZE + 3)
    local luck = p4.mem.read("u8", rows + enemy * ENEMY_ROW_SIZE + 0xC)
    return math.clamp(15 * luck / (4 + (level - 1) / 2), 7.5, 85)
end

-- Deterministic per encounter (seeded from the game's own update counter); never
-- draws from the game's own random streams.
local function windowRoll(): number
    windowSeed = (windowSeed * 1103515245 + 12345) % 0x80000000
    return windowSeed % 100
end

local windowGuard = p4.chance.on({kind = "multiply", priority = WINDOW_PRIORITY,
    fn = function(hit: P4ChanceHit): number?
    if hit.target_side ~= "enemy" or hit.component ~= "ailment" then
        return nil
    end
    local mask = tuning().windows.bosses[hit.target_id]
    if mask == nil or bit32.band(hit.statuses, mask) == 0 then
        return nil
    end
    local current = run
    if current == nil or not current.technical or windowProtected[hit.target_id] then
        return 0
    end
    return nil
end})

-- At each of a window boss's own turns, with the action it will take: detect a
-- landing since its last turn, roll the wear-off and cure at the bound, run the
-- post-recovery immunity down, and decide whether it is guarded until its next
-- turn (immune, at the per-fight cap, a guard skill this turn, or the first
-- turn of an HP band).
local function windowTurn(current: Run, unit: number, enemy: number, chosen: number)
    local data = tuning()
    local mask = data.windows.bosses[enemy]
    if mask == nil then
        return
    end
    local w = windows[unit]
    if w == nil then
        w = {held = 0, immune = 0, count = 0, mask = 0, band = nil} :: Window
        windows[unit] = w
    end
    local bad = p4.mem.read("u32", unit + 0xC)
    if w.mask == 0 and bit32.band(bad, mask) ~= 0 then
        w.count += 1
        w.mask = mask
        w.held = 0
        log(string.format("HARDMODE_WINDOW enemy=%d ailment=0x%x landed count=%d", enemy, mask, w.count))
    end
    if w.mask ~= 0 then
        if bit32.band(bad, w.mask) ~= 0 then
            w.held += 1
            local chance = wearOffBase(enemy) + 15 * (w.held - 1)
            if w.held >= data.windows.turns or windowRoll() < chance then
                p4.mem.write("u32", unit + 0xC, bit32.band(bad, bit32.bnot(w.mask)))
                log(string.format("HARDMODE_WINDOW enemy=%d ailment=0x%x recovered turns=%d", enemy, w.mask, w.held))
                w.mask = 0
                w.immune = data.windows.immuneTurns
            end
        else
            log(string.format("HARDMODE_WINDOW enemy=%d ailment=0x%x recovered natively", enemy, w.mask))
            w.mask = 0
            w.immune = data.windows.immuneTurns
        end
    elseif w.immune > 0 then
        w.immune -= 1
    end
    local reason: string? = nil
    if data.guards.skills[chosen] then
        reason = string.format("skill %d", chosen)
    end
    local bands = data.guards.bands[enemy]
    if bands ~= nil then
        local maximum = maxHp(current, unit)
        local hp = p4.mem.read("u16", unit + 8)
        local band = if maximum > 0 then math.min(bands - 1, math.floor((1 - hp / maximum) * bands)) else 0
        if w.band ~= band then
            w.band = band
            reason = reason or string.format("band %d", band)
        end
    end
    if reason == nil and w.immune > 0 then
        reason = "immune"
    elseif reason == nil and w.count >= data.windows.cap then
        reason = "cap"
    end
    local protected = reason ~= nil
    if protected ~= (windowProtected[enemy] == true) then
        log(string.format("HARDMODE_GUARD enemy=%d %s", enemy, if protected then "on " .. (reason :: string) else "off"))
    end
    windowProtected[enemy] = protected
end

-- Boss tactics, redesigns and window turns ---------------------------------------

-- Every boss chooses an action through btlAICommandSetActionCode (0x001DE640):
-- the scripted bosses from their enemy scripts, the Knight through
-- btlTargetAIPickAction (0x001EB1D0), whose target condition then collects
-- targets for the skill actually set. A substitution is only ever a self
-- (charge) or all-party (burst, debuff) skill, so it never needs a target the
-- game's choice did not provide. The game's own choice and its random draws
-- always happen first. The window turn follows, with the action the boss will
-- actually take.
--
-- An action whose BtlAction +0x28 counter (btlCond_MORE) is above zero is the
-- One More of a weakness or critical, not a base turn: it never advances a
-- tactic or a rotation (design §4: count scheduler opportunities).
local function isExtra(action: number): boolean
    return p4.mem.read("u8", action + 0x28) > 0
end

local function substitute(context: P4HookContext, native: number, wanted: number?): (number, string?)
    if wanted == nil or wanted == native then
        return native, nil
    end
    -- The enemy AI carrier authorizes the authored command (with Physical
    -- Split's expanded routing it is not in the enemy's own offer) and the
    -- call writes args[3]. A refusal keeps the game's own choice.
    local ok, why = p4.battle.substitute(context.args[1] :: number, wanted :: number)
    if not ok then
        return native, why or "refused"
    end
    return wanted :: number, nil
end

-- The battle's top-centre panel (btlPanelStartEntry mode 0: raw game text,
-- the box that shows skill names) holds the formation cue until the next skill
-- or command name replaces it. It is raised once at the action's end, after
-- the native skill name. The battle state's HUD task is at +0xDD4; the text
-- lives in this mod's scratch. The panel's plate fits about 36 characters.
local BANNER_HOLD = -1 -- btlPanelSetCountdown: hold until replaced
local BANNER_CAPACITY = 48
local bannerText = p4.mem.alloc(BANNER_CAPACITY)

local function showBanner(text: string, why: string)
    local battle = p4.mem.read("u32", BATTLE)
    local ui = if battle ~= 0 then p4.mem.read("u32", battle + 0xDD4) else 0
    if ui == 0 then
        return
    end
    p4.mem.write_string(bannerText, BANNER_CAPACITY, text)
    p4.call(A.btlPanelStartEntry.address, A.btlPanelStartEntry.signature, ui, bannerText.address)
    p4.call(A.btlPanelMarkEntryStarted.address, A.btlPanelMarkEntryStarted.signature, ui)
    p4.call(A.btlPanelSetCountdown.address, A.btlPanelSetCountdown.signature, ui, BANNER_HOLD)
    log(string.format("HARDMODE_BANNER %s %s", why, text))
end

local bannerHook = p4.hook(A.btlActionEndInit.address, A.btlActionEndInit.signature, "after", function(context)
    local action = context.args[1] :: number
    local text = pendingBanners[action]
    if text ~= nil then
        pendingBanners[action] = nil
        showBanner(text, "end")
    end
end)

local function queueBanner(action: number, text: string)
    pendingBanners[action] = text
end

-- Avenger Knight: every knight.cycle-th base turn he visibly charges (Power
-- Charge) and his next base turn is Gale Slash on the whole party, so every
-- living party member acts between the warning and the sweep (the charge hits
-- nothing, so it never earns a One More). His own table pick otherwise.
local function knightTactic(context: P4HookContext, unit: number, native: number): number
    if isExtra(context.args[1] :: number) then
        log(string.format("HARDMODE_BOSS enemy=%d unit=0x%x turn=%d native=%d chosen=%d phase=0 beat=own extra",
            KNIGHT, unit, knightTurns[unit] or 0, native, native))
        return native
    end
    local cycle = tuning().bosses.knightCycle
    local turn = (knightTurns[unit] or 0) + 1
    knightTurns[unit] = turn
    local position = turn % (cycle + 1)
    local wanted: number? = nil
    local beat = "own"
    if position == cycle then
        wanted, beat = POWER_CHARGE, "charge"
    elseif position == 0 then
        wanted, beat = GALE_SLASH, "sweep"
    end
    local chosen, refused = substitute(context, native, wanted)
    log(string.format("HARDMODE_BOSS enemy=%d unit=0x%x turn=%d native=%d chosen=%d phase=0 beat=%s%s", KNIGHT,
        unit, turn, native, chosen, beat, if refused then " refused=" .. refused else ""))
    return chosen
end

-- Boss redesigns (design §4.A Shadow Yukiko, §4.B Shadow Kanji).
--
-- Yukiko (encounter 514) with her summoned Charming Prince, and Kanji with Nice
-- Guy and Tough Guy (encounter 515), keep their own enemy scripts (AI_yukiko,
-- AI_ouji, enemyAI_d_kanji, enemyAI_aniki_A/B), skills and native targets.
-- Each boss unit is a finite state (phase by HP, never going back, entered at
-- its next base turn; its position in the phase's base-turn rotation), and
-- each evaluation of its script is steered at the script's own decision
-- points, from its first steered query to the action it sets:
-- - its random draw (btlAiRandom) answers the rotation beat's branch;
-- - its condition queries answer for the beat: the HP band dispatch and HP
--   thresholds (QueryMyHp), One More (QueryMore), the charge test
--   (SelfHasBuff 0x400), the downed-party test (QueryEnBadAll) and the
--   defender's heal and cover queries;
-- - its battle script flags (btlInit clears them per battle) keep the retail
--   one-shot openers, Kanji's band-entry bursts and Yukiko's resummon closed;
-- - a self or all-party beat (charge, burst, recovery, formation change) the
--   steered script did not pick is set through p4.battle.substitute.
-- A boss under rage, fear, silence or panic is neither steered nor advanced:
-- that turn is the party's window and the rotation waits for it.

-- Base-turn rotations by phase. A rotation restarts at a phase entry, whose
-- entry action (the Prince's summon, Kanji's formation change) comes first; a
-- committed charge always resolves before anything else.
local YUKIKO_ROTATIONS: {{string}} = {
    {"fire", "physical", "announce", "burst", "recovery"}, -- P1: above yukiko.prince_at
    {"pressure", "announce", "burst", "recovery"}, -- P2: after the Prince's summon
    {"fire", "physical", "announce", "burst", "recovery"}, -- P3: at or below yukiko.final_at
}
local KANJI_ROTATIONS: {{string}} = {
    {"probe", "announce", "strike", "recovery"}, -- P1: above kanji.formation_at, both supports up
    {"electric", "announce", "strike", "recovery"}, -- P2
    {"pressure", "announce", "strike", "recovery"}, -- P3: the first charged strike is the finisher
}
local SUMMON_PRINCE = 356 -- AI_yukiko's first summon (btlEffectScriptSetActionParams, unit 269)

local function stateOf(unit: number): BossState
    local state = bossStates[unit]
    if state == nil then
        local fresh: BossState = {phase = 1, step = 0, turns = 0, summoned = false, formed = false, armed = false,
            finished = false, last = ""}
        bossStates[unit] = fresh
        return fresh
    end
    return state
end

local function newPlan(enemy: number, extra: boolean, beat: string): Plan
    return {steer = true, enemy = enemy, extra = extra, beat = beat, cyclic = false}
end

local function setFlag(flag: number, on: boolean)
    p4.call(A.datSetFlag.address, A.datSetFlag.signature, flag, if on then 1 else 0)
end

-- The living enemy-side DatUnit with this enemy ID, if any.
local function livingEnemy(enemy: number): number?
    local battle = p4.mem.read("u32", BATTLE)
    if battle == 0 then
        return nil
    end
    local node = p4.mem.read("u32", battle + ENEMY_LIST_HEAD)
    local guard = 0
    while node ~= 0 and guard < 16 do
        guard += 1
        local unit = p4.mem.read("u32", node + 0xA64)
        if unit ~= 0 and p4.mem.read("u16", unit + 2) == enemy and p4.mem.read("u16", unit + 8) > 0
            and bit32.band(p4.mem.read("u32", unit + 0xC), UNIT_KO) == 0 then
            return unit
        end
        node = p4.mem.read("u32", node + 0xA6C)
    end
    return nil
end

local function hpFraction(current: Run, unit: number): number
    local maximum = maxHp(current, unit)
    return if maximum > 0 then p4.mem.read("u16", unit + 8) / maximum else 1
end

local function charged(unit: number, enemy: number): boolean
    return bit32.band(p4.mem.read("u32", unit + 0x14), CHARGE_BITS[enemy]) ~= 0
end

-- Whether a living party member holds what enemyAI_aniki_B's Dekaja branch asks
-- for (QueryEnHojo 0x1, 0x10, 0x40).
local function partyBuffed(): boolean
    local units = p4.battle.units()
    if units == nil then
        return false
    end
    for _, member in units.party do
        if member.data ~= 0 and bit32.band(p4.mem.read("u32", member.data + 0xC), UNIT_KO) == 0 then
            for _, mask in {0x1, 0x10, 0x40} do
                if (p4.call(A.datCalcHasBuffMask.address, A.datCalcHasBuffMask.signature, member.data, mask) :: number) ~= 0 then
                    return true
                end
            end
        end
    end
    return false
end

local function rotationBeat(state: BossState, rotations: {{string}}): string
    local rotation = rotations[state.phase]
    return rotation[state.step % #rotation + 1]
end

-- Whether a support's heal is allowed on its coming base turn: at most one per
-- `turns` of its own base turns.
local function healReady(state: BossState, turns: number): boolean
    local last = state.lastHeal
    return last == nil or state.turns + 1 - last >= turns
end

-- Shadow Yukiko. Her script answers its band dispatch as HP above 75%
-- (AI_yukiko_100: Agi on a random member below 40, Double Fangs below 80, no
-- Assault Dive or Burn to Ashes branch) and its HP thresholds as unmet, so her
-- retail summon at 85% and Burn to Ashes at 50%/25% never fire on their own.
-- Fire pressure is Agi on a random member, never a weakness read; her own
-- branches that name Chie stay hers (after the party downs her Prince).
local function yukikoPlan(current: Run, unit: number, extra: boolean): Plan
    local data = tuning().bosses
    local state = stateOf(unit)
    setFlag(FLAG_BIT_448, true)
    setFlag(FLAG_BIT_449, true) -- never the retail resummon
    if extra then
        if state.last == "burst" then
            -- No bonus follow-up to a burst: her own attack debuff instead.
            local plan = newPlan(YUKIKO, true, "follow-up")
            plan.hp, plan.wants = 100, MATARUNDA
            return plan
        end
        local plan = newPlan(YUKIKO, true, "extra") -- Double Fangs, not her One More Mind Charge
        plan.hp, plan.more, plan.random = 100, 0, 50
        return plan
    end
    local fraction = hpFraction(current, unit)
    local phase = if fraction <= data.yukikoFinalAt then 3 elseif fraction <= data.yukikoPrinceAt then 2 else 1
    if phase > state.phase then
        state.phase, state.step = phase, 0
    end
    if charged(unit, YUKIKO) then
        local plan = newPlan(YUKIKO, false, "burst")
        plan.hp, plan.wants = 100, BURN_TO_ASHES
        plan.cyclic = rotationBeat(state, YUKIKO_ROTATIONS) == "burst"
        return plan
    end
    if state.phase == 2 and not state.summoned and livingEnemy(PRINCE) == nil then
        -- The Prince's entry through her own first-summon branch, which asks
        -- for HP at or below 85% and above 50%. Below final_at she never
        -- summons.
        local plan = newPlan(YUKIKO, false, "summon")
        plan.hp = 74
        setFlag(FLAG_BIT_448, false)
        return plan
    end
    local plan = newPlan(YUKIKO, false, rotationBeat(state, YUKIKO_ROTATIONS))
    plan.cyclic, plan.hp = true, 100
    if plan.beat == "fire" then
        plan.random = 0
    elseif plan.beat == "physical" then
        plan.random = 50
    elseif plan.beat == "announce" then
        plan.random, plan.wants = 50, MIND_CHARGE
    elseif plan.beat == "burst" then
        plan.wants = BURN_TO_ASHES -- her charge was taken away: the warned burst still comes
    elseif plan.beat == "recovery" then
        plan.random, plan.wants = 50, MATARUNDA
    end
    return plan
end

-- Charming Prince: his own script (Diarama on Yukiko below 75%, Sukukaja,
-- Terror Voice, Cleave, Life Drain on a downed member), with the Diarama branch
-- (flag 467) open at most once per yukiko.prince_heal_turns of his base turns.
local function princePlan(unit: number, extra: boolean): Plan
    local state = stateOf(unit)
    if not extra then
        setFlag(FLAG_PRINCE_HEAL, not healReady(state, tuning().bosses.princeHealTurns))
    end
    return newPlan(PRINCE, extra, if extra then "extra" else "support")
end

-- Shadow Kanji. His script answers its band dispatch for the beat's band and
-- its one-shot flags as spent: AI_kanji_100 has Fatal End on a random member
-- at 30..59, Power Charge at 60..89 and his attack from 90; AI_kanji_50's
-- Fanatical Spark is below 20. The formation change is AI_kanji_25's Tetrakarn
-- on himself (answered as right after his Roar of Wrath and not after Spirit
-- Drain): a visible barrier, no damage and no party ailment. Charged, every band
-- answers Swift Strike first; for the finisher his charge test answers no, so
-- the charged hit is AI_kanji_100's single-target Fatal End on a random living
-- member. Otherwise he never reaches AI_kanji_25 (drains): no support setup.
local function kanjiPlan(current: Run, unit: number, extra: boolean): Plan
    local data = tuning().bosses
    local state = stateOf(unit)
    for _, flag in {FLAG_BIT_448, FLAG_BIT_449, FLAG_BIT_450, FLAG_BIT_451} do
        setFlag(flag, true)
    end
    setFlag(FLAG_BIT_452, false)
    if extra then
        if state.last == "strike" or state.last == "finisher" or state.last == "electric" then
            -- No bonus follow-up to a burst: his One More branch's attack.
            local plan = newPlan(KANJI, true, "follow-up")
            plan.hp, plan.enBadAll = 100, 1
            return plan
        end
        local plan = newPlan(KANJI, true, "extra")
        plan.hp = 100
        return plan
    end
    local fraction = hpFraction(current, unit)
    local supportDown = livingEnemy(NICE_GUY) == nil or livingEnemy(TOUGH_GUY) == nil
    local phase = if fraction <= data.kanjiFinalAt then 3
        elseif fraction <= data.kanjiFormationAt or supportDown then 2 else 1
    if phase > state.phase then
        state.phase, state.step = phase, 0
    end
    if charged(unit, KANJI) then
        local finisher = state.armed and not state.finished -- only after its own charge
        local plan = newPlan(KANJI, false, if finisher then "finisher" else "strike")
        plan.hp = 100
        if finisher then
            plan.selfBuff, plan.random = 0, 45
        else
            plan.wants = SWIFT_STRIKE
        end
        plan.cyclic = rotationBeat(state, KANJI_ROTATIONS) == "strike"
        return plan
    end
    if state.phase >= 2 and not state.formed then
        local plan = newPlan(KANJI, false, "formation")
        plan.hp, plan.random, plan.useSkill = 24, 35, {[SPIRIT_DRAIN] = 0, [ROAR_OF_WRATH] = 1}
        return plan
    end
    local beat = rotationBeat(state, KANJI_ROTATIONS)
    if beat == "pressure" then
        -- P3 pressure alternates electric and physical by rotation.
        beat = if (state.step // #KANJI_ROTATIONS[3]) % 2 == 0 then "electric" else "probe"
    elseif beat == "strike" and state.armed then
        beat = "finisher" -- his charge was taken away: the warned blow still comes, on one member
    end
    local plan = newPlan(KANJI, false, beat)
    plan.cyclic, plan.hp = true, 100
    if beat == "probe" or beat == "finisher" then
        plan.random = 45
    elseif beat == "announce" then
        plan.random, plan.wants = 75, POWER_CHARGE
    elseif beat == "strike" then
        plan.wants = SWIFT_STRIKE -- his charge was taken away: the warned strike still comes
    elseif beat == "recovery" then
        plan.random = 95
    elseif beat == "electric" then
        plan.hp, plan.random, plan.wants = 49, 10, FANATICAL_SPARK
    end
    return plan
end

-- Nice Guy, the defender: his own Diarama branch only for an ally below
-- kanji.heal_below, at most once per kanji.heal_turns of his base turns, and
-- then ahead of his cover queries; otherwise his own cover order (Mutudi and
-- White Wall on Tough Guy, Heat Riser on Kanji, Dekunda, Tarukaja) or attack.
local function niceGuyPlan(current: Run, unit: number, extra: boolean): Plan
    local data = tuning().bosses
    local state = stateOf(unit)
    local plan = newPlan(NICE_GUY, extra, "cover")
    if not healReady(state, data.kanjiHealTurns) then
        return plan
    end
    local lowest = data.kanjiHealBelow
    for _, ally in {KANJI, TOUGH_GUY} do
        local other = livingEnemy(ally)
        if other ~= nil then
            local fraction = hpFraction(current, other)
            if fraction < lowest then
                lowest, plan.heal, plan.beat = fraction, ally, "heal"
            end
        end
    end
    return plan
end

-- Tough Guy, the attacker: Dekaja (his own One More branch, answered as taken)
-- while a party member holds a buff; else his attack on a random member first,
-- then Kill Rush, his attack and Rampage in turn, but no Rampage right after
-- Kanji hit the whole party. His own rule stands: right after Kanji's Swift
-- Strike he only attacks.
local function toughGuyPlan(unit: number, extra: boolean): Plan
    local state = stateOf(unit)
    if extra then
        return newPlan(TOUGH_GUY, true, "extra")
    end
    if partyBuffed() then
        local plan = newPlan(TOUGH_GUY, false, "dispel")
        plan.more = 1
        return plan
    end
    local position = state.turns % 3
    local plan = newPlan(TOUGH_GUY, false, "attack")
    plan.random = 90
    if state.turns > 0 and (position == 0 or (position == 2 and kanjiSweptSinceTough)) then
        plan.beat, plan.random = "kill-rush", 60
    elseif state.turns > 0 and position == 2 then
        plan.beat, plan.random = "rampage", 20
    end
    return plan
end

-- The plan for this evaluation of `unit`'s script.
local function beginPlan(action: number, unit: number): Plan
    local current = run
    local enemy = p4.mem.read("u16", unit + 2)
    local extra = isExtra(action)
    if current == nil or not current.bosses or steeringEncounter == 0 then
        return {steer = false, enemy = enemy, extra = extra, beat = "native", cyclic = false}
    end
    if bit32.band(p4.mem.read("u32", unit + 0xC), DISABLING) ~= 0 then
        -- The party's window: no beat and no advance (an extra-like turn), but
        -- the script stays closed to its retail one-shots (summon, resummon,
        -- HP-threshold bursts) and in its first band.
        local plan = newPlan(enemy, true, "ailing")
        if enemy == YUKIKO then
            setFlag(FLAG_BIT_448, true)
            setFlag(FLAG_BIT_449, true)
            plan.hp = 100
        elseif enemy == KANJI then
            for _, flag in {FLAG_BIT_448, FLAG_BIT_449, FLAG_BIT_450, FLAG_BIT_451} do
                setFlag(flag, true)
            end
            setFlag(FLAG_BIT_452, false)
            plan.hp = 100
        end
        return plan
    end
    if enemy == YUKIKO then
        return yukikoPlan(current, unit, extra)
    elseif enemy == PRINCE then
        return princePlan(unit, extra)
    elseif enemy == KANJI then
        return kanjiPlan(current, unit, extra)
    elseif enemy == NICE_GUY then
        return niceGuyPlan(current, unit, extra)
    elseif enemy == TOUGH_GUY then
        return toughGuyPlan(unit, extra)
    end
    return {steer = false, enemy = enemy, extra = extra, beat = "native", cyclic = false}
end

-- The enemy unit whose script is running (scrGetAiState: its BtlAction).
local function scriptUnit(): (number, number)
    local action = p4.call(A.scrGetAiState.address, A.scrGetAiState.signature) :: number
    if action == 0 then
        return 0, 0
    end
    local actor = p4.mem.read("u32", action + 0x30)
    if actor == 0 or p4.mem.read("u8", actor + 0xA2) ~= 1 then
        return action, 0
    end
    return action, p4.mem.read("u32", actor + 0xA64)
end

-- The steered plan of the evaluation in progress, begun at its first query.
local function steeredPlan(): Plan?
    if steeringEncounter == 0 then
        return nil
    end
    local action, unit = scriptUnit()
    if unit == 0 then
        return nil
    end
    local now = p4.clock.updates()
    local plan = plans[unit]
    if plan == nil or plan.update ~= now then
        local fresh = beginPlan(action, unit)
        fresh.update = now
        plans[unit] = fresh
        plan = fresh
    end
    return if plan.steer then plan else nil
end

-- A turn of a redesign boss is done: its rotation, entries and support
-- cadences move on; the formation cue is queued for the turn's end.
local function advance(action: number, unit: number, plan: Plan, native: number, chosen: number, refused: string?)
    local state = stateOf(unit)
    if not plan.extra then
        state.turns += 1
        if plan.cyclic then
            state.step += 1
        end
        if plan.beat == "summon" then
            state.summoned = true
            if chosen == SUMMON_PRINCE then
                -- The Prince's Ice weakness is shown openly: Analyze knows it
                -- (the retail knowledge bit a landed Ice hit sets).
                p4.call(A.datSetBitmapBit.address, A.datSetBitmapBit.signature, PRINCE, ICE)
                log(string.format("HARDMODE_REVEAL enemy=%d element=%d", PRINCE, ICE))
            end
        elseif plan.beat == "formation" then
            state.formed = true
        elseif plan.beat == "finisher" then
            state.finished, state.armed = true, false
        end
        if chosen == DIA or chosen == DIARAMA then
            state.lastHeal = state.turns
        end
    end
    state.last = plan.beat
    if chosen == POWER_CHARGE and plan.enemy == KANJI then
        state.armed = state.phase == 3 and not state.finished
    elseif plan.beat == "formation" then
        queueBanner(action, "Kanji changes his stance!")
    end
    if plan.enemy == KANJI then
        finisherArmed = plan.beat == "finisher"
        if chosen == SWIFT_STRIKE or chosen == FANATICAL_SPARK then
            kanjiSweptSinceTough = true
        end
    elseif plan.enemy == TOUGH_GUY then
        kanjiSweptSinceTough = false
    end
    local extra = if plan.extra then " extra" else ""
    log(string.format("HARDMODE_BOSS enemy=%d unit=0x%x turn=%d native=%d chosen=%d phase=%d beat=%s%s%s",
        plan.enemy, unit, state.turns, native, chosen, state.phase, plan.beat, extra,
        if refused then " refused=" .. refused else ""))
end

local function redesignTurn(context: P4HookContext, unit: number, native: number): number
    local plan = plans[unit]
    plans[unit] = nil
    if plan == nil or plan.update ~= p4.clock.updates() then
        plan = beginPlan(context.args[1] :: number, unit) -- a turn that asked no steered query
    end
    local turnPlan = plan :: Plan
    if not turnPlan.steer then
        if turnPlan.beat == "ailing" then
            local state = stateOf(unit)
            log(string.format("HARDMODE_BOSS enemy=%d unit=0x%x turn=%d native=%d chosen=%d phase=%d beat=ailing",
                turnPlan.enemy, unit, state.turns, native, native, state.phase))
        end
        return native
    end
    local chosen, refused = substitute(context, native, turnPlan.wants)
    advance(context.args[1] :: number, unit, turnPlan, native, chosen, refused)
    return chosen
end

local function scriptParam(index: number): number
    return p4.call(A.scrGetParamInt.address, A.scrGetParamInt.signature, index) :: number
end

-- One steering point: after the script command ran, its result slot answers
-- for the plan when `decide` has an answer.
local function steeringHook(point: Seam, decide: (plan: Plan) -> number?): P4Subscription
    return p4.hook(point.address, point.signature, "after", function(_context)
        local plan = steeredPlan()
        if plan == nil then
            return
        end
        local value = decide(plan)
        if value ~= nil then
            p4.call(A.scrSetReturnInt.address, A.scrSetReturnInt.signature, value)
        end
    end)
end

local STEERING_HOOKS: {P4Subscription} = {
    -- AI_RND: the beat's branch of its band procedure, inside the draw's range.
    steeringHook(A.btlAiRandom, function(plan: Plan): number?
        local random = plan.random
        return if random ~= nil then math.min(random, math.max(scriptParam(0) - 1, 0)) else nil
    end),
    -- AI_CHK_MYHP (HP at or below the percentage): as if at plan.hp percent.
    steeringHook(A.queryMyHp, function(plan: Plan): number?
        local hp = plan.hp
        return if hp ~= nil then (if hp <= scriptParam(0) then 1 else 0) else nil
    end),
    steeringHook(A.queryMore, function(plan: Plan): number?
        return plan.more
    end),
    steeringHook(A.queryEnBadAll, function(plan: Plan): number?
        return plan.enBadAll
    end),
    -- AI_CHK_MYUSESKIL (the skill he used last): the formation change's answers.
    steeringHook(A.queryMyUseSkil, function(plan: Plan): number?
        local answers = plan.useSkill
        return if answers ~= nil then answers[scriptParam(0)] else nil
    end),
    -- AI_CHK_MYHOJO 0x400: Power Charge (the finisher answers it as absent).
    steeringHook(A.selfHasBuff, function(plan: Plan): number?
        return if plan.selfBuff ~= nil and scriptParam(0) == 0x400 then plan.selfBuff else nil
    end),
    -- enemyAI_aniki_A's heal branches (Diarama at 70, Dia at 80): only the
    -- planned heal; for it, his cover queries answer as already covered.
    steeringHook(A.sideIdQueryMyHp, function(plan: Plan): number?
        if plan.enemy ~= NICE_GUY then
            return nil
        end
        return if plan.heal ~= nil and scriptParam(0) == plan.heal and scriptParam(1) == 70 then 1 else 0
    end),
    steeringHook(A.sideIdQueryMyBad, function(plan: Plan): number?
        return if plan.enemy == NICE_GUY and plan.heal ~= nil then 0 else nil
    end),
    steeringHook(A.sideIdQueryHasBuff, function(plan: Plan): number?
        return if plan.enemy == NICE_GUY and plan.heal ~= nil then 1 else nil
    end),
    steeringHook(A.queryFrHojo, function(plan: Plan): number?
        return if plan.enemy == NICE_GUY and plan.heal ~= nil then 0 else nil
    end),
}

-- Yukiko's summon sets its action through btlEffectScriptSetActionParams,
-- not btlAICommandSetActionCode: her turn ends here.
local summonHook = p4.hook(A.setActionParams.address, A.setActionParams.signature, "after", function(_context)
    if steeringEncounter == 0 then
        return
    end
    local action, unit = scriptUnit()
    local plan = if unit ~= 0 then plans[unit] else nil
    if plan == nil then
        return
    end
    plans[unit] = nil
    local skill = p4.mem.read("u16", action + 0x6E)
    if plan.steer and plan.update == p4.clock.updates() then
        advance(action, unit, plan, skill, skill, nil)
    elseif skill == SUMMON_PRINCE then
        stateOf(unit).summoned = true -- never a second summon
    end
end)

local bossTurnHook = p4.hook(A.btlAICommandSetActionCode.address, A.btlAICommandSetActionCode.signature, "before",
    function(context)
    local current = run
    if current == nil then
        return
    end
    local actor = p4.mem.read("u32", (context.args[1] :: number) + 0x30)
    if actor == 0 or p4.mem.read("u8", actor + 0xA2) ~= 1 then
        return
    end
    local unit = p4.mem.read("u32", actor + 0xA64)
    if unit == 0 or not isBoss(unit) then
        return
    end
    local enemy = p4.mem.read("u16", unit + 2)
    local native = bit32.band(context.args[3] :: number, 0xffff)
    local chosen = native
    if current.bosses then
        if enemy == KNIGHT then
            chosen = knightTactic(context, unit, native)
        elseif steeringEncounter ~= 0 then
            chosen = redesignTurn(context, unit, native)
        end
    end
    if current.technical then
        windowTurn(current, unit, enemy, chosen)
    end
end)

-- The bursts' size per difficulty. Only the Knight's tactic casts Gale Slash;
-- every Burn to Ashes arrives warned; Kanji's Swift Strike is his charged
-- strike, Fanatical Spark his electric pressure and the Fatal End after his P3
-- charge the finisher. A pure function of the hit and Lua state.
local bossModifier = p4.damage.on({kind = "multiply", priority = BOSS_PRIORITY,
    fn = function(hit: P4DamageHit): number?
    local current = run
    if current == nil or hit.source_side ~= "enemy" then
        return nil
    end
    local data = tuning().bosses
    local factor: number? = nil
    if hit.skill == GALE_SLASH and hit.source_id == KNIGHT then
        factor = data.knightSweep[current.mode]
    elseif hit.skill == BURN_TO_ASHES and hit.source_id == YUKIKO then
        factor = data.yukikoBurst[current.mode]
    elseif hit.source_id == KANJI then
        if hit.skill == SWIFT_STRIKE then
            factor = data.kanjiStrike[current.mode]
        elseif hit.skill == FANATICAL_SPARK then
            factor = data.kanjiSpark[current.mode]
        elseif hit.skill == FATAL_END and finisherArmed then
            factor = data.kanjiFinisher[current.mode]
        end
    end
    return if factor ~= nil and factor ~= 1 then factor else nil
end})

-- Story epochs: victory over the epoch's principal encounter moves the run on.
local epochHook = p4.events.on("battle_end", function(event: P4BattleEndEvent)
    local current = run
    if current == nil or event.outcome ~= 1 then
        return
    end
    local record = p4.mem.read("u32", event.battle + BATTLE_RECORD)
    if record == 0 then
        return
    end
    local encounter = p4.mem.read("u16", record + 8)
    local epoch = tuning().epochs[current.epoch]
    if epoch ~= nil and encounter == epoch.encounter then
        current.epoch += 1
        log(string.format("HARDMODE_EPOCH epoch=%d encounter=%d", current.epoch, encounter))
    end
end)

-- Each feature's hooks run only in a run that has it. Every feature needs the
-- encounter class, recorded per enemy unit as it is created.
local bossHooks: {P4Subscription} = {bossModifier, summonHook, bannerHook}
for _, hook in STEERING_HOOKS do
    table.insert(bossHooks, hook)
end
local FEATURE_HOOKS: {[string]: {P4Subscription}} = {
    stats = {maxHpHook, maxSpHook, statHook, pressureModifier},
    rewards = {expHook, moneyHook},
    ai = {aiHook, groupHook},
    technical = {technicalModifier, technicalLog},
    bosses = bossHooks,
}
local CLASS_HOOKS = {encounterHook, unitInitHook, epochHook}

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
    setEnabled(FEATURE_HOOKS.technical, current ~= nil and current.technical)
    setEnabled(FEATURE_HOOKS.bosses, current ~= nil and current.bosses)
    setEnabled({bossTurnHook}, current ~= nil and (current.bosses or current.technical))
    setEnabled(CLASS_HOOKS, current ~= nil)
end
applyFeatures() -- everything but the window guard off until a run is armed

-- Run lifecycle -------------------------------------------------------------------

-- The Camp root's HARD badge (art/badge.png: the 58x21 Camp label in a 64x32
-- atlas) in its empty upper left, with a word under it only when the run is
-- HARDER or adopted (beside it would run into the menu column at x 180).
-- Registered while a run is active, removed when none is. The sprite is made
-- once: sprite records come from a pool shared by every mod (32) and are
-- released only when the VM closes, while every LOAD GAME resets the run first
-- and so removes and re-registers the overlay.
local campBadge: P4UIOverlay? = nil
local badgeSprite: P4UISprite? = nil
local function updateStatus()
    local current = run
    if current == nil then
        if campBadge ~= nil then
            campBadge:remove()
            campBadge = nil
        end
        return
    end
    local words = {}
    if current.mode == "harder" then table.insert(words, "HARDER") end
    if current.adopted then table.insert(words, "ADOPTED") end
    local word = if #words > 0 then table.concat(words, ", ") else "HARD"
    if campBadge == nil then
        local sprite = badgeSprite
        if sprite == nil then
            local made, why = p4.ui.sprite("badge", {x = 0, y = 0, w = 58, h = 21})
            if made == nil then
                log("HARDMODE_BADGE refused: " .. tostring(why))
                return
            end
            sprite = made
            badgeSprite = made
        end
        local overlay, reason = p4.ui.overlay({context = "camp", fade = true, items = {
            {key = "badge", sprite = sprite, x = 22, y = 64},
            {key = "word", text = word, x = 24, y = 90, visible = #words > 0},
        }})
        if overlay == nil then
            log("HARDMODE_BADGE refused: " .. tostring(reason))
            return
        end
        campBadge = overlay
    else
        campBadge:set("word", word)
        campBadge:show("word", #words > 0)
    end
end

local function adopt(next: Run?, why: string)
    run = next
    unitBoss = {}
    rewardLogged = {}
    currentBoss = false
    table.clear(windowProtected)
    local current = run
    if current then
        local installed = tuning().revision
        if current.revision ~= installed then
            -- Only the installed tuning file exists; the run continues on it.
            log(string.format("HARDMODE_REVISION saved=%d current=%d", current.revision, installed))
        end
        local physsplit = p4.mods.required("p4pc.physsplit").version
        if current.physsplit ~= nil and current.physsplit ~= physsplit then
            -- The admitted Physical Split satisfies the requirement; a run that
            -- began on another version continues on it.
            log(string.format("HARDMODE_PHYSSPLIT saved=%s current=%s", current.physsplit, physsplit))
        end
        if current.epoch == 0 then
            -- Saved before the epochs: start from the protagonist's level.
            current.epoch = startingEpoch(true)
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
            table.clear(windowProtected)
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
