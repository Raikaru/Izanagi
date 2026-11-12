--!strict
-- Bundled challenge rules. Every rule is OFF. The rules a player enables in
-- MODS > Challenge modes are armed at the next New Game and saved with the run;
-- loading a save adopts that save's rules, never the current settings.
--
-- The remaining legacy guest-address hooks and reads are awaiting semantic
-- providers; this package is not yet an API-first Challenge release.

local GP = 0x007690F0
local function gp(offset: number): number
    return GP + offset - 0x10000
end
local SKILL_TABLE = gp(0xb3b8) -- 0x28-byte skill rows
local ENCOUNTER_TABLE = gp(0xb414) -- 0x18-byte encounter rows, flags word first
local FIELD_SCRIPT = gp(0xb284) -- the field script whose procedure 15 is Game Over
local HERO_EXP_TABLE = 0x005dd6e0 -- u32[99] hero EXP thresholds in the ELF

local A = {
    datPersonaPreviewLevelUp = 0x0010be60,
    datPersonaAddExp = 0x0010c980,
    datPersonaGetExpForLevel = 0x0010c750,
    datCalcUnitClass = 0x00243e90,
    datSetUnitHp = 0x001056e0,
    datGetEquippedItemByUnit = 0x00106cd0,
    datSetEquippedItemByUnit = 0x00105dc0,
    datEntryGetPrimaryValue = 0x001068b0,
    datEntryHasFlag = 0x00106c30,
    datItemPrice = 0x00106a90,
    datCalcIsDeadById = 0x00105610,
    scrCommonLoadPersonaRecord = 0x0025d580,
    scrGetParamInt = 0x0029cc00,
    datPersonaGetRecord = 0x0010a900,
    datPersonaRosterSlot = 0x0010ace0,
    datPersonaCountValidMembers = 0x0010b6f0,
    btlXpPrepareAwards = 0x0021e110,
    datSetHeroExp = 0x00105990,
    datPersonaGetExpById = 0x00105210,
    datCalcSetLevel = 0x00231ef0,
    FldAmbientFlagPtr = 0x001823c0,
    FldEvent_FindNearestVisibleActor = 0x0014c540,
    datUnitGetRecord = 0x00105510,
    datCarryOverPersistentState = 0x00111bc0,
    datCalcGetLevelById = 0x00104c70,
    datEntryAt = 0x00106820,
    btlTargetCheckCommandAvailability = 0x001f0620,
    radarEntryTypeIsActive = 0x0013c6d0,
    datCalcThresholdTriggerById = 0x0010f6a0,
    scrStartProcedureIndex = 0x0029da90,
    scrStartProcedureChecked = 0x0029db50,
}

-- Story dungeon epochs: the principal final boss of each mandatory dungeon, by
-- encounter (victory advances the epoch) and enemy row (its level is the cap).
local EPOCHS = {
    {encounter = 0x202, enemy = 258}, -- castle
    {encounter = 0x203, enemy = 259}, -- bathhouse
    {encounter = 0x205, enemy = 260}, -- striptease (both of its bosses share the level)
    {encounter = 0x206, enemy = 262}, -- void quest
    {encounter = 0x207, enemy = 263}, -- secret laboratory
    {encounter = 0x208, enemy = 264}, -- heaven
    {encounter = 0x20a, enemy = 266}, -- magatsu inaba
    {encounter = 0x215, enemy = 270}, -- yomotsu hirasaka
}
local SAVES_PER_EPOCH = 3
-- Solo combatants by playable battle ID. Companions join this list with their
-- early-curve progression in a later update.
local SOLO_IDS: {[string]: number} = {YU = 1, YOSUKE = 2, NAOTO = 7}
local SOLO_NAMES: {[number]: string} = {[1] = "YU", [2] = "YOSUKE", [7] = "NAOTO"}

local RULES = {"ironman", "boss_cap", "no_fusion", "solo", "no_battle_items", "no_field_recovery", "severe_damage",
    "limited_saves", "mandatory_only", "no_weapon_fusion"}
local LABELS: {[string]: string} = {
    ironman = "IRONMAN - LOCK ON LOSS",
    boss_cap = "BOSS LEVEL CAP",
    no_fusion = "NO FUSION",
    no_weapon_fusion = "NO WEAPON FUSION",
    solo = "SOLO MEMBER",
    no_battle_items = "NO ITEMS IN BATTLE",
    no_field_recovery = "NO FIELD RECOVERY",
    severe_damage = "SEVERE DAMAGE",
    mandatory_only = "MANDATORY BATTLES ONLY",
    limited_saves = "LIMITED SAVES",
}

type Rules = {[string]: boolean}
-- A solo companion's early curve: join level j, join stats a, level-1 bases b,
-- the join skills, and the skills already granted once.
type Curve = {j: number, a: {number}, b: {number}, join: {number}, given: {number}}
type BossLevels = {levels: {number}, source: string, sha256: string}
type Run = {id: string, rules: Rules, epoch: number, saves: number, gen: number, custom: boolean, solo: number,
    strict: boolean, curve: Curve?, bossLevels: BossLevels?}
-- state: A active, D ended by a battle loss, U ended by an unclean exit during a
-- STRICT battle. battle: "-" or the open STRICT battle "B<session>.<n>".
type Ledger = {state: string, gen: number, recoveries: number, session: string, epoch: number, saves: number,
    battle: string}

local run: Run? = nil
local ledgerView: Ledger? = nil -- the ledger entry of the current run, if any
local inBattle = false
local pendingDefeat = false
-- Retail resets to the New Game defaults at boot, when LOAD GAME starts and
-- before New Game. A reset that no load follows is a New Game: the run arms
-- when that game first reaches gameplay. New Game Plus carries clear data over,
-- so its run is a tagged CUSTOM run, never a verified-from-start one.
local pendingNewGame = false
local carryOver = false
local campOpen = false
local itemRefusalLogged = false
local refusalsLogged: {[string]: boolean} = {}
local severeLogged: {[string]: boolean} = {}
local severeSeen: {[string]: string} = {} -- first resolved hit of each kind, reported at battle_end
local encounterRefusals: {[string]: boolean} = {}
local curveCheck: P4Subscription? = nil -- frame_start, enabled while a solo curve step waits
local soloLineup: string? = nil -- the battle's own lineup, until reported
local soloReported = false
local session = 0 -- this launch's port session (p4.lifecycle), 0 when unavailable
local battleCount = 0 -- STRICT battles opened in this session

local function log(message: string)
    p4.log(message)
end

local function active(rule: string): boolean
    local current = run
    return current ~= nil and current.rules[rule] == true
end

local function soloId(): number
    local current = run
    if current and current.rules.solo then return current.solo end
    return 0
end

local function ruleCount(rules: Rules): number
    local count = 0
    for _, name in RULES do
        if rules[name] then count += 1 end
    end
    return count
end

local function ruleList(rules: Rules): string
    local names = {}
    for _, name in RULES do
        if rules[name] then table.insert(names, name) end
    end
    return table.concat(names, ",")
end

-- The durable per-run ledger lives in this mod's settings file, one bounded
-- scalar per run, written synchronously before the game proceeds. It is
-- monotonic: generations, recoveries and saves only grow, and a locked run
-- never returns to ACTIVE.
local function ledgerKey(id: string): string
    return "run." .. string.lower(id)
end

local function ledgerGet(id: string): Ledger?
    local raw = p4.settings.get(ledgerKey(id))
    if type(raw) ~= "string" then return nil end
    local state, gen, recoveries, sessionText, epoch, saves, battle =
        string.match(raw, "^([ADU])|(%d+)|(%d+)|([CO]%d*)|(%d+)|(%d+)|(%-?B?[%d%.]*)$")
    if not state or not (battle == "-" or string.match(battle :: string, "^B%d+%.%d+$")) then return nil end
    return {state = state, gen = tonumber(gen) :: number, recoveries = tonumber(recoveries) :: number,
        session = sessionText :: string, epoch = tonumber(epoch) :: number, saves = tonumber(saves) :: number,
        battle = battle :: string}
end

local function ledgerPut(id: string, entry: Ledger)
    local text = string.format("%s|%d|%d|%s|%d|%d|%s", entry.state, entry.gen, entry.recoveries,
        entry.session, entry.epoch, entry.saves, entry.battle)
    local ok, why = p4.settings.set(ledgerKey(id), text)
    if not ok then
        -- A run that cannot record its lock or quota must not continue silently.
        error("challenge ledger could not be written: " .. tostring(why))
    end
    ledgerView = entry
end

-- Captured from the selected disc, separately from loaded or edited rows.
-- Freeze all epochs and their file identity with the run, not on each read.
local function originalBossLevels(): BossLevels
    local levels = {}
    local source, sha256 = "", ""
    for _, epoch in EPOCHS do
        local original = p4.data.original_enemy_level(epoch.enemy)
        if original.level < 1 or original.level > 99 then
            error("challenge: original boss level is outside 1..99")
        end
        if source ~= "" and (source ~= original.source or sha256 ~= original.sha256) then
            error("challenge: original boss levels have inconsistent provenance")
        end
        source, sha256 = original.source, original.sha256
        table.insert(levels, original.level)
    end
    return {levels = levels, source = source, sha256 = sha256}
end

local function currentCap(): number
    local current = run
    if not current then return 99 end
    if not EPOCHS[current.epoch] then return 99 end
    if not current.bossLevels then error("challenge: boss level provenance is missing") end
    return current.bossLevels.levels[current.epoch]
end

local HEAL_KINDS = {[2] = true, [5] = true, [7] = true, [9] = true, [11] = true, [15] = true}
local function restoresHpOrSp(skill: number): boolean
    if skill <= 0 or skill >= 0x1b8 then return false end
    local row = p4.mem.read("u32", SKILL_TABLE) + skill * 0x28
    return HEAL_KINDS[p4.mem.read("u8", row + 0x11)] == true or HEAL_KINDS[p4.mem.read("u8", row + 0x14)] == true
end

-- Camp marker and details ---------------------------------------------------

-- The Camp marker never relies on color: plain text, at most two rows.
local function statusLines(): {string}?
    local current = run
    if not current then return nil end
    local head = "CHALLENGE " .. ruleCount(current.rules) .. " RULES" .. (if current.custom then " CUSTOM" else "")
    local ledger = ledgerView
    if current.rules.ironman and ledger and ledger.state ~= "A" then
        return {head, if ledger.state == "U" then "ENDED - UNCLEAN EXIT IN BATTLE" else "ENDED - IRONMAN LOSS"}
    end
    local parts = {}
    if current.rules.ironman then
        table.insert(parts, (if current.strict then "STRICT IRONMAN" else "IRONMAN") .. " RECOVERY x" ..
            (if ledger then ledger.recoveries else 0))
    end
    if current.rules.solo then table.insert(parts, "SOLO " .. SOLO_NAMES[current.solo]) end
    if current.rules.boss_cap then table.insert(parts, "CAP " .. currentCap()) end
    if current.rules.limited_saves then
        table.insert(parts, "SAVES " .. math.max(0, SAVES_PER_EPOCH - current.saves))
    end
    if #parts == 0 then return {head} end
    return {head, table.concat(parts, " ")}
end

local function refreshStatus()
    local lines = statusLines()
    if not p4.ui.status(lines) and lines then
        log("CHALLENGE_STATUS refused text=" .. table.concat(lines, " / "))
        return
    end
    log("CHALLENGE_STATUS " .. (if lines then table.concat(lines, " / ") else "cleared"))
end

local function detailText(): string
    local current = run
    if not current then
        return "No challenge run is active.\nEnable rules on the Challenge modes page, then start a New Game to arm them.\nLoading a save uses the rules it was started with."
    end
    local lines = {"Run " .. current.id .. ": " .. ruleCount(current.rules) .. " rules" ..
        (if current.custom then ", CUSTOM continuation of an ordinary save." else ", verified from New Game.")}
    for _, name in RULES do
        if current.rules[name] then
            table.insert(lines, "  " .. LABELS[name] .. (if name == "solo" then ": " .. SOLO_NAMES[current.solo] else ""))
        end
    end
    local ledger = ledgerView
    if current.rules.ironman then
        local state = if ledger and ledger.state == "D" then "ENDED by a battle loss (locked)"
            elseif ledger and ledger.state == "U" then "ENDED by an unclean exit during a battle (locked)"
            else "ACTIVE"
        table.insert(lines, (if current.strict then "Strict Ironman: " else "Ironman: ") .. state ..
            ", recoveries " .. (if ledger then ledger.recoveries else 0) .. ".")
        if current.strict then
            table.insert(lines, "  Exiting while a battle is unresolved ends this run unless the port recorded a crash.")
        end
    end
    if current.rules.boss_cap then
        table.insert(lines, "Level cap: " .. currentCap() .. " until the next story dungeon boss falls.")
    end
    if current.rules.limited_saves then
        table.insert(lines, "Saves left this chapter: " .. math.max(0, SAVES_PER_EPOCH - current.saves) ..
            " of " .. SAVES_PER_EPOCH .. ". The next story dungeon boss replenishes them.")
    end
    return table.concat(lines, "\n")
end

p4.ui.detail({key = "details", title = "CHALLENGE DETAILS", detail = detailText})

-- Run lifecycle -------------------------------------------------------------

local function nextRunId(): string
    local counter = p4.settings.get("runs.next")
    local n = (if type(counter) == "number" then counter else 0) + 1
    local ok, why = p4.settings.set("runs.next", n)
    if not ok then error("challenge run counter could not be written: " .. tostring(why)) end
    return "R" .. n
end

local function armedRules(): Rules
    local rules: Rules = {}
    for _, name in RULES do
        if name == "ironman" then
            rules.ironman = p4.settings.get("ironman") == true and p4.settings.get("ironman_confirm") == true
        elseif name == "solo" then
            rules.solo = SOLO_IDS[tostring(p4.settings.get("solo"))] ~= nil
        else
            rules[name] = p4.settings.get(name) == true
        end
    end
    return rules
end

-- STRICT IRONMAN is an Ironman parameter, armed with it and its own confirmation.
local function armedStrict(rules: Rules): boolean
    if not rules.ironman or p4.settings.get("strict_ironman") ~= true or p4.settings.get("strict_confirm") ~= true then
        return false
    end
    if session == 0 then
        log("CHALLENGE_RULE_UNAVAILABLE rule=strict_ironman")
        return false
    end
    return true
end

local function openSession(current: Run, ledger: Ledger)
    ledger.session = "O" .. session
    ledgerPut(current.id, ledger)
end

-- Solo companions: the early curve from level 1 -------------------------------
-- A companion chosen at New Game fights from the first battle with their own
-- Persona, rebuilt at level 1: each stat's level-1 base is its join value less
-- the Persona's own fixed growth from level 2 to the join level J (at least 1;
-- a stat clamped at 1 instead gains exactly the rest over levels 2..J,
-- proportional to that stat's own growth, largest remainders first, lower
-- levels first on ties). Native growth carries on after J. Skills follow the
-- design's early learnset (levels 1, 4, 8, 12), then the down-tier
-- predecessors of the Persona's own elemental lines at 20, 28, ... below J,
-- weakest first, then at J the join skills not yet held. Each is granted once,
-- into a free slot; a forgotten skill is not restored.
local PARTY_GROWTH = gp(0xb3e4) -- per party Persona (from 0xC0) a 0x26E-byte row
local GROWTH_ROW = 0x26e
local GROWTH_LEARN = 0x04 -- 32 events {level u8, kind u8, skill u16}; kind 1 learns
local GROWTH_STATS = 0x7a -- + level * 5: the five stat gains reaching that level

-- Original P4 skill IDs (names from the disc's battle message table).
local EARLY_SKILLS: {[number]: {[number]: number}} = {
    [2] = {[1] = 14, [4] = 192, [8] = 220, [12] = 120}, -- Yosuke: Garu, Dia, Sukukaja, Bash
    [3] = {[1] = 26, [4] = 150, [8] = 218, [12] = 29}, -- Chie: Bufu, Skewer, Tarukaja, Mabufu
    [4] = {[1] = 1, [4] = 192, [8] = 200, [12] = 195}, -- Yukiko: Agi, Dia, Me Patra, Media
    [6] = {[1] = 39, [4] = 120, [8] = 218, [12] = 42}, -- Kanji: Zio, Bash, Tarukaja, Mazio
    [7] = {[1] = 123, [4] = 61, [8] = 72, [12] = 220}, -- Naoto: Sonic Punch, Hama, Mudo, Sukukaja
    [8] = {[1] = 26, [4] = 192, [8] = 222, [12] = 29}, -- Teddie: Bufu, Dia, Rakukaja, Mabufu
}
-- Tiered elemental lines, weakest first.
local LINES: {{number}} = {{1, 2, 3}, {4, 5, 6}, {14, 15, 16}, {17, 18, 19}, {26, 27, 28}, {29, 30, 31}, {39, 40, 41},
    {42, 43, 44}, {51, 52, 53}, {61, 62}, {63, 64}, {72, 73}, {74, 75}}

local function growthRow(persona: number): number
    return p4.mem.read("u32", PARTY_GROWTH) + (persona - 0xc0) * GROWTH_ROW
end
local function growth(persona: number, level: number, stat: number): number
    return p4.mem.read("u8", growthRow(persona) + GROWTH_STATS + level * 5 + stat)
end

-- Per stat, the gains for levels 2..J (the stat's own fixed growth, or for a
-- stat clamped at 1 its exact redistribution).
local function curveGains(curve: Curve, persona: number): {{number}}
    local gains = {}
    for stat = 0, 4 do
        local own, total = {}, 0
        for level = 2, curve.j do
            own[level] = growth(persona, level, stat)
            total += own[level]
        end
        local need = curve.a[stat + 1] - curve.b[stat + 1]
        if need ~= total then
            -- Clamped at 1: distribute exactly `need` over the same levels.
            local shares, given = {}, 0
            for level = 2, curve.j do
                local exact = if total > 0 then need * own[level] / total else need / math.max(1, curve.j - 1)
                shares[level] = math.floor(exact)
                given += shares[level]
                own[level] = exact - shares[level]
            end
            while given < need do
                local best = 2
                for level = 3, curve.j do
                    if own[level] > own[best] then best = level end
                end
                shares[best] += 1
                own[best] = -1
                given += 1
            end
            own = shares
        end
        gains[stat + 1] = own
    end
    return gains
end

-- Skills by level: the early learnset, gap predecessors, then the join skills.
local function curveSchedule(curve: Curve, member: number, persona: number): {{level: number, skill: number}}
    local schedule = {}
    local seen: {[number]: boolean} = {}
    local function add(level: number, skill: number)
        if skill ~= 0 and not seen[skill] then
            seen[skill] = true
            table.insert(schedule, {level = level, skill = skill})
        end
    end
    local early = EARLY_SKILLS[member]
    for _, level in {1, 4, 8, 12} do add(level, early[level]) end
    -- The Persona's own lines: its join skills and its learn events.
    local own: {[number]: boolean} = {}
    for _, skill in curve.join do own[skill] = true end
    local row = growthRow(persona)
    for event = 0, 31 do
        local kind = p4.mem.read("u8", row + GROWTH_LEARN + event * 4 + 1)
        if kind == 0 then break end
        if kind == 1 then own[p4.mem.read("u16", row + GROWTH_LEARN + event * 4 + 2)] = true end
    end
    local predecessors: {{tier: number, skill: number}} = {}
    for _, line in LINES do
        for tier = #line, 2, -1 do
            if own[line[tier]] then
                for lower = 1, tier - 1 do table.insert(predecessors, {tier = lower, skill = line[lower]}) end
                break
            end
        end
    end
    table.sort(predecessors, function(x: {tier: number, skill: number}, y: {tier: number, skill: number}): boolean
        return x.tier < y.tier or (x.tier == y.tier and x.skill < y.skill)
    end)
    local level = 20
    for _, entry in predecessors do
        if level >= curve.j then break end
        if not seen[entry.skill] then
            add(level, entry.skill)
            level += 8
        end
    end
    for _, skill in curve.join do add(curve.j, skill) end
    return schedule
end

-- The record a level-1..J companion should hold: stats on the curve, and the
-- scheduled skills reached so far that were never granted, into free slots.
local function curveTarget(current: Run, record: P4PartyRecord): P4PartyRecord?
    local curve = current.curve
    if not curve then return nil end
    local changed = false
    local stats = table.clone(record.stats)
    if record.level <= curve.j then
        local gains = curveGains(curve, record.persona)
        for stat = 1, 5 do
            local expected = curve.b[stat]
            for level = 2, record.level do expected += gains[stat][level] end
            if stats[stat] ~= expected then
                stats[stat] = expected
                changed = true
            end
        end
    end
    local skills = table.clone(record.skills)
    local held: {[number]: boolean} = {}
    for _, skill in skills do held[skill] = true end
    local granted: {[number]: boolean} = {}
    for _, skill in curve.given do granted[skill] = true end
    for _, entry in curveSchedule(curve, current.solo, record.persona) do
        if entry.level <= record.level and not granted[entry.skill] then
            if held[entry.skill] or #skills < 8 then
                -- Granted once: a skill later forgotten is not restored.
                granted[entry.skill] = true
                table.insert(curve.given, entry.skill)
            end
            if not held[entry.skill] and #skills < 8 then
                table.insert(skills, entry.skill)
                held[entry.skill] = true
                changed = true
                log(string.format("SOLO_SKILL member=%s level=%d skill=%d", SOLO_NAMES[current.solo], record.level,
                    entry.skill))
            end
        end
    end
    if not changed then return nil end
    return {persona = record.persona, level = record.level, exp = record.exp, stats = stats, skills = skills}
end

-- The companion's weapon and armor are held to Yu's New Game starting tier:
-- one above it gives way to the strongest priced item of the companion's own
-- equip category (the Camp equip list's legality test) that does not exceed
-- it, so a late companion's story join kit is not free at level 1.
local function starterEquipment(member: number): {number}
    local kit = {}
    for slot = 0, 1 do
        local current = p4.call(A.datGetEquippedItemByUnit, "hhh", member, slot) :: number
        local reference = p4.call(A.datEntryGetPrimaryValue, "Hh",
            p4.call(A.datGetEquippedItemByUnit, "hhh", 1, slot) :: number) :: number
        local best = current
        if (p4.call(A.datEntryGetPrimaryValue, "Hh", current) :: number) > reference then
            local bestValue, lowest, lowestValue = -1, current, math.huge
            for item = slot * 0x100 + 1, slot * 0x100 + 0xff do
                if p4.call(A.datEntryHasFlag, "ihh", item, member) ~= 0 and
                    (p4.call(A.datItemPrice, "Ih", item) :: number) > 0 then
                    local value = p4.call(A.datEntryGetPrimaryValue, "Hh", item) :: number
                    if value > 0 and value <= reference and value > bestValue then best, bestValue = item, value end
                    if value > 0 and value < lowestValue then lowest, lowestValue = item, value end
                end
            end
            if bestValue < 0 then best = lowest end
            if best ~= current then p4.call(A.datSetEquippedItemByUnit, "vhhh", member, slot, best) end
        end
        table.insert(kit, best)
    end
    table.insert(kit, p4.call(A.datGetEquippedItemByUnit, "hhh", member, 2) :: number)
    return kit
end

-- New Game: the chosen companion's record becomes level 1 on the curve.
local function companionCurve(member: number): Curve?
    local record = p4.party.record(member)
    if not record then return nil end
    local a, b = table.clone(record.stats), {}
    for stat = 0, 4 do
        local total = 0
        for level = 2, record.level do total += growth(record.persona, level, stat) end
        b[stat + 1] = math.max(1, a[stat + 1] - total)
    end
    local first = EARLY_SKILLS[member][1]
    local curve: Curve = {j = record.level, a = a, b = b, join = table.clone(record.skills), given = {first}}
    local ok, why = p4.party.set(member, {persona = record.persona, level = 1, exp = 0, stats = b, skills = {first}})
    if not ok then error("solo early curve could not be set: " .. tostring(why)) end
    local join = {}
    for slot = 0, 2 do table.insert(join, p4.call(A.datGetEquippedItemByUnit, "hhh", member, slot) :: number) end
    local kit = starterEquipment(member)
    log(string.format("SOLO_CURVE member=%s persona=%d join_level=%d join_stats=%s base=%s skill=%d join_equipment=%s equipment=%s",
        SOLO_NAMES[member], record.persona, record.level, table.concat(a :: {any}, ","),
        table.concat(b :: {any}, ","), first, table.concat(join :: {any}, ","), table.concat(kit :: {any}, ",")))
    return curve
end

local function startRun(custom: boolean)
    local rules = armedRules()
    if ruleCount(rules) == 0 then
        run = nil
        ledgerView = nil
        log("CHALLENGE_NEW_GAME armed=0")
        return
    end
    local bossLevels = if rules.boss_cap or custom then originalBossLevels() else nil
    local epoch = 1
    if custom then
        -- Provisional custom-run epoch inference still follows hero level.
        local level = p4.call(A.datCalcGetLevelById, "ii", 1) :: number
        while EPOCHS[epoch] and (bossLevels :: BossLevels).levels[epoch] <= level do epoch += 1 end
    end
    local solo = if rules.solo then SOLO_IDS[tostring(p4.settings.get("solo"))] else 0
    local curve: Curve? = nil
    if solo > 1 then
        if custom then
            -- A companion's early curve starts at New Game, never from a save.
            log("CHALLENGE_RULE_UNAVAILABLE rule=solo member=" .. SOLO_NAMES[solo] .. " reason=new_game_only")
            rules.solo = false
            solo = 0
        else
            curve = companionCurve(solo)
        end
    end
    local strict = armedStrict(rules)
    local current: Run = {id = nextRunId(), rules = rules, epoch = epoch, saves = 0, gen = 0, custom = custom, solo = solo,
        strict = strict, curve = curve, bossLevels = bossLevels}
    run = current
    openSession(current, {state = "A", gen = 0, recoveries = 0, session = "C", epoch = epoch, saves = 0, battle = "-"})
    log(string.format("CHALLENGE_ARMED run=%s rules=%s custom=%s epoch=%d solo=%d strict=%s", current.id, ruleList(rules),
        tostring(custom), epoch, solo, tostring(strict)))
end

local function decodeCurve(value: any): Curve?
    if type(value) ~= "table" or type(value.a) ~= "table" or type(value.b) ~= "table" then return nil end
    local function numbers(list: any): {number}
        local out = {}
        if type(list) == "table" then
            for _, item in list do
                if type(item) == "number" then table.insert(out, item) end
            end
        end
        return out
    end
    return {j = tonumber(value.j) or 1, a = numbers(value.a), b = numbers(value.b), join = numbers(value.join),
        given = numbers(value.given)}
end

local function decodeBossLevels(value: any): BossLevels?
    if type(value) ~= "table" or type(value.levels) ~= "table" or type(value.source) ~= "string" or
        value.source == "" or type(value.sha256) ~= "string" or
        not string.match(value.sha256, "^[0-9a-f]+$") or #value.sha256 ~= 64 or #value.levels ~= #EPOCHS then
        return nil
    end
    local levels = {}
    for i = 1, #EPOCHS do
        local level = value.levels[i]
        if type(level) ~= "number" or level < 1 or level > 99 or level ~= math.floor(level) then return nil end
        levels[i] = level
    end
    return {levels = levels, source = value.source, sha256 = value.sha256}
end

local function decodeRun(value: any): Run?
    if type(value) ~= "table" or value.v ~= 1 or type(value.id) ~= "string" or type(value.rules) ~= "table" then
        return nil
    end
    local rules: Rules = {}
    for _, name in RULES do rules[name] = value.rules[name] == true end
    return {id = value.id, rules = rules, epoch = tonumber(value.epoch) or 1, saves = tonumber(value.saves) or 0,
        gen = tonumber(value.gen) or 0, custom = value.custom == true,
        solo = if rules.solo and SOLO_NAMES[tonumber(value.solo) or 0] then tonumber(value.solo) :: number else 0,
        strict = rules.ironman and value.strict == true, curve = decodeCurve(value.curve),
        bossLevels = decodeBossLevels(value.bossLevels)}
end

local function adoptRun(value: any)
    local current = decodeRun(value)
    run = current
    ledgerView = nil
    if not current then
        if p4.settings.get("custom_on_load") == true then
            -- One-shot: this ordinary save becomes a CUSTOM run; the file on
            -- the card is unchanged until the player saves the run.
            p4.settings.set("custom_on_load", false)
            startRun(true)
        else
            log("CHALLENGE_LOADED run=none")
        end
        return
    end
    local ledger = ledgerGet(current.id)
    if not ledger then
        -- authorize_load refused every save of a run without its ledger entry.
        error("challenge run " .. current.id .. " has no ledger entry")
    end
    -- An older slot never refills the quota or rewinds the epoch.
    if ledger.epoch > current.epoch then
        current.epoch = ledger.epoch
        current.saves = ledger.saves
    elseif ledger.epoch == current.epoch then
        current.saves = math.max(current.saves, ledger.saves)
    end
    local recovered = false
    if current.rules.ironman and string.sub(ledger.session, 1, 1) == "O" and ledger.session ~= "O" .. session then
        -- The previous session of this run never closed: a crash, kill or power loss.
        ledger.recoveries += 1
        recovered = true
    end
    openSession(current, ledger)
    log(string.format("CHALLENGE_LOADED run=%s rules=%s gen=%d epoch=%d saves=%d recoveries=%d recovered=%s",
        current.id, ruleList(current.rules), current.gen, current.epoch, current.saves, ledger.recoveries,
        tostring(recovered)))
end

-- The first gameplay after a reset that no load followed.
local function armNewGame()
    if not pendingNewGame then return end
    pendingNewGame = false
    startRun(carryOver)
    carryOver = false
    refreshStatus()
end

p4.save.register({
    schema = 1,
    max_bytes = 2048,
    snapshot = function()
        local current = run
        if not current then return nil end
        -- The saved state includes this save: its generation and quota debit.
        local saves = current.saves + (if current.rules.limited_saves then 1 else 0)
        return {v = 1, id = current.id, rules = current.rules, epoch = current.epoch, saves = saves,
            gen = current.gen + 1, custom = current.custom, solo = current.solo, strict = current.strict,
            curve = current.curve, bossLevels = current.bossLevels}
    end,
    load = function(value, origin)
        if origin == "reset" then
            run = nil
            ledgerView = nil
            pendingNewGame = true
        else
            pendingNewGame = false
            carryOver = false
            adoptRun(value)
        end
        pendingDefeat = false
        refreshStatus()
    end,
    authorize_save = function(): (boolean, ...string)
        local current = run
        if not current then return true end
        local ledger = ledgerGet(current.id)
        if current.rules.ironman and ledger and ledger.state ~= "A" then
            return false, "This IRONMAN run has ended; its saves are locked."
        end
        if current.rules.limited_saves and current.saves >= SAVES_PER_EPOCH then
            return false, "LIMITED SAVES: no saves left until the next story dungeon boss is defeated."
        end
        return true
    end,
    authorize_load = function(value: P4SaveValue): (boolean, ...string)
        local current = decodeRun(value)
        if not current then return true end
        if current.rules.boss_cap and not current.bossLevels then
            return false, "This provisional Challenge save lacks original boss-level provenance and cannot be resumed with the cap."
        end
        local ledger = ledgerGet(current.id)
        if not ledger then
            return false, "Challenge run " .. current.id .. " has no ledger record on this device; its saves cannot be played."
        end
        if current.rules.ironman then
            if ledger.state == "D" then
                return false, "IRONMAN run " .. current.id .. " ended in defeat. Its saves are locked, not erased."
            end
            if ledger.state == "U" then
                return false, "STRICT IRONMAN run " .. current.id ..
                    " ended: the game exited during a battle without a recorded crash. Its saves are locked, not erased."
            end
            if ledger.battle ~= "-" then
                return false, "STRICT IRONMAN run " .. current.id ..
                    " has an unreconciled battle record; its crash marker could not be read."
            end
            if current.gen < ledger.gen then
                return false, string.format("IRONMAN run %s: only its newest save (save %d) can be loaded; this is save %d.",
                    current.id, ledger.gen, current.gen)
            end
        end
        return true
    end,
    published = function(value)
        local current = run
        local saved = decodeRun(value)
        if not current or not saved or saved.id ~= current.id then return end
        current.gen = math.max(current.gen, saved.gen)
        current.saves = saved.saves
        local ledger = ledgerGet(current.id) or {state = "A", gen = 0, recoveries = 0, session = "O" .. session,
            epoch = current.epoch, saves = 0, battle = "-"}
        ledger.gen = math.max(ledger.gen, saved.gen)
        if ledger.epoch == current.epoch then
            ledger.saves = math.max(ledger.saves, current.saves)
        end
        ledgerPut(current.id, ledger)
        log(string.format("CHALLENGE_SAVED run=%s gen=%d saves=%d", current.id, current.gen, current.saves))
        refreshStatus()
    end,
})

p4.hook(A.datCarryOverPersistentState, "v", "before", function()
    carryOver = true
end)

-- Ironman: a battle loss that reaches Game Over ends the run ----------------

local function commitLoss(where: string)
    local current = run
    if not current or not current.rules.ironman or not pendingDefeat then return end
    pendingDefeat = false
    local ledger = ledgerGet(current.id) or {state = "A", gen = current.gen, recoveries = 0, session = "C",
        epoch = current.epoch, saves = current.saves, battle = "-"}
    ledger.state = "D"
    ledger.session = "C"
    -- The terminal tombstone is committed before the open battle is resolved.
    ledger.battle = "-"
    ledgerPut(current.id, ledger)
    if current.strict then p4.lifecycle.tag(nil) end
    log("IRONMAN_LOSS run=" .. current.id .. " via=" .. where .. " locked=true")
    refreshStatus()
end

local function gameOverHook(where: string)
    return function(context: P4HookContext)
        if context.args[1] == 15 and context.args[2] == p4.mem.read("u32", FIELD_SCRIPT) then
            commitLoss(where)
        end
    end
end
p4.hook(A.scrStartProcedureIndex, "iipi", "before", gameOverHook("field"))
p4.hook(A.scrStartProcedureChecked, "iiiii", "before", gameOverHook("script"))

-- STRICT IRONMAN: an open battle is durable before the battle runs ----------
-- Its record names this session and battle; the port's crash handler records
-- the same tag. The record clears once the battle resolves: victory, escape or
-- another nonterminal end, or the terminal tombstone of a loss.

local function openStrictBattle()
    local current = run
    if not current or not current.strict then return end
    local ledger = ledgerGet(current.id)
    if not ledger or ledger.state ~= "A" then return end
    battleCount += 1
    local battle = string.format("B%d.%d", session, battleCount)
    local ok, why = p4.lifecycle.tag(current.id .. ":" .. battle)
    if not ok then error("STRICT IRONMAN could not arm the crash marker: " .. tostring(why)) end
    ledger.battle = battle
    ledger.session = "O" .. session
    ledgerPut(current.id, ledger)
    log("STRICT_BATTLE_OPEN run=" .. current.id .. " battle=" .. battle)
end

local function closeStrictBattle(via: string)
    local current = run
    if not current or not current.strict then return end
    local ledger = ledgerGet(current.id)
    if not ledger or ledger.battle == "-" then return end
    local battle = ledger.battle
    ledger.battle = "-"
    ledgerPut(current.id, ledger)
    p4.lifecycle.tag(nil)
    log("STRICT_BATTLE_CLOSED run=" .. current.id .. " battle=" .. battle .. " via=" .. via)
end

-- Before any load: an open battle of an earlier session is a counted recovery
-- only when the port recorded a crash with that battle's tag; otherwise the run
-- ends. An unreadable marker blocks the run's saves until it can be read.
local function reconcileStrict()
    local count = p4.settings.get("runs.next")
    if type(count) ~= "number" then return end
    for n = 1, count do
        local id = "R" .. n
        local ledger = ledgerGet(id)
        local opened = if ledger and ledger.battle ~= "-" then tonumber(string.match(ledger.battle, "^B(%d+)%.")) else nil
        if ledger and opened and opened ~= session then
            local crashed, tag = p4.lifecycle.crashed(opened)
            if crashed == nil then
                log("STRICT_RECONCILE_BLOCKED run=" .. id .. " battle=" .. ledger.battle .. " reason=" .. tostring(tag))
            else
                local battle = ledger.battle
                local recovered = crashed == true and tag == id .. ":" .. battle
                if recovered then
                    ledger.recoveries += 1
                else
                    ledger.state = "U"
                end
                ledger.battle = "-"
                ledger.session = "C"
                ledgerPut(id, ledger)
                if crashed then p4.lifecycle.consume(opened) end
                if recovered then
                    log(string.format("STRICT_RECOVERY run=%s battle=%s recoveries=%d", id, battle, ledger.recoveries))
                else
                    log("IRONMAN_LOSS run=" .. id .. " via=unclean_exit_in_battle battle=" .. battle ..
                        " crash=" .. tostring(crashed) .. " locked=true")
                end
            end
        end
    end
end

-- Battle bookkeeping: defeats, epoch advances --------------------------------

p4.events.on("battle_start", function()
    armNewGame()
    inBattle = true
    pendingDefeat = false
    openStrictBattle()
    itemRefusalLogged = false
    severeLogged = {}
end)

p4.events.on("battle_end", function(event: P4BattleEndEvent)
    for key, line in severeSeen do
        severeLogged[key] = true
        log(line)
    end
    severeSeen = {}
    local lineup = soloLineup
    if lineup and not soloReported then
        soloReported = true
        log(string.format("SOLO_LINEUP member=%s from=%s", SOLO_NAMES[soloId()] or "?", lineup))
    end
    inBattle = false
    local current = run
    if not current then return end
    if current.curve and curveCheck then (curveCheck :: P4Subscription):enable() end
    if event.outcome == 2 then
        pendingDefeat = true
        log("CHALLENGE_DEFEAT run=" .. current.id)
        if current.solo > 1 and p4.call(A.datCalcIsDeadById, "Ih", current.solo) ~= 0 then
            -- The chosen companion fell; benched Yu must not keep the run going.
            -- A scripted loss (the first night's dream) leaves the fighter
            -- standing and the story goes on as in retail.
            p4.call(A.datSetUnitHp, "vhh", 1, 0)
            log("SOLO_DEFEAT member=" .. SOLO_NAMES[current.solo])
        end
        return
    end
    if event.outcome ~= 1 then
        closeStrictBattle("outcome" .. event.outcome)
        return
    end
    local record = p4.mem.read("u32", event.battle + 0xc68)
    if record == 0 then
        closeStrictBattle("victory")
        return
    end
    local encounter = p4.mem.read("u16", record + 8)
    local epoch = EPOCHS[current.epoch]
    if epoch and encounter == epoch.encounter then
        current.epoch += 1
        current.saves = 0
        local ledger = ledgerGet(current.id)
        if ledger then
            ledger.epoch = math.max(ledger.epoch, current.epoch)
            ledger.saves = 0
            ledgerPut(current.id, ledger)
        end
        log(string.format("CHALLENGE_EPOCH run=%s epoch=%d cap=%d", current.id, current.epoch, currentCap()))
        refreshStatus()
    end
    closeStrictBattle("victory")
end)

p4.events.on("field_enter", function()
    armNewGame()
    -- A loss the story continues from (no Game Over) resolved its battle.
    closeStrictBattle("field")
    pendingDefeat = false
    encounterRefusals = {}
end)

-- An orderly quit outside battle closes this session; any other end of the
-- process leaves it open, and the next load of the run counts a recovery.
p4.events.on("exit", function()
    local current = run
    if not current or inBattle then return end
    local ledger = ledgerGet(current.id)
    if ledger and ledger.state == "A" then
        ledger.session = "C"
        ledgerPut(current.id, ledger)
        log("CHALLENGE_SESSION_CLOSED run=" .. current.id)
    end
end)

p4.events.on("menu_open", function(event: P4MenuOpenEvent)
    if event.kind == 0 then
        armNewGame()
        campOpen = true
        refusalsLogged = {}
        refreshStatus()
    end
end)
p4.events.on("menu_close", function(event: P4MenuCloseEvent)
    if event.kind == 0 then campOpen = false end
end)

-- Boss level cap --------------------------------------------------------------

local function personaRoom(persona: number): number
    local level = p4.mem.read("u8", persona + 4)
    local cap = currentCap()
    if level >= cap then return 0 end
    local ceiling = p4.call(A.datPersonaGetExpForLevel, "IpH", persona, cap + 1) :: number
    local exp = p4.mem.read("i32", persona + 8)
    return math.max(0, ceiling - 1 - exp)
end

local function capPersonaExp(argument: number)
    return function(context: P4HookContext)
        if active("boss_cap") then
            local requested = context.args[argument] :: number
            local allowed = math.min(requested, personaRoom(context.args[1] :: number))
            if allowed ~= requested then
                context.args[argument] = allowed
                log(string.format("CAP_PERSONA cap=%d requested=%d allowed=%d", currentCap(), requested, allowed))
            end
        end
        context.result = context.next()
    end
end
p4.hook(A.datPersonaPreviewLevelUp, "vppi", "replace", capPersonaExp(3))
p4.hook(A.datPersonaAddExp, "vpi", "replace", capPersonaExp(2))

-- The battle results screens add these prepared awards to the Persona records
-- themselves (not through datPersonaAddExp), so each award is clamped where
-- btlXpPrepareAwards leaves it, before anything reads it. Its level-up previews
-- were already computed from the clamped amounts above.
local AWARD_FLAGS = 0x00 -- u32: 2 hero, 8 roster, 0x10 party level-up screens
local AWARD_HERO_EXP = 0x04 -- s32
local AWARD_ROSTER_EXP = 0x08 -- s32[12], one per roster slot
local AWARD_ROSTER_PREVIEW = 0x38 -- 0x88-byte level-up previews; byte 0 is levels gained
local AWARD_PARTY_PREVIEW = 0x6b4
local AWARD_PARTY_IDS = 0x69a -- u16[4]
local AWARD_PARTY_EXP = 0x6a4 -- s32[4]

local function heroThreshold(level: number): number
    return p4.mem.read("u32", HERO_EXP_TABLE + 4 * (math.min(level, 99) - 1))
end

local function clampAward(cell: number, persona: number)
    local requested = p4.mem.read("i32", cell)
    if requested <= 0 or persona == 0 then return end
    local allowed = math.min(requested, personaRoom(persona))
    if allowed ~= requested then
        p4.mem.write("i32", cell, allowed)
        log(string.format("CAP_AWARD persona=%d cap=%d requested=%d allowed=%d", p4.mem.read("u16", persona + 2),
            currentCap(), requested, allowed))
    end
end

p4.hook(A.btlXpPrepareAwards, "vpp", "after", function(context)
    if not active("boss_cap") then return end
    local awards = context.args[1] :: number
    local roster = math.min(12, p4.call(A.datPersonaCountValidMembers, "H") :: number)
    for slot = 0, roster - 1 do
        clampAward(awards + AWARD_ROSTER_EXP + slot * 4, p4.call(A.datPersonaRosterSlot, "ph", slot) :: number)
    end
    local partyUp = false
    for slot = 0, 3 do
        local id = p4.mem.read("u16", awards + AWARD_PARTY_IDS + slot * 2)
        if id ~= 0 then
            clampAward(awards + AWARD_PARTY_EXP + slot * 4, p4.call(A.datPersonaGetRecord, "pH", id) :: number)
            partyUp = partyUp or p4.mem.read("u8", awards + AWARD_PARTY_PREVIEW + slot * 0x88) > 0
        end
    end
    -- Yu's own award, so the results never show EXP the cap takes away.
    local cap = currentCap()
    local heroExp = p4.call(A.datPersonaGetExpById, "Ih", 1) :: number
    local heroLevel = p4.call(A.datCalcGetLevelById, "ii", 1) :: number
    local heroAward = p4.mem.read("i32", awards + AWARD_HERO_EXP)
    if cap < 99 and heroAward > 0 then
        local allowed = math.max(0, math.min(heroAward, heroThreshold(cap + 1) - 1 - heroExp))
        if allowed ~= heroAward then
            p4.mem.write("i32", awards + AWARD_HERO_EXP, allowed)
            log(string.format("CAP_AWARD persona=hero cap=%d requested=%d allowed=%d", cap, heroAward, allowed))
            heroAward = allowed
        end
    end
    -- The level-up screens follow the clamped awards: their previews were made
    -- from the clamped amounts, so a screen with no level left is not shown.
    local rosterUp = false
    for slot = 0, roster - 1 do
        rosterUp = rosterUp or p4.mem.read("u8", awards + AWARD_ROSTER_PREVIEW + slot * 0x88) > 0
    end
    local heroUp = heroLevel < 99 and heroThreshold(heroLevel + 1) <= heroExp + heroAward
    local flags = p4.mem.read("u32", awards + AWARD_FLAGS)
    local kept = bit32.band(flags, bit32.bnot(bit32.bor(if heroUp then 0 else 2, if rosterUp then 0 else 8,
        if partyUp then 0 else 0x10)))
    if kept ~= flags then
        p4.mem.write("u32", awards + AWARD_FLAGS, kept)
        log(string.format("CAP_RESULT flags=0x%x shown=0x%x", flags, kept))
    end
end)

-- No fusion: every fusion kind is withheld from the Velvet Room ----------------

p4.authorize.on("fusion", function(context: P4FusionContext): (boolean, ...string)
    if active("no_fusion") then
        return false, "NO FUSION challenge rule"
    end
    return true
end)

-- Item-backed death prevention is separate from passive and social-link Endure.
p4.authorize.on("battle_protection_item", function(_context: P4BattleProtectionItemContext): (boolean, ...string)
    if active("no_battle_items") then
        return false, "NO ITEMS IN BATTLE challenge rule"
    end
    return true
end)

-- Both the Velvet offer and the forge's safe-point commit ask this authorizer.
-- Persona fusion is a separate operation and remains independently configurable.
p4.authorize.on("weapon_fusion", function(_context: P4WeaponFusionContext): (boolean, ...string)
    if active("no_weapon_fusion") then
        return false, "NO WEAPON FUSION challenge rule"
    end
    return true
end)

p4.hook(A.datSetHeroExp, "vhI", "replace", function(context)
    if active("boss_cap") and context.args[1] == 1 then
        local cap = currentCap()
        local requested = context.args[2] :: number
        if cap < 99 then
            local current = p4.call(A.datPersonaGetExpById, "Ih", 1) :: number
            local limit = math.max(current, heroThreshold(cap + 1) - 1)
            if requested > limit then
                context.args[2] = limit
                log(string.format("CAP_HERO cap=%d requested=%d allowed=%d", cap, requested, limit))
            end
        end
    end
    context.next()
end)

p4.hook(A.datCalcSetLevel, "vpB", "replace", function(context)
    if active("boss_cap") then
        local hero = p4.call(A.datUnitGetRecord, "ph", 1) :: number
        if context.args[1] == hero then
            local cap = currentCap()
            local current = p4.call(A.datCalcGetLevelById, "ii", 1) :: number
            local requested = context.args[2] :: number
            local limit = math.max(cap, current)
            if requested > limit then
                context.args[2] = limit
                log(string.format("CAP_HERO_LEVEL cap=%d requested=%d allowed=%d", cap, requested, limit))
            end
        end
    end
    context.next()
end)

-- Solo member: the chosen character alone fights every battle ---------------
-- Every battle's lineup, at the one place any encounter route creates its
-- player-side units, is projected to the chosen character. Story party, field,
-- Social Links and recruitment are untouched; with one fighter the retail
-- rules withhold All-Out Attacks and follow-ups (they need a second one). A
-- chosen companion takes Direct Commands, earns the battle EXP (Yu, benched,
-- earns none), and their fall is the loss: Yu's record is knocked out before
-- the field's own Game Over check reads it.
p4.battle.formation(function(context: P4FormationContext): {number}?
    local member = soloId()
    local party = context.party
    if member == 0 or (#party == 1 and party[1] == member) then return nil end
    -- Snapshot mode: no logging here; battle_end reports the first projection.
    if soloLineup == nil then
        soloLineup = table.concat(party :: {any}, ",")
    end
    return {member}
end)

p4.hook(A.datCalcUnitClass, "ip", "replace", function(context)
    local member = soloId()
    if member > 1 and p4.mem.read("u16", (context.args[1] :: number) + 2) == member then
        context.result = 4 -- Direct Commands for the chosen fighter only
        return
    end
    context.result = context.next()
end)

-- Yu and his own Personas did not fight: their prepared awards and level-up
-- screens are withdrawn before anything reads them.
p4.hook(A.btlXpPrepareAwards, "vpp", "after", function(context)
    if soloId() <= 1 then return end
    local awards = context.args[1] :: number
    p4.mem.write("i32", awards + AWARD_HERO_EXP, 0)
    for slot = 0, 11 do p4.mem.write("i32", awards + AWARD_ROSTER_EXP + slot * 4, 0) end
    local flags = p4.mem.read("u32", awards + AWARD_FLAGS)
    p4.mem.write("u32", awards + AWARD_FLAGS, bit32.band(flags, bit32.bnot(2 + 8)))
end)

-- The story reloads a companion's Persona from its template
-- (scrCommonLoadPersonaRecord: datPersonaLoadRecord replays the template from
-- its join level with the record's EXP on top, keeping skills), at a join or a
-- Social Link evolution. The chosen companion takes the template's Persona
-- (an evolution happens) but keeps the earned level, EXP and stats; the
-- carried skills are the record's own.
p4.hook(A.scrCommonLoadPersonaRecord, "i", "replace", function(context)
    local member = soloId()
    if member <= 1 or p4.call(A.scrGetParamInt, "ii", 0) ~= member then
        context.result = context.next()
        return
    end
    local before = p4.party.record(member)
    context.result = context.next()
    local after = p4.party.record(member)
    if not before or not after then return end
    local ok, why = p4.party.set(member, {persona = after.persona, level = before.level, exp = before.exp,
        stats = before.stats, skills = after.skills})
    log(string.format("SOLO_RECORD_KEPT member=%s persona=%d->%d level=%d template_level=%d ok=%s why=%s",
        SOLO_NAMES[member], before.persona, after.persona, before.level, after.level, tostring(ok), tostring(why)))
end)

-- After a battle the chosen companion's record is brought onto the curve at the
-- next frame outside battle (stats up to the join level, scheduled skills).
local function checkCurve(): boolean
    local current = run
    if not current or not current.curve then return true end
    local record = p4.party.record(current.solo)
    if not record then return true end
    local target = curveTarget(current, record)
    if not target then return true end
    local ok = p4.party.set(current.solo, target)
    if ok then
        log(string.format("SOLO_CURVE_STEP member=%s level=%d stats=%s skills=%s", SOLO_NAMES[current.solo],
            target.level, table.concat(target.stats :: {any}, ","), table.concat(target.skills :: {any}, ",")))
    end
    return ok
end
local check = p4.events.on("frame_start", function()
    if checkCurve() and curveCheck then (curveCheck :: P4Subscription):disable() end
end)
check:disable()
curveCheck = check

-- Severe damage: our own asymmetric curve in the native final-damage stage ---
-- A MULTIPLY modifier of the final-damage stage (0x00235520), which offers only
-- eligible hits: the retail non-fixed HP damage equation, never fixed,
-- percentage or scripted amounts, SP, misses, nulls, drains or heals. Enemy
-- hits on the party x1.5, party hits on enemies x0.75; a critical or weakness
-- hit x1.25 more. A repel is judged by who it was aimed at (origin), so it is
-- scaled once. The stage composes and rounds; no RNG is drawn.
p4.damage.on({kind = "multiply", fn = function(hit: P4DamageHit): number?
    if not active("severe_damage") or hit.source_side == hit.origin_side then return nil end
    local factor = if hit.source_side == "enemy" then 1.5 else 0.75
    local boosted = hit.hit_kind == "critical" or hit.hit_kind == "weak"
    if boosted then factor *= 1.25 end
    local key = hit.source_side .. (if boosted then "+" else "")
    if hit.resolved and not severeLogged[key] and not severeSeen[key] then
        -- Snapshot frame: no logging here.
        severeSeen[key] = string.format("SEVERE_DAMAGE from_enemy=%s kind=%s stock=%d factor=%g",
            tostring(hit.source_side == "enemy"), hit.hit_kind, hit.stock, factor)
    end
    return factor
end})

-- Mandatory battles only: optional field encounters never start ------------
-- The game marks an encounter escapable (flags bit 0) exactly when it is an
-- ordinary field shadow; bosses, minibosses and scripted fights are not, and
-- scripted fights never pass through these field seams. A refused shadow
-- keeps moving normally: the player's swing finds no target, and a touching
-- shadow is not reported as a contact, so no battle, reward or field
-- transition starts and no RNG is drawn on the refused path.
local noContact = p4.mem.alloc(4) -- stays zero: "no shadow in contact"
local function optionalEncounter(actor: number): (boolean, number)
    if actor == 0 then return false, 0 end
    local unit = p4.mem.read("u32", actor + 0x48)
    if unit == 0 then return false, 0 end
    local id = p4.mem.read("u16", unit + 8)
    local flags = p4.mem.read("u32", p4.mem.read("u32", ENCOUNTER_TABLE) + id * 0x18)
    return bit32.band(flags, 1) ~= 0, id
end
local function encounterRefused(id: number, how: string)
    local key = "e" .. id .. how
    if not encounterRefusals[key] then
        encounterRefusals[key] = true
        log(string.format("ENCOUNTER_REFUSED encounter=%d via=%s", id, how))
    end
end
p4.hook(A.FldAmbientFlagPtr, "p", "replace", function(context)
    local real = context.next() :: number
    context.result = real
    if not active("mandatory_only") then return end
    local optional, id = optionalEncounter(p4.mem.read("u32", real))
    if optional then
        context.result = noContact.address
        encounterRefused(id, "contact")
    end
end)
p4.hook(A.FldEvent_FindNearestVisibleActor, "ppff", "replace", function(context)
    local found = context.next() :: number
    context.result = found
    if found == 0 or not active("mandatory_only") then return end
    local optional, id = optionalEncounter(found)
    if optional then
        context.result = 0
        encounterRefused(id, "attack")
    end
end)

-- No items in battle: the native Item command is unavailable ----------------

p4.hook(A.btlTargetCheckCommandAvailability, "ipq", "replace", function(context)
    if active("no_battle_items") then
        local _, kind = p4.split64(context.args[2] :: buffer)
        if kind == 3 then
            if not itemRefusalLogged then
                itemRefusalLogged = true
                log("BATTLE_ITEMS_REFUSED run=" .. (run :: Run).id)
            end
            context.result = 0
            return
        end
    end
    context.result = context.next()
end)

-- No field recovery: Camp refuses HP/SP-restoring items and skills -----------

p4.hook(A.radarEntryTypeIsActive, "ih", "replace", function(context)
    local available = context.next() :: number
    if available ~= 0 and active("no_field_recovery") then
        local entry = p4.call(A.datEntryAt, "ph", context.args[1] :: number) :: number
        local effect = p4.mem.read("u16", entry + 0x24)
        if restoresHpOrSp(effect) then
            if not refusalsLogged["i" .. tostring(context.args[1])] then
                refusalsLogged["i" .. tostring(context.args[1])] = true
                log("FIELD_RECOVERY_REFUSED item=" .. tostring(context.args[1]) .. " effect=" .. effect)
            end
            available = 0
        end
    end
    context.result = available
end)

p4.hook(A.datCalcThresholdTriggerById, "ihI", "replace", function(context)
    local blocked = context.next() :: number
    if blocked == 0 and campOpen and active("no_field_recovery") and restoresHpOrSp(context.args[2] :: number) then
        if not refusalsLogged["s" .. tostring(context.args[2])] then
            refusalsLogged["s" .. tostring(context.args[2])] = true
            log("FIELD_RECOVERY_REFUSED skill=" .. tostring(context.args[2]))
        end
        blocked = 1
    end
    context.result = blocked
end)

-- Session identity: the port's, one per launch, so a reload in this same
-- session is never mistaken for a crash recovery.
do
    local id, why = p4.lifecycle.session()
    if id then
        session = id
    else
        log("CHALLENGE_SESSION unavailable reason=" .. tostring(why))
    end
end
reconcileStrict()

log("CHALLENGE_READY session=" .. session)
