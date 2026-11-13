--!strict
-- p4pc.practice: practice and developer tools (docs/modding/lua.md "Game services").
-- Every feature has its own persistent toggle, OFF by default. Native Config
-- pages only stage values; a confirmed change is committed by the game thread
-- at the next frame boundary, after its lifecycle state is checked again.
-- Any committed change or running tool marks the session PRACTICE, and a
-- PRACTICE session refuses to save, so practice state never reaches a normal
-- save.

local VERSION = "1.1.0"

-- ---------------------------------------------------------------- helpers

local function hex(value: number, digits: number): string
    local text = string.format("%X", value)
    return "0x" .. string.rep("0", digits - #text) .. text
end

local function idText(value: number, digits: number): string
    return tostring(value) .. " (" .. hex(value, digits) .. ")"
end

local function clamp(value: number, low: number, high: number): number
    if value < low then return low end
    if value > high then return high end
    return value
end

local function log(text: string)
    p4.log(text)
end

-- P18 owns only the passive grants it made itself (owner == p4.id); another
-- mod's grant of the same skill is never read as, or taken back as, ours.
local function practiceHolds(member: number, skill: number): boolean
    for _, grant in p4.passive.list() do
        if grant.owner == p4.id and grant.kind == "party" and grant.id == member and grant.skill == skill then
            return true
        end
    end
    return false
end
local function otherHolds(member: number, skill: number): boolean
    for _, grant in p4.passive.list() do
        if grant.owner ~= p4.id and grant.kind == "party" and grant.id == member and grant.skill == skill then
            return true
        end
    end
    return false
end

-- ---------------------------------------------------------------- features

type Feature = { key: string, code: string, label: string }
local FEATURES: { [string]: Feature } = {
    warp = { key = "warp", code = "P01", label = "WARP" },
    date = { key = "date", code = "P02", label = "SET DATE" },
    time = { key = "time", code = "P03", label = "SET TIME SLOT" },
    weather = { key = "weather", code = "P04", label = "SET WEATHER" },
    stats = { key = "stats", code = "P05", label = "PARTY STATS" },
    levels = { key = "levels", code = "P06", label = "LEVEL / EXP" },
    personas = { key = "personas", code = "P09", label = "PERSONAS" },
    money = { key = "money", code = "P07", label = "MONEY" },
    items = { key = "items", code = "P08", label = "ITEMS" },
    links = { key = "links", code = "P10", label = "SOCIAL LINK RANKS" },
    flags = { key = "flags", code = "P11", label = "STORY FLAGS" },
    encounter = { key = "encounter", code = "P12", label = "START ENCOUNTER" },
    camera = { key = "camera", code = "P13", label = "FREE CAMERA" },
    hud = { key = "hud", code = "P14", label = "HIDE HUD" },
    frames = { key = "frames", code = "P16", label = "FRAME COUNTER" },
    pace = { key = "pace", code = "P15", label = "PRACTICE SPEED" },
    timer = { key = "timer", code = "P17", label = "PRACTICE TIMER" },
    passives = { key = "passives", code = "P18", label = "PASSIVE SKILLS" },
}

-- Toggle changes made on our pages are written by the frame pump (settings
-- writes are not allowed inside synchronous UI callbacks).
local pendingSettings: { [string]: boolean } = {}

local function enabled(key: string): boolean
    local staged = pendingSettings[key]
    if staged ~= nil then return staged end
    return p4.settings.get(key) == true
end

-- ---------------------------------------------------------------- session

local session = {
    practice = false,   -- this session used a practice feature
    reasons = {} :: { [string]: boolean },
    edits = 0,
    loadedPractice = false,
    epoch = 0,          -- bumped by every load and New Game: a new timeline
}

local refreshHud: () -> ()

local function markPractice(reason: string)
    if not session.reasons[reason] then
        session.reasons[reason] = true
        log("PRACTICE_SESSION marked by " .. reason)
    end
    if not session.practice then
        session.practice = true
    end
    refreshHud()
end

local function sessionSummary(): string
    if not session.practice then return "CLEAN" end
    local names = {}
    for reason in session.reasons do table.insert(names, reason) end
    table.sort(names)
    return "PRACTICE (" .. table.concat(names, ", ") .. ")"
end

p4.save.register({
    schema = 1,
    max_bytes = 1024,
    snapshot = function(): P4SaveValue
        if not session.practice then return nil end
        local reasons: { [string]: P4SaveValue } = {}
        for reason in session.reasons do reasons[reason] = true end
        return { version = VERSION, edits = session.edits, reasons = reasons }
    end,
    -- Practice state never reaches a normal save: a PRACTICE session refuses
    -- the save before any byte of the slot is written.
    authorize_save = function(): (boolean, ...string)
        if session.practice then
            return false, "PRACTICE SESSION - SAVING IS DISABLED"
        end
        return true
    end,
    load = function(value: P4SaveValue, origin: "load" | "reset")
        session.reasons = {}
        session.edits = 0
        session.practice = false
        session.loadedPractice = false
        session.epoch += 1
        -- P18 grants belong to the session they were made in.
        local cleared = p4.passive.clear()
        if cleared > 0 then log("PRACTICE_PASSIVES cleared=" .. tostring(cleared) .. " origin=" .. origin) end
        if type(value) == "table" then
            session.practice = true
            session.loadedPractice = true
            session.reasons["loaded practice save"] = true
            local reasons = value.reasons
            if type(reasons) == "table" then
                for reason in reasons do
                    if type(reason) == "string" then session.reasons[reason] = true end
                end
            end
        end
        log("PRACTICE_LOAD origin=" .. origin .. " session=" .. sessionSummary())
        refreshHud()
    end,
})

-- ---------------------------------------------------------------- guest data

-- Retail data read for display only; every change goes through the game's
-- own setter at the frame boundary.
local MONEY = 0x0079B68C
local FLAG_WORDS = 0x0079B1CC
local DATE = 0x00797B7A
local TIME_SLOT = 0x00797B7C
local ITEM_COUNTS = 0x0079757A
-- Character records (src/g_data/g_data.c): the protagonist's unit at
-- 0x007973C4, party characters 2..10 in 0x88-byte slots; level byte at +6,
-- current HP/SP halfwords at +8/+0xA. Protagonist social points s16[5].
-- PARTY_UNITS + id * 0x88 is character id's unit (slot + 4).
local HERO_UNIT = 0x007973C4
local PARTY_UNITS = 0x00796D44
local HERO_SOCIAL = 0x007973F4
local PARTY_IDS = 0x00797560
-- P06: the protagonist's EXP (u32), the EXP thresholds (u32[99], first entry
-- for level 2; datGetExperienceToNextLevel 0x00105A50) and the level table
-- (22 halfwords per level, the protagonist's base HP/SP at +4/+6;
-- datGetLevelTableFirstValue 0x001053B0).
local HERO_EXP = 0x00797400
local EXP_TABLE = 0x005DD6E0
local LEVEL_TABLE = 0x005DD844
-- P09 (src/Main/Battle/Data/datPersona.c): roster records of 0x30 bytes from
-- 0x00797F8C (flags bit 0 active, id +2, level +4, EXP +8, skills u16[8] at
-- +0xC, stats u8[5] at +0x1C), the lead slot halfword at 0x00797F88 and the
-- capacity mode flags at 0x0079B40C (datPersonaGetRosterCapacity 0x0010B5B0).
-- The Persona table pointer at 0x007644C4 has 14-byte rows: flags u16, arcana
-- +2, base level +3; fusion considers ids below 0xC0 (fclCombineMisc.c).
local ROSTER = 0x00797F8C
local ROSTER_LEAD = 0x00797F88
local ROSTER_MODE = 0x0079B40C
local PERSONA_TABLE = 0x007644C4
local PERSONA_IDS = 0xC0
-- P04 (datCalendar_inferred_001228c0.c, unplaced_inferred_00110850.c): the
-- calendar state pointer (gp-0x4E24) holds the day (+4) and period (+6) it
-- was synchronized to and that slot's weather (+8, previous value +0xA).
-- calendarSyncFromDate (0x00123850) stores the disc's weather there (day
-- table of 6 bytes per day from 0x005E3A02: periods 0-1 at +0, 2-5 at +2);
-- while the day and period are current, calGetDayEntryByPhase (0x00110960)
-- returns this value instead of the table. The game itself replaces it for
-- story fields with calendarPushValue (0x00123AA0) and restores it with
-- calendarPopValue (0x00123AC0).
local CALENDAR_STATE = 0x007642CC
local DAY_WEATHER = 0x005E3A02
local function weatherOfDay(): number
    local date, slot = p4.mem.read("i16", 0x00797B7A), p4.mem.read("u8", 0x00797B7C)
    return p4.mem.read("i8", DAY_WEATHER + date * 6 + (if slot >= 2 then 2 else 0))
end
-- The weather the calendar serves now when it differs from the disc's.
local function weatherOverride(): number?
    local state = p4.mem.read("u32", CALENDAR_STATE)
    if state == 0 then return nil end
    if p4.mem.read("i16", state + 4) ~= p4.mem.read("i16", 0x00797B7A) or
        p4.mem.read("i16", state + 6) ~= p4.mem.read("u8", 0x00797B7C) then return nil end
    local value = p4.mem.read("i8", state + 8)
    return if value == weatherOfDay() then nil else value
end

local function rosterCapacity(): number
    local mode = p4.mem.read("u32", ROSTER_MODE)
    if bit32.btest(mode, 4) then return 12 end
    if bit32.btest(mode, 2) then return 10 end
    if bit32.btest(mode, 1) then return 8 end
    return 6
end
local function rosterActive(slot: number): boolean
    return slot >= 0 and slot < rosterCapacity() and bit32.btest(p4.mem.read("u16", ROSTER + slot * 0x30), 1)
end
local function rosterId(slot: number): number return p4.mem.read("u16", ROSTER + slot * 0x30 + 2) end
local function rosterCount(): number
    local count = 0
    for slot = 0, rosterCapacity() - 1 do
        if rosterActive(slot) then count += 1 end
    end
    return count
end
local function rosterFind(id: number): number?
    for slot = 0, rosterCapacity() - 1 do
        if rosterActive(slot) and rosterId(slot) == id then return slot end
    end
    return nil
end
-- A Persona id the hero may carry: a populated table row (base level > 0)
-- that fusion may produce (flags bits 0 and 1 clear).
local function personaRow(id: number): number?
    local base = p4.mem.read("u32", PERSONA_TABLE)
    if base == 0 or id <= 0 or id >= PERSONA_IDS then return nil end
    local row = base + id * 14
    if p4.mem.read("u8", row + 3) == 0 or bit32.btest(p4.mem.read("u16", row), 3) then return nil end
    return row
end
local ITEM_RETAIL = 1536
local LINK_TABLE = 0x00797410
local LINK_RECORDS = 0x15
local MONTH_LENGTHS = 0x005E45E0

local function readMoney(): number return p4.mem.read("u32", MONEY) end
local function readFlag(id: number): boolean
    local word = p4.mem.read("u32", FLAG_WORDS + (id // 32) * 4)
    return bit32.btest(word, bit32.lshift(1, id % 32))
end
local function readDate(): number return p4.mem.read("i16", DATE) end
local function readTimeSlot(): number return p4.mem.read("u8", TIME_SLOT) end
local function readItemCount(id: number): number return p4.mem.read("u8", ITEM_COUNTS + id) end

local function dateText(index: number): string
    -- calSplitDayOfYear (001104d0): day 0 is 1 April; month lengths from the
    -- game's own table. Weekday as calGetWeekdayOfDay (00110580), 0 Sunday.
    local remaining = index % 365
    local month = 4
    for _ = 1, 12 do
        local length = p4.mem.read("i32", MONTH_LENGTHS + ((month - 1) % 12) * 4)
        if remaining < length then break end
        remaining -= length
        month = month % 12 + 1
    end
    local weekdays = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" }
    local weekday = (((index % 365) % 7) + 5) % 7
    return string.format("%02d/%02d %s  (DAY %d)", month, remaining + 1, weekdays[weekday + 1], index)
end

-- Independently written labels for the six slots datSetTimeSlot stores.
local TIME_NAMES = { "EARLY MORNING", "MORNING", "LUNCHTIME", "AFTERNOON", "AFTER SCHOOL", "EVENING" }
local function slotText(slot: number): string
    return "SLOT " .. tostring(slot) .. " " .. (TIME_NAMES[slot + 1] or "UNKNOWN")
end

-- Item catalog, built at frame boundaries through the game's own accessors
-- (datGetItemNameId 0x001067f0, datEntryGetKind 0x00106850), which also
-- answer for expanded IDs. Unused retail rows are named "Blank" or carry an
-- "0x" placeholder; those are not inventory rows.
local catalog = {
    ids = {} :: { number }, names = {} :: { [number]: string }, kinds = {} :: { [number]: number },
    next = 0, done = false,
}

local function buildCatalog(budget: number)
    local id = catalog.next
    local stop = math.min(ITEM_RETAIL, id + budget)
    while id < stop do
        local address = p4.call(0x001067F0, "Ih", id) :: number
        local name = if address > 0 then p4.mem.read_string(address, 40) else ""
        if name ~= "" and name ~= "Blank" and string.sub(name, 1, 2) ~= "0x" and not string.find(name, "[^%w%p ]") then
            table.insert(catalog.ids, id)
            catalog.names[id] = string.upper(name)
            catalog.kinds[id] = p4.call(0x00106850, "Ih", id) :: number
        end
        id += 1
    end
    catalog.next = id
    catalog.done = id >= ITEM_RETAIL
end

local function itemLabel(id: number): string
    return catalog.names[id] or ("UNKNOWN ID " .. hex(id, 4))
end

local function stepItem(id: number, direction: number): number
    local ids = catalog.ids
    if #ids == 0 then return id end
    if direction > 0 then
        for _, candidate in ids do
            if candidate > id then return candidate end
        end
        return ids[1]
    end
    for i = #ids, 1, -1 do
        if ids[i] < id then return ids[i] end
    end
    return ids[#ids]
end

type Link = { id: number, rank: number, slot: number }
local function readLinks(): { Link }
    local links: { Link } = {}
    for slot = 0, LINK_RECORDS - 1 do
        local base = LINK_TABLE + slot * 16
        local id = p4.mem.read("u16", base + 4)
        if id ~= 0 and id < 0x1F then
            table.insert(links, { id = id, rank = p4.mem.read("u16", base + 6), slot = slot })
        end
    end
    return links
end
local function linkRank(id: number): number?
    for _, link in readLinks() do
        if link.id == id then return link.rank end
    end
    return nil
end

-- ---------------------------------------------------------------- commits

type Op = { kind: string, a: number, b: number, label: string, field: string? }
local queue: { Op } = {}
local results: { [string]: string } = {}

local pump: P4Subscription
local updatePump: () -> ()

local function editable(): (boolean, string?)
    local state = p4.scene.state()
    if state.context ~= "field" and state.context ~= "dungeon" then
        return false, "LOAD A GAME AND OPEN CONFIG FROM A FIELD"
    end
    if state.pending then return false, "A SCENE CHANGE IS PENDING" end
    if state.event then return false, "AN EVENT HOLDS THE GAME" end
    return true, nil
end

local function commit(op: Op): string
    local ok, why = editable()
    if not ok then return "REFUSED: " .. (why or "NOT AVAILABLE") end
    local kind = op.kind
    local before, after
    if kind == "money" then
        before = readMoney()
        p4.call(0x00105FA0, "vI", op.a)
        after = readMoney()
        if after ~= op.a then return "FAILED: READ BACK " .. tostring(after) end
    elseif kind == "flags" then
        before = readFlag(op.a) and 1 or 0
        p4.call(0x00106390, "vii", op.a, op.b)
        after = readFlag(op.a) and 1 or 0
        if after ~= op.b then return "FAILED: READ BACK " .. tostring(after) end
    elseif kind == "items" then
        -- Only from an idle field or the native Config: never while a shop,
        -- battle or Camp item list holds a list built before the edit.
        local state = p4.scene.state()
        if not (state.ready or state.menu) then return "REFUSED: OPEN CONFIG FROM A FIELD" end
        before = p4.call(0x00106600, "ih", op.a)
        p4.call(0x00106620, "vhi", op.a, op.b)
        after = p4.call(0x00106600, "ih", op.a)
        if after ~= op.b then return "FAILED: READ BACK " .. tostring(after) end
    elseif kind == "date" then
        before = readDate()
        p4.call(0x00106100, "vh", op.a)
        after = readDate()
        if after ~= op.a then return "FAILED: READ BACK " .. tostring(after) end
    elseif kind == "time" then
        before = readTimeSlot()
        p4.call(0x001062F0, "vB", op.a)
        after = readTimeSlot()
        if after ~= op.a then return "FAILED: READ BACK " .. tostring(after) end
    elseif kind == "links" then
        local rank = linkRank(op.a)
        if rank == nil then return "REFUSED: NO SUCH SOCIAL LINK RECORD" end
        before = rank
        p4.call(0x00108B60, "vii", op.a, op.b)
        p4.call(0x00106F40, "vi", op.a)
        after = linkRank(op.a)
        if after ~= op.b then return "FAILED: READ BACK " .. tostring(after) end
    elseif kind == "levels" then
        -- One protagonist record transaction: the level byte and the minimum
        -- EXP of that level, both through the game's setters.
        before = "LV" .. tostring(p4.mem.read("u8", HERO_UNIT + 6)) .. " EXP" .. tostring(p4.mem.read("u32", HERO_EXP))
        p4.call(0x00105690, "vhB", 1, op.a)
        p4.call(0x00105990, "vhI", 1, op.b)
        if op.field == "normalize" then p4.call(0x00105780, "vh", 1) end
        local level, exp = p4.mem.read("u8", HERO_UNIT + 6), p4.mem.read("u32", HERO_EXP)
        after = "LV" .. tostring(level) .. " EXP" .. tostring(exp)
        if level ~= op.a or exp ~= op.b then return "FAILED: READ BACK " .. after end
    elseif kind == "weather" then
        local state = p4.mem.read("u32", CALENDAR_STATE)
        if state == 0 then return "REFUSED: NO CALENDAR STATE" end
        if p4.mem.read("i16", state + 4) ~= readDate() or p4.mem.read("i16", state + 6) ~= readTimeSlot() then
            -- The game's own resynchronization to the current date and slot.
            p4.call(0x00123850, "v")
            if p4.mem.read("i16", state + 4) ~= readDate() or p4.mem.read("i16", state + 6) ~= readTimeSlot() then
                return "FAILED: THE CALENDAR DID NOT SYNCHRONIZE"
            end
        end
        before = weatherOverride() or weatherOfDay()
        if op.field == "release" then
            if weatherOverride() == nil then return "REFUSED: NO OVERRIDE" end
            p4.call(0x00123AC0, "v")
            after = weatherOverride() or weatherOfDay()
        else
            p4.call(0x00123AA0, "vh", op.a)
            after = p4.mem.read("i8", state + 8)
            if after ~= op.a then return "FAILED: READ BACK " .. tostring(after) end
        end
    elseif kind == "personas" then
        local field = op.field or ""
        if field == "add" then
            if rosterFind(op.a) then return "REFUSED: ALREADY CARRIED" end
            if not personaRow(op.a) then return "REFUSED: NOT A CARRIABLE PERSONA" end
            before = rosterCount()
            -- The non-asserting try-add (0x0010B060) returns NULL when the roster is full.
            local record = p4.call(0x0010B060, "pH", op.a)
            after = rosterCount()
            if record == 0 or record == nil then return "REFUSED: THE ROSTER IS FULL" end
            if not rosterFind(op.a) then return "FAILED: READ BACK" end
        elseif field == "remove" then
            if not rosterActive(op.a) then return "REFUSED: EMPTY SLOT" end
            if rosterCount() <= 1 then return "REFUSED: THE LAST PERSONA STAYS" end
            local id = rosterId(op.a)
            before = id
            -- Shifts later slots down and reselects the lead (0x0010AE30).
            p4.call(0x0010AE30, "ii", op.a)
            if rosterFind(id) then return "FAILED: READ BACK" end
            after = rosterCount()
        elseif field == "lead" then
            if not rosterActive(op.a) then return "REFUSED: EMPTY SLOT" end
            before = p4.mem.read("i16", ROSTER_LEAD)
            p4.call(0x0010B3B0, "ii", op.a)
            after = p4.mem.read("i16", ROSTER_LEAD)
            if after ~= op.a then return "FAILED: READ BACK " .. tostring(after) end
        elseif string.sub(field, 1, 4) == "stat" then
            local index = tonumber(string.sub(field, 5)) or 0
            if not rosterActive(op.a) then return "REFUSED: EMPTY SLOT" end
            local record = ROSTER + op.a * 0x30
            local value = p4.mem.read("u8", record + 0x1C + index)
            before = value
            -- datPersonaAddStat (0x0010A780) clamps the result to 0..99.
            p4.call(0x0010A780, "ipii", record, index, op.b - value)
            after = p4.mem.read("u8", record + 0x1C + index)
            if after ~= op.b then return "FAILED: READ BACK " .. tostring(after) end
        else
            return "REFUSED: UNKNOWN PERSONA OPERATION"
        end
    elseif kind == "stats" then
        local unit, field = op.a, op.field or ""
        if field == "hp" then
            before = p4.call(0x00104CE0, "Ih", unit)
            p4.call(0x001056E0, "vhh", unit, op.b)
            after = p4.call(0x00104CE0, "Ih", unit)
        elseif field == "sp" then
            before = p4.call(0x00104D50, "Ih", unit)
            p4.call(0x00105730, "vhh", unit, op.b)
            after = p4.call(0x00104D50, "Ih", unit)
        elseif field == "normalize" then
            -- The game's own clamp of current HP/SP to their maxima.
            before = tostring(p4.call(0x00104CE0, "Ih", unit)) .. "/" .. tostring(p4.call(0x00104D50, "Ih", unit))
            p4.call(0x00105780, "vh", unit)
            after = tostring(p4.call(0x00104CE0, "Ih", unit)) .. "/" .. tostring(p4.call(0x00104D50, "Ih", unit))
        elseif string.sub(field, 1, 6) == "social" and unit == 1 then
            local category = tonumber(string.sub(field, 7)) or 0
            before = p4.call(0x00104EA0, "hhh", 1, category)
            p4.call(0x001051A0, "vhhh", 1, category, op.b)
            after = p4.call(0x00104EA0, "hhh", 1, category)
        else
            return "REFUSED: UNKNOWN STAT"
        end
        if field ~= "normalize" and after ~= op.b then return "FAILED: READ BACK " .. tostring(after) end
    elseif kind == "passives" then
        -- p4.passive (passive.grant@1): the member has the skill in every
        -- passive check the game makes; the skill list never changes. Only
        -- P18's own grant is made or taken back; a full grant table refuses.
        local who: number | P4PassiveCharacter = op.a
        before = if practiceHolds(op.a, op.b) then "GRANTED" else "NONE"
        local ok, why = pcall(function()
            if op.field == "revoke" then p4.passive.revoke({ party = who }, op.b) else p4.passive.grant({ party = who }, op.b) end
        end)
        if not ok then
            local text = string.upper(tostring(why))
            if string.find(text, "GRANT TABLE IS FULL", 1, true) then return "REFUSED: EVERY MOD'S GRANTS FILL THE TABLE" end
            return "REFUSED: " .. text
        end
        after = if practiceHolds(op.a, op.b) then "GRANTED" else "NONE"
        if (after == "GRANTED") == (op.field == "revoke") then return "FAILED: READ BACK " .. tostring(after) end
    else
        return "REFUSED: UNKNOWN OPERATION"
    end
    session.edits += 1
    markPractice(FEATURES[kind].code)
    log(string.format("PRACTICE_APPLY %s %s before=%s after=%s", kind, op.label, tostring(before), tostring(after)))
    return "APPLIED: " .. tostring(before) .. " -> " .. tostring(after)
end

local function enqueue(op: Op)
    table.insert(queue, op)
    results[op.kind] = "QUEUED"
    updatePump()
end

-- ---------------------------------------------------------------- timer / counters

local timer = {
    running = false,
    startWall = 0, startUpdates = 0,
    wall = 0, updates = 0,      -- accumulated while stopped
    splits = {} :: { string },
    epoch = 0,
    marks = {} :: { [string]: boolean },
}
local counters = { baseUpdates = 0, baseRetail = 0, baseFrame = 0, frame = 0, epoch = 0, resets = 0 }

local function clockText(seconds: number): string
    local minutes = seconds // 60
    return string.format("%d:%05.2f", minutes, seconds - minutes * 60)
end

local function timerValues(): (number, number)
    if not timer.running then return timer.wall, timer.updates end
    return timer.wall + (p4.clock.wall() - timer.startWall), timer.updates + (p4.clock.updates() - timer.startUpdates)
end

local function timerLine(): string
    local wall, updates = timerValues()
    local state = if timer.running then "" else " STOP"
    local marked = if next(timer.marks) then " MOD" else ""
    return "T " .. clockText(wall) .. "  UPD " .. tostring(updates) .. state .. marked
end

local function counterLine(): string
    local updates = p4.clock.updates() - counters.baseUpdates
    local retail = p4.clock.retail()
    return "UPD " .. tostring(updates) .. "  RET " .. tostring(retail) .. "  FLIP " .. tostring(counters.frame - counters.baseFrame)
end

-- ---------------------------------------------------------------- practice speed (P15)

-- p4.pace changes wall time only: it never marks the session PRACTICE. A
-- staged rate is set at the next frame (not from a UI callback); L3 restores
-- 1x from gameplay without opening the Config.
-- actual: presented game frames per wall second over the last second, as a
-- multiple of the game's 59.94 Hz, so a host that cannot keep up shows it.
local paceTool = { staged = 1, pending = nil :: number?, result = "-", shown = "", actual = 0,
    sampleFrame = 0, sampleWall = 0 }
local function paceText(rate: number): string
    if rate == 0.25 then return "0.25X" elseif rate == 0.5 then return "0.5X" end
    return tostring(math.floor(rate)) .. "X"
end
local function paceFrame()
    local pending = paceTool.pending
    if pending then
        paceTool.pending = nil
        local ok, why = p4.pace.set(pending)
        paceTool.result = if ok then "SET " .. paceText(pending) else "REFUSED: " .. string.upper(why or "")
        log("PRACTICE_PACE set " .. tostring(pending) .. " " .. paceTool.result)
    end
    local requested, effective, held = p4.pace.state()
    local wall = p4.clock.wall()
    if wall - paceTool.sampleWall >= 1 then
        if paceTool.sampleWall > 0 then
            paceTool.actual = (counters.frame - paceTool.sampleFrame) / (wall - paceTool.sampleWall) / 59.94
        end
        paceTool.sampleWall, paceTool.sampleFrame = wall, counters.frame
    end
    local shown = tostring(effective) .. " " .. (held or "")
    if shown ~= paceTool.shown then
        paceTool.shown = shown
        log("PRACTICE_PACE now " .. tostring(effective) .. "x" .. (if held then " held: " .. held else ""))
    end
    if requested ~= 1 then
        local _, restore = p4.input.action("l3")
        if restore or not enabled("pace") then
            p4.pace.set(1)
            paceTool.staged = 1
            paceTool.result = "RESTORED 1X"
            log("PRACTICE_PACE restored 1x")
        end
    end
end
local function paceLine(): string?
    local requested, effective, held = p4.pace.state()
    if requested == 1 then return nil end
    return string.format("SPEED %s ACTUAL %.2fX", paceText(effective), paceTool.actual) ..
        (if held then " (" .. string.upper(held) .. ")" else "")
end

local lastHud = ""
refreshHud = function()
    local lines: { string } = {}
    if session.practice then table.insert(lines, "PRACTICE") end
    if enabled("timer") and (timer.running or timer.wall > 0) then table.insert(lines, timerLine()) end
    if enabled("frames") then table.insert(lines, counterLine()) end
    local speed = paceLine()
    if speed then table.insert(lines, speed) end
    local joined = table.concat(lines, "\n")
    if joined == lastHud then return end
    lastHud = joined
    if #lines == 0 then
        p4.ui.status(nil)
    elseif not p4.ui.status(lines, "hud") then
        log("PRACTICE_HUD refused: no free status rows")
    end
end

local function timerStart()
    if timer.running then return end
    timer.running = true
    timer.startWall = p4.clock.wall()
    timer.startUpdates = p4.clock.updates()
    timer.epoch = session.epoch
    markPractice("P17")
    log("PRACTICE_TIMER start")
end
local function timerStop()
    if not timer.running then return end
    local wall, updates = timerValues()
    timer.running = false
    timer.wall, timer.updates = wall, updates
    log("PRACTICE_TIMER stop " .. timerLine())
end
local function timerReset()
    timer.running = false
    timer.wall, timer.updates = 0, 0
    timer.splits = {}
    timer.marks = {}
    log("PRACTICE_TIMER reset")
end
local function timerSplit(source: string)
    if not timer.running then return end
    local wall, updates = timerValues()
    local text = clockText(wall) .. " / " .. tostring(updates) .. " " .. source
    table.insert(timer.splits, text)
    if #timer.splits > 8 then table.remove(timer.splits, 1) end
    log("PRACTICE_TIMER split " .. text)
end

-- ---------------------------------------------------------------- scene, camera and HUD tools

-- A confirmed warp, encounter or free camera needs the field idle with the
-- Config closed. It waits while the Config/Camp stays open and runs at the
-- first frame after it closes; if the field changed or is busy then, it is
-- refused and discarded, never retried later.
type SceneOp = { kind: string, field: number, room: number, encounter: number, fromField: number, fromRoom: number,
    settle: number }
local sceneOp: SceneOp? = nil
local watching: string? = nil -- the request whose completion is still to be logged

local cameraTool = { lease = nil :: P4CameraLease?, move = 5, turn = 5, state = "OFF" }
local hudWanted: { [string]: boolean } = {}

local function upper(text: string?): string
    return string.upper(text or "NOT AVAILABLE")
end

local function stageScene(op: SceneOp)
    sceneOp = op
    results[op.kind] = "WAITING FOR CONFIG TO CLOSE"
    updatePump()
end

local function runSceneOp()
    local op = sceneOp
    if not op then
        local kind = watching
        if kind then
            local state, detail, outcome = p4.scene.status()
            if state == "done" or state == "refused" then
                watching = nil
                log("PRACTICE_REQUEST " .. kind .. " " .. state .. " " .. detail .. " outcome=" .. tostring(outcome))
            end
        end
        return
    end
    local state = p4.scene.state()
    if state.menu then return end
    -- The player-control task returns a few frames after the Config closes.
    if not state.ready and op.settle < 60 and state.field == op.fromField and state.room == op.fromRoom then
        op.settle += 1
        return
    end
    sceneOp = nil
    if not state.ready or state.field ~= op.fromField or state.room ~= op.fromRoom then
        results[op.kind] = "REFUSED: " .. upper(state.reason or "THE FIELD CHANGED")
        log("PRACTICE_RESULT " .. op.kind .. " " .. results[op.kind])
        return
    end
    if op.kind == "warp" then
        local ok, why = p4.scene.warp({ field = op.field, room = op.room, entry = 0 })
        results.warp = if ok then "REQUESTED" else "REFUSED: " .. upper(why)
        if ok then
            session.edits += 1
            markPractice("P01")
            watching = "warp"
        end
    elseif op.kind == "encounter" then
        local ok, why = p4.encounter.launch(op.encounter)
        results.encounter = if ok then "REQUESTED" else "REFUSED: " .. upper(why)
        if ok then
            session.edits += 1
            markPractice("P12")
            watching = "encounter"
        end
    elseif op.kind == "camera" then
        local lease, why, owner = p4.camera.acquire("practice free camera")
        if lease then
            lease:input(true)
            cameraTool.lease = lease
            cameraTool.state = "ACTIVE"
            markPractice("P13")
        else
            cameraTool.state = "REFUSED: " .. upper(why) .. (owner and (" (" .. upper(owner) .. ")") or "")
        end
        results.camera = cameraTool.state
    end
    log("PRACTICE_RESULT " .. op.kind .. " " .. tostring(results[op.kind]) .. " target=" ..
        tostring(op.field) .. ":" .. tostring(op.room) .. " encounter=" .. tostring(op.encounter))
end

local function deadZone(value: number): number
    if math.abs(value) < 0.15 then return 0 end
    return value
end

-- Left stick moves in the view's horizontal plane, right stick looks, L1/R1
-- lower/raise, L2/R2 narrow/widen the view, Circle exits. The game receives
-- a neutral pad while the lease holds the input.
local function cameraFrame()
    local lease = cameraTool.lease
    if not lease then return end
    local active, why = lease:active()
    if not active then
        cameraTool.lease = nil
        cameraTool.state = "RELEASED: " .. upper(why)
        log("PRACTICE_CAMERA " .. cameraTool.state)
        return
    end
    local _, exit = p4.input.action("cancel")
    if exit then
        lease:release()
        cameraTool.lease = nil
        cameraTool.state = "EXITED"
        log("PRACTICE_CAMERA exited")
        return
    end
    local pose = lease:get()
    if not pose then return end
    local lx, ly, rx, ry = p4.input.sticks()
    lx, ly, rx, ry = deadZone(lx), deadZone(ly), deadZone(rx), deadZone(ry)
    local speed, turn = cameraTool.move * 2, cameraTool.turn * 0.5
    local yaw = math.rad(pose.yaw)
    local forward, strafe = -ly * speed, lx * speed
    local down = p4.input.action("l1")
    local up = p4.input.action("r1")
    local narrow = p4.input.action("l2")
    local widen = p4.input.action("r2")
    lease:set({
        x = pose.x + math.sin(yaw) * forward - math.cos(yaw) * strafe,
        z = pose.z + math.cos(yaw) * forward + math.sin(yaw) * strafe,
        y = pose.y + ((up and speed or 0) - (down and speed or 0)),
        yaw = pose.yaw - rx * turn,
        pitch = clamp(pose.pitch - ry * turn, -89, 89),
        fov = clamp(pose.fov + ((widen and 1 or 0) - (narrow and 1 or 0)), 10, 120),
    })
end

-- The P05 page's cache refresh, defined with the page below.
local statsOpen = false
local statsFrame: () -> () = function() end

-- Hides are per mod; p4.hud.hidden reports any mod's, so this mod keeps its
-- own record and learns from it that a scene change dropped its hide.
local hudMine: { [string]: boolean } = {}
local function hudFrame()
    local on = enabled("hud")
    for _, group in p4.hud.groups() do
        local want = on and hudWanted[group] == true
        if hudMine[group] and not p4.hud.hidden(group) then hudMine[group] = nil end
        if want and not hudMine[group] then
            -- Refused outside fields, floors and battles; the next frame retries.
            if p4.hud.hide(group, true) then
                hudMine[group] = true
                markPractice("P14")
                log("PRACTICE_HUD hidden " .. group)
            end
        elseif not want and hudMine[group] then
            p4.hud.hide(group, false)
            hudMine[group] = nil
            log("PRACTICE_HUD shown " .. group)
        end
    end
end

local function hudActive(): boolean
    return (enabled("hud") and next(hudWanted) ~= nil) or next(hudMine) ~= nil
end

-- ---------------------------------------------------------------- frame pump

local hudTick = 0
local function displayActive(): boolean
    return session.practice or enabled("frames") or (enabled("timer") and timer.running) or
        paceTool.pending ~= nil or (p4.pace.state()) ~= 1
end

local function onFrame(event: P4FrameEvent)
    counters.frame = event.frame
    for key, value in pendingSettings do
        local stored, why = p4.settings.set(key, value)
        log("PRACTICE_SETTING " .. key .. "=" .. tostring(value) .. (stored and "" or (" refused: " .. tostring(why))))
        if not value then
            if key == "timer" then timerStop() end
        end
        -- Encounter launches need the field's entry record, which the scene
        -- service observes only from its first query on.
        if value and (key == "encounter" or key == "warp") then p4.scene.state() end
        if value and key == "frames" then
            counters.baseUpdates = p4.clock.updates()
            counters.baseFrame = counters.frame
            counters.epoch = session.epoch
            markPractice("P16")
        end
    end
    table.clear(pendingSettings)
    if enabled("items") and not catalog.done then buildCatalog(192) end
    local op = table.remove(queue, 1)
    if op then
        local result = commit(op)
        results[op.kind] = result
        log("PRACTICE_RESULT " .. op.kind .. " " .. result)
    end
    runSceneOp()
    if statsOpen then statsFrame() end
    cameraFrame()
    hudFrame()
    paceFrame()
    if enabled("frames") and session.epoch ~= counters.epoch then
        -- A load starts a new timeline instead of a backwards jump.
        counters.epoch = session.epoch
        counters.baseUpdates = p4.clock.updates()
        counters.baseFrame = counters.frame
        counters.resets += 1
    end
    if timer.running and session.epoch ~= timer.epoch then
        timer.epoch = session.epoch
        timer.marks["LOAD"] = true
    end
    -- A tool still running across a load keeps the new session PRACTICE.
    if not session.practice then
        if timer.running then markPractice("P17") end
        if enabled("frames") then markPractice("P16") end
    end
    hudTick += 1
    if hudTick >= 6 then
        hudTick = 0
        refreshHud()
    end
    updatePump()
end

pump = p4.events.on("frame_start", onFrame)
pump:disable()
local pumping = true -- forced to sync below
updatePump = function()
    local want = next(pendingSettings) ~= nil or #queue > 0 or displayActive() or
        (enabled("items") and not catalog.done) or sceneOp ~= nil or watching ~= nil or statsOpen or
        cameraTool.lease ~= nil or hudActive()
    if want == pumping then return end
    pumping = want
    if want then pump:enable() else pump:disable() end
end
pumping = false

-- Event splits for the timer, enabled only while the timer runs.
local splitOn = { field = false, battle = false, calendar = false }
local splitSubs: { P4Subscription } = {}
local function splitEvent(name: string) if timer.running then timerSplit(name) end end
table.insert(splitSubs, p4.events.on("field_enter", function(e: P4FieldEnterEvent)
    if splitOn.field then splitEvent("FIELD " .. tostring(e.field) .. ":" .. tostring(e.room)) end
end))
table.insert(splitSubs, p4.events.on("battle_end", function(e: P4BattleEndEvent)
    if splitOn.battle then splitEvent("BATTLE " .. tostring(e.outcome)) end
end))
table.insert(splitSubs, p4.events.on("calendar_advance", function(e: P4CalendarEvent)
    if splitOn.calendar then splitEvent("DAY " .. tostring(e.date) .. "/" .. tostring(e.period)) end
end))
local function updateSplitSubs()
    local any = splitOn.field or splitOn.battle or splitOn.calendar
    for _, subscription in splitSubs do
        if any and timer.running then subscription:enable() else subscription:disable() end
    end
end
updateSplitSubs()

-- ---------------------------------------------------------------- page helpers

type RowList = { P4UIRow }
-- A typed parameter gives row literals their kind types (table.insert is generic).
local function addRow(rows: RowList, row: P4UIRow)
    table.insert(rows, row)
end
type PageDef = {
    key: string, title: string, listed: boolean?,
    rows: () -> RowList,
    adjust: ((key: string, direction: number) -> ())?,
    opened: (() -> ())?, closed: (() -> ())?,
}

local pages: { [string]: P4UIPage } = {}

-- The native list draws a value column for toggle/value/link rows only, so
-- an info row shows "LABEL: VALUE".
local function buildRows(def: PageDef): RowList
    local rows = def.rows()
    for _, row in rows do
        if row.kind == "info" and row.value then
            row.label = row.label .. ": " .. row.value
            row.value = nil
        end
    end
    return rows
end

local function definePage(def: PageDef): P4UIPage
    local cache: RowList = {}
    local page: P4UIPage
    local adjust = def.adjust
    page = p4.ui.page({
        key = def.key, title = def.title, listed = def.listed,
        count = function(): number
            cache = buildRows(def)
            return #cache
        end,
        row = function(index: number): P4UIRow
            return cache[index] or { key = "missing", label = "", kind = "info" }
        end,
        find = function(key: string): number?
            if #cache == 0 then cache = buildRows(def) end
            for index, row in cache do
                if row.key == key then return index end
            end
            return nil
        end,
        adjust = adjust and function(key: string, direction: number)
            adjust(key, direction)
            page:refresh()
        end or nil,
        opened = def.opened,
        closed = def.closed,
    })
    pages[def.key] = page
    return page
end

local function toggleRow(feature: Feature): P4UIRow
    return {
        key = "toggle." .. feature.key,
        label = feature.code .. " " .. feature.label,
        kind = "toggle",
        checked = enabled(feature.key),
        help = "Persistent toggle, OFF by default. Turning a feature off never makes an edited session clean.",
    }
end

local function linkRow(feature: Feature, target: string): P4UIRow
    local on = enabled(feature.key)
    return {
        key = "open." .. feature.key,
        label = "  OPEN " .. feature.label,
        kind = "link",
        target = target,
        disabled = not on,
        help = on and "Open the editor." or "Turn the feature ON first.",
    }
end

local function toggleFeature(key: string)
    local feature = FEATURES[key]
    if not feature then return end
    pendingSettings[key] = not enabled(key)
    updatePump()
end

-- A staged single-value edit: APPLY arms, CONFIRM commits; leaving the page
-- disarms. Nothing changes the game before CONFIRM.
type Editor = { armed: boolean }
local editors: { [string]: Editor } = {}
local function editor(key: string): Editor
    local e = editors[key]
    if not e then
        e = { armed = false }
        editors[key] = e
    end
    return e
end

-- warning: a short caption for the row and the full text for its help line.
local function applyRows(key: string, preview: string, warning: string?, detail: string?): RowList
    local e = editor(key)
    local rows: RowList = {
        { key = "preview", label = "CHANGE", kind = "info", value = preview },
    }
    if warning then
        addRow(rows, { key = "warning", label = "WARNING", kind = "info", value = warning, help = detail })
    end
    if e.armed then
        addRow(rows, { key = "confirm", label = "CONFIRM - PRACTICE ONLY", kind = "action",
            help = "PRACTICE ONLY - THIS SESSION WILL BE MARKED AND CANNOT BE SAVED. Press again to apply." })
        addRow(rows, { key = "cancel", label = "CANCEL", kind = "action" })
    else
        addRow(rows, { key = "apply", label = "APPLY", kind = "action",
            help = "Shows the change for confirmation. Nothing changes yet." })
    end
    addRow(rows, { key = "result", label = "RESULT", kind = "info", value = results[key] or "-" })
    return rows
end

-- Returns true when the action key was consumed by the apply flow.
local function applyAction(key: string, actionKey: string, direction: number, makeOp: () -> Op?): boolean
    local e = editor(key)
    if actionKey == "apply" and direction == 0 then
        e.armed = true
        return true
    elseif actionKey == "cancel" and direction == 0 then
        e.armed = false
        results[key] = "CANCELLED"
        return true
    elseif actionKey == "confirm" and direction == 0 then
        e.armed = false
        local op = makeOp()
        if op then enqueue(op) end
        return true
    end
    return false
end

local STEPS = { 1, 10, 100, 1000, 10000, 100000, 1000000 }
local function stepRow(key: string, index: number): P4UIRow
    return { key = key, label = "STEP", kind = "value", value = tostring(STEPS[index]),
        help = "Left/right choose how much the value rows change per press." }
end

-- ---------------------------------------------------------------- editors

local money = { staged = 0, step = 4, loaded = false }
definePage({
    key = "money", title = "P07 MONEY", listed = false,
    opened = function() money.staged = readMoney(); money.loaded = true; editor("money").armed = false end,
    closed = function() editor("money").armed = false end,
    rows = function(): RowList
        local current = readMoney()
        local rows: RowList = {
            { key = "current", label = "CURRENT YEN", kind = "info", value = tostring(current) },
            { key = "staged", label = "NEW YEN", kind = "value", value = tostring(money.staged),
                help = "0 to 9,999,999. Out-of-range values are refused, never clamped." },
            stepRow("step", money.step),
        }
        for _, row in applyRows("money", tostring(current) .. " -> " .. tostring(money.staged), nil) do table.insert(rows, row) end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "staged" then
            money.staged = clamp(money.staged + direction * STEPS[money.step], 0, 9999999)
            editor("money").armed = false
        elseif key == "step" then
            money.step = clamp(money.step + direction, 1, #STEPS)
        else
            applyAction("money", key, direction, function(): Op?
                return { kind = "money", a = money.staged, b = 0, label = "yen=" .. tostring(money.staged) }
            end)
        end
    end,
})

-- P05: maxima and the social rank thresholds are computed by game functions,
-- which synchronous page callbacks may not call, so the frame pump refreshes
-- this cache while the page is open.
local MEMBERS = { 1, 2, 3, 4, 6, 7, 8 }
local MEMBER_NAMES: { [number]: string } = {
    [1] = "PROTAGONIST", [2] = "YOSUKE", [3] = "CHIE", [4] = "YUKIKO", [6] = "KANJI", [7] = "NAOTO", [8] = "TEDDIE",
}
type StatsCache = { maxHp: { [number]: number }, maxSp: { [number]: number }, thresholds: { { number } }, ready: boolean }
local statsCache: StatsCache = { maxHp = {}, maxSp = {}, thresholds = {}, ready = false }
local statsPage = { index = 1, step = 1, hp = -1, sp = -1, social = {} :: { [number]: number } }

local function unitBase(unit: number): number
    return if unit == 1 then HERO_UNIT else PARTY_UNITS + unit * 0x88
end
-- The protagonist's level is his unit's; a party character's is the persona
-- record's in its slot (+0x54, level at +4), as datCalcGetLevel (0x00231E20)
-- reads it. 0 means no record yet: the game's maxima would assert, so no game
-- function is called for that character.
local function unitLevel(unit: number): number
    if p4.mem.read("u16", unitBase(unit) + 2) ~= unit then return 0 end
    if unit == 1 then return p4.mem.read("u8", HERO_UNIT + 6) end
    return p4.mem.read("u8", PARTY_UNITS + unit * 0x88 + 0x54)
end
local function unitHp(unit: number): number return p4.mem.read("u16", unitBase(unit) + 8) end
local function unitSp(unit: number): number return p4.mem.read("u16", unitBase(unit) + 10) end
local function socialPoints(category: number): number return p4.mem.read("i16", HERO_SOCIAL + category * 2) end

-- Returns true when a value changed, so the page is rebuilt only then (a
-- rebuild while the player navigates would move the cursor).
local function refreshStatsCache(): boolean
    local changed = not statsCache.ready
    for _, unit in MEMBERS do
        if unitLevel(unit) > 0 then
            local maxHp = p4.call(0x00104DC0, "Hh", unit) :: number
            local maxSp = p4.call(0x00104E30, "Hh", unit) :: number
            changed = changed or statsCache.maxHp[unit] ~= maxHp or statsCache.maxSp[unit] ~= maxSp
            statsCache.maxHp[unit], statsCache.maxSp[unit] = maxHp, maxSp
        end
    end
    if #statsCache.thresholds == 0 then
        for category = 0, 4 do
            local row: { number } = {}
            for rank = 1, 5 do row[rank] = p4.call(0x00246A50, "ihh", category, rank) :: number end
            statsCache.thresholds[category + 1] = row
        end
    end
    statsCache.ready = true
    return changed
end

-- datRankForPoints (0x00104F50): the first rank whose cumulative threshold exceeds the points.
local function socialRank(category: number, points: number): number
    local row = statsCache.thresholds[category + 1]
    if not row then return 0 end
    local sum = 0
    for rank = 1, 5 do
        sum += row[rank]
        if sum > points then return rank end
    end
    return 5
end
local function socialCap(category: number): number
    local row = statsCache.thresholds[category + 1]
    if not row then return 0 end
    local sum = 0
    for rank = 1, 5 do sum += row[rank] end
    return sum
end

local function partyText(unit: number): string
    if unit == 1 then return "PROTAGONIST" end
    for slot = 0, 2 do
        if p4.mem.read("i16", PARTY_IDS + slot * 2) == unit then return "PARTY SLOT " .. tostring(slot + 1) end
    end
    return "NOT IN THE ACTIVE PARTY"
end

local function statsReset()
    statsPage.hp, statsPage.sp = -1, -1
    table.clear(statsPage.social)
    editor("stats").armed = false
end

definePage({
    key = "stats", title = "P05 PARTY STATS", listed = false,
    opened = function()
        statsOpen = true
        statsCache.ready = false
        statsReset()
        updatePump()
    end,
    closed = function()
        statsOpen = false
        editor("stats").armed = false
    end,
    rows = function(): RowList
        local unit = MEMBERS[statsPage.index]
        local level = unitLevel(unit)
        local rows: RowList = {
            { key = "member", label = "MEMBER", kind = "value", value = MEMBER_NAMES[unit],
                help = "Records of characters not in the party can be edited; that never recruits them." },
            { key = "party", label = "PARTY", kind = "info", value = partyText(unit) },
            { key = "level", label = "LEVEL", kind = "info", value = level > 0 and tostring(level) or "NO RECORD YET" },
        }
        local maxHp, maxSp = statsCache.maxHp[unit], statsCache.maxSp[unit]
        if level == 0 or not maxHp or not maxSp then
            if level > 0 then addRow(rows, { key = "wait", label = "READING", kind = "info", value = "..." }) end
            return rows
        end
        local hp, sp = unitHp(unit), unitSp(unit)
        local newHp = if statsPage.hp >= 0 then statsPage.hp else hp
        local newSp = if statsPage.sp >= 0 then statsPage.sp else sp
        addRow(rows, { key = "hp", label = "HP", kind = "value", value = tostring(newHp) .. " / " .. tostring(maxHp),
            help = "1 to the maximum. Over-maximum values are not offered." })
        addRow(rows, { key = "sp", label = "SP", kind = "value", value = tostring(newSp) .. " / " .. tostring(maxSp) })
        addRow(rows, stepRow("step", statsPage.step))
        local changes: { string } = {}
        if newHp ~= hp then table.insert(changes, "HP " .. tostring(hp) .. ">" .. tostring(newHp)) end
        if newSp ~= sp then table.insert(changes, "SP " .. tostring(sp) .. ">" .. tostring(newSp)) end
        if unit == 1 then
            for category = 0, 4 do
                local points = socialPoints(category)
                local staged = statsPage.social[category] or points
                addRow(rows, { key = "social" .. tostring(category), label = "SOCIAL STAT " .. tostring(category + 1),
                    kind = "value", value = tostring(staged) .. " (RANK " .. tostring(socialRank(category, staged)) .. ")",
                    help = "Points; the rank shown is derived by the game's thresholds (cap " ..
                        tostring(socialCap(category)) .. ")." })
                if staged ~= points then
                    table.insert(changes, "S" .. tostring(category + 1) .. " " .. tostring(points) .. ">" .. tostring(staged))
                end
            end
        end
        local over = hp > maxHp or sp > maxSp
        addRow(rows, { key = "normalize", label = over and "NORMALIZE HP/SP TO MAXIMUM" or "HP/SP WITHIN MAXIMUM",
            kind = "action", disabled = not over,
            help = "Runs the game's clamp (0x00105780) once. Nothing is clamped otherwise." })
        for _, row in applyRows("stats", #changes > 0 and table.concat(changes, " ") or "NO CHANGE", nil) do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        local unit = MEMBERS[statsPage.index]
        local step = STEPS[statsPage.step]
        if key == "member" then
            statsPage.index = (statsPage.index - 1 + direction) % #MEMBERS + 1
            statsReset()
        elseif key == "step" then
            statsPage.step = clamp(statsPage.step + direction, 1, 3)
        elseif key == "hp" then
            local base = if statsPage.hp >= 0 then statsPage.hp else unitHp(unit)
            statsPage.hp = clamp(base + direction * step, 1, statsCache.maxHp[unit] or 1)
            editor("stats").armed = false
        elseif key == "sp" then
            local base = if statsPage.sp >= 0 then statsPage.sp else unitSp(unit)
            statsPage.sp = clamp(base + direction * step, 0, statsCache.maxSp[unit] or 0)
            editor("stats").armed = false
        elseif string.sub(key, 1, 6) == "social" then
            local category = tonumber(string.sub(key, 7)) or 0
            local base = statsPage.social[category] or socialPoints(category)
            statsPage.social[category] = clamp(base + direction * step, 0, socialCap(category))
            editor("stats").armed = false
        elseif key == "normalize" and direction == 0 then
            enqueue({ kind = "stats", a = unit, b = 0, field = "normalize", label = "unit=" .. tostring(unit) .. " normalize" })
        else
            applyAction("stats", key, direction, function(): Op?
                local label = "unit=" .. tostring(unit)
                if statsPage.hp >= 0 and statsPage.hp ~= unitHp(unit) then
                    enqueue({ kind = "stats", a = unit, b = statsPage.hp, field = "hp", label = label .. " hp" })
                end
                if statsPage.sp >= 0 and statsPage.sp ~= unitSp(unit) then
                    enqueue({ kind = "stats", a = unit, b = statsPage.sp, field = "sp", label = label .. " sp" })
                end
                for category, points in statsPage.social do
                    if unit == 1 and points ~= socialPoints(category) then
                        enqueue({ kind = "stats", a = 1, b = points, field = "social" .. tostring(category),
                            label = label .. " social" .. tostring(category) })
                    end
                end
                statsReset()
                return nil
            end)
        end
    end,
})
statsFrame = function()
    if statsCache.ready and counters.frame % 30 ~= 0 then return end
    local page = pages.stats
    if refreshStatsCache() and page then page:refresh() end
end

local function levelExp(level: number): number
    if level <= 1 then return 0 end
    return p4.mem.read("u32", EXP_TABLE + (level - 2) * 4)
end
local function baseHp(level: number): number return p4.mem.read("u16", LEVEL_TABLE + level * 44 + 4) end
local function baseSp(level: number): number return p4.mem.read("u16", LEVEL_TABLE + level * 44 + 6) end

local levels = { staged = 0, normalize = false }
definePage({
    key = "levels", title = "P06 LEVEL / EXP", listed = false,
    opened = function()
        levels.staged = p4.mem.read("u8", HERO_UNIT + 6)
        levels.normalize = false
        editor("levels").armed = false
    end,
    closed = function() editor("levels").armed = false end,
    rows = function(): RowList
        local level, exp = p4.mem.read("u8", HERO_UNIT + 6), p4.mem.read("u32", HERO_EXP)
        local target = levels.staged
        local hp, sp = unitHp(1), unitSp(1)
        local rows: RowList = {
            { key = "member", label = "MEMBER", kind = "info", value = "PROTAGONIST",
                help = "Party characters' levels are their Personas' and need a growth rebuild; not offered." },
            { key = "level", label = "NEW LEVEL", kind = "value", value = tostring(target) .. " (NOW " .. tostring(level) .. ")",
                help = "1 to 99, up or down. EXP becomes that level's minimum." },
            { key = "exp", label = "EXP", kind = "info", value = tostring(exp) .. " -> " .. tostring(levelExp(target)) },
            { key = "hp", label = "BASE HP", kind = "info", value = tostring(baseHp(level)) .. " -> " .. tostring(baseHp(target)) },
            { key = "sp", label = "BASE SP", kind = "info", value = tostring(baseSp(level)) .. " -> " .. tostring(baseSp(target)) },
            { key = "normalize", label = "NORMALIZE HP/SP AFTER", kind = "toggle", checked = levels.normalize,
                help = "Also run the game's HP/SP clamp once (current HP " .. tostring(hp) .. ", SP " .. tostring(sp) ..
                    "). Off: a lower level can leave HP/SP above the new maximum." },
        }
        for _, row in applyRows("levels", "LV" .. tostring(level) .. ">" .. tostring(target) .. " EXP " ..
            tostring(exp) .. ">" .. tostring(levelExp(target)), nil) do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "level" then
            levels.staged = clamp(levels.staged + direction, 1, 99)
            editor("levels").armed = false
        elseif key == "normalize" then
            levels.normalize = not levels.normalize
            editor("levels").armed = false
        else
            applyAction("levels", key, direction, function(): Op?
                return { kind = "levels", a = levels.staged, b = levelExp(levels.staged),
                    field = if levels.normalize then "normalize" else nil,
                    label = "level=" .. tostring(levels.staged) .. " exp=" .. tostring(levelExp(levels.staged)) }
            end)
        end
    end,
})

-- The disc's day table uses codes 0-2; FldEnvResolveState (0x00154720) gives
-- each its own field environment set.
local WEATHER_NAMES: { [number]: string } = { [0] = "CLEAR", [1] = "RAIN", [2] = "CLOUDY" }
local function weatherText(code: number?): string
    if code == nil then return "-" end
    return (WEATHER_NAMES[code] or "CODE") .. " (" .. tostring(code) .. ")"
end
local weather = { code = 0 }
definePage({
    key = "weather", title = "P04 WEATHER", listed = false,
    opened = function()
        weather.code = weatherOverride() or weatherOfDay()
        if not WEATHER_NAMES[weather.code] then weather.code = 0 end
        editor("weather").armed = false
    end,
    closed = function() editor("weather").armed = false end,
    rows = function(): RowList
        local override = weatherOverride()
        local rows: RowList = {
            { key = "calendar", label = "CALENDAR", kind = "info", value = weatherText(weatherOfDay()),
                help = "The disc's weather for today's time of day." },
            { key = "override", label = "OVERRIDE", kind = "info", value = override ~= nil and weatherText(override) or "NONE" },
            { key = "code", label = "NEW WEATHER", kind = "value", value = weatherText(weather.code),
                help = "Used by the calendar and by every field loaded until the next date or time-slot change, then " ..
                    "the disc's weather returns. The current field keeps its look until you leave it (or use P01)." },
            { key = "release", label = "RELEASE OVERRIDE", kind = "action", disabled = override == nil,
                help = "Restore the weather saved by the override now." },
        }
        for _, row in applyRows("weather", weatherText(override or weatherOfDay()) .. " -> " .. weatherText(weather.code),
            "UNTIL NEXT DATE/SLOT", "A one-slot override; the calendar's own table is not changed.") do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "code" then
            weather.code = (weather.code + direction) % 3
            editor("weather").armed = false
        elseif key == "release" and direction == 0 then
            enqueue({ kind = "weather", a = 0, b = 0, field = "release", label = "release" })
        else
            applyAction("weather", key, direction, function(): Op?
                return { kind = "weather", a = weather.code, b = weather.code, label = "code=" .. tostring(weather.code) }
            end)
        end
    end,
})

local STAT_NAMES = { "ST", "MA", "EN", "AG", "LU" }
type PersonaAction = { field: string, a: number, text: string }
local personas = { slot = 0, addId = 0, stats = {} :: { [number]: number }, action = nil :: PersonaAction? }

local function nextPersona(id: number, direction: number): number
    local candidate = id
    for _ = 1, PERSONA_IDS do
        candidate = (candidate + direction) % PERSONA_IDS
        if personaRow(candidate) and not rosterFind(candidate) then return candidate end
    end
    return 0
end
local function personasReset()
    table.clear(personas.stats)
    personas.action = nil
    editor("personas").armed = false
end

definePage({
    key = "personas", title = "P09 PERSONAS", listed = false,
    opened = function()
        personas.slot = math.max(p4.mem.read("i16", ROSTER_LEAD), 0)
        if personas.addId == 0 then personas.addId = nextPersona(0, 1) end
        personasReset()
    end,
    closed = function() editor("personas").armed = false end,
    rows = function(): RowList
        local capacity, lead = rosterCapacity(), p4.mem.read("i16", ROSTER_LEAD)
        local slot = personas.slot
        local active = rosterActive(slot)
        local record = ROSTER + slot * 0x30
        local rows: RowList = {
            { key = "roster", label = "ROSTER", kind = "info",
                value = tostring(rosterCount()) .. " / " .. tostring(capacity) .. "  LEAD SLOT " .. tostring(lead + 1) },
            { key = "slot", label = "SLOT", kind = "value", value = tostring(slot + 1) .. ": " .. (if active then
                "ID " .. tostring(rosterId(slot)) .. " LV " .. tostring(p4.mem.read("u8", record + 4)) else "EMPTY"),
                help = "Persona names are not shown; IDs follow the game's Persona table." },
        }
        local changes: { string } = {}
        if active then
            for index = 0, 4 do
                local value = p4.mem.read("u8", record + 0x1C + index)
                local staged = personas.stats[index] or value
                addRow(rows, { key = "stat" .. tostring(index), label = STAT_NAMES[index + 1], kind = "value",
                    value = tostring(staged), help = "0 to 99 through the game's stat adder." })
                if staged ~= value then
                    table.insert(changes, STAT_NAMES[index + 1] .. " " .. tostring(value) .. ">" .. tostring(staged))
                end
            end
            local skills: { string } = {}
            for index = 0, 7 do
                local skill = p4.mem.read("u16", record + 0xC + index * 2)
                if skill ~= 0 then table.insert(skills, tostring(skill)) end
            end
            addRow(rows, { key = "skills", label = "SKILL IDS", kind = "info",
                value = #skills > 0 and table.concat(skills, " ") or "NONE" })
            addRow(rows, { key = "lead", label = "MAKE LEAD", kind = "action", disabled = slot == lead,
                help = "Stages this slot as the equipped Persona." })
            addRow(rows, { key = "remove", label = "REMOVE FROM ROSTER", kind = "action",
                disabled = rosterCount() <= 1, help = "Stages removal; later slots move up. The last Persona stays." })
        end
        local row = personaRow(personas.addId)
        local carried = rosterFind(personas.addId) ~= nil
        addRow(rows, { key = "addId", label = "ADD PERSONA ID", kind = "value", value = tostring(personas.addId) ..
            (if carried then " (CARRIED)" elseif row then " (ARCANA " .. tostring(p4.mem.read("u8", row + 2)) ..
                ", LV " .. tostring(p4.mem.read("u8", row + 3)) .. ")" else " (NONE LEFT)"),
            help = "Persona ids that fusion can produce and that are not carried. A full roster refuses; nothing is evicted." })
        addRow(rows, { key = "add", label = "ADD TO ROSTER", kind = "action", disabled = row == nil or carried })
        local action = personas.action
        if action then table.insert(changes, 1, action.text) end
        for _, applyRow in applyRows("personas", #changes > 0 and table.concat(changes, " ") or "NO CHANGE", nil) do
            table.insert(rows, applyRow)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        local slot = personas.slot
        if key == "slot" then
            personas.slot = (slot + direction) % rosterCapacity()
            personasReset()
        elseif string.sub(key, 1, 4) == "stat" then
            local index = tonumber(string.sub(key, 5)) or 0
            local base = personas.stats[index] or p4.mem.read("u8", ROSTER + slot * 0x30 + 0x1C + index)
            personas.stats[index] = clamp(base + direction, 0, 99)
            editor("personas").armed = false
        elseif key == "addId" then
            if direction ~= 0 then personas.addId = nextPersona(personas.addId, direction) end
        elseif key == "lead" and direction == 0 then
            personas.action = { field = "lead", a = slot, text = "LEAD>SLOT " .. tostring(slot + 1) }
            editor("personas").armed = false
        elseif key == "remove" and direction == 0 then
            personas.action = { field = "remove", a = slot, text = "REMOVE SLOT " .. tostring(slot + 1) }
            editor("personas").armed = false
        elseif key == "add" and direction == 0 then
            personas.action = { field = "add", a = personas.addId, text = "ADD ID " .. tostring(personas.addId) }
            editor("personas").armed = false
        else
            applyAction("personas", key, direction, function(): Op?
                -- Stat edits go first: a staged removal shifts the slots after it.
                for index, value in personas.stats do
                    enqueue({ kind = "personas", a = slot, b = value, field = "stat" .. tostring(index),
                        label = "slot=" .. tostring(slot) .. " stat" .. tostring(index) })
                end
                local action = personas.action
                if action then
                    enqueue({ kind = "personas", a = action.a, b = 0, field = action.field,
                        label = action.field .. "=" .. tostring(action.a) })
                end
                personasReset()
                return nil
            end)
        end
    end,
})

local flags = { id = 0, value = 1, step = 1 }
definePage({
    key = "flags", title = "P11 STORY FLAGS", listed = false,
    closed = function() editor("flags").armed = false end,
    rows = function(): RowList
        local current = readFlag(flags.id)
        local rows: RowList = {
            { key = "id", label = "FLAG", kind = "value", value = idText(flags.id, 4),
                help = "0 to 5631 (0x15FF). Purposes of most flags are unknown; changes can break story scripts." },
            stepRow("step", flags.step),
            { key = "current", label = "CURRENT", kind = "info", value = current and "ON" or "OFF" },
            { key = "value", label = "NEW VALUE", kind = "toggle", checked = flags.value == 1 },
        }
        local warning, detail = "STORY MAY BREAK", "Advanced: flags drive story scripts; a change may be irreversible."
        if flags.id == 0x1576 then
            warning, detail = "DIAGNOSTIC FLAG", "Setting flag 0x1576 also runs the game's own diagnostic hook."
        end
        for _, row in applyRows("flags", (current and "ON" or "OFF") .. " -> " .. (flags.value == 1 and "ON" or "OFF"), warning, detail) do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "id" then
            flags.id = clamp(flags.id + direction * STEPS[flags.step], 0, 0x15FF)
            editor("flags").armed = false
        elseif key == "step" then
            flags.step = clamp(flags.step + direction, 1, 4)
        elseif key == "value" then
            flags.value = 1 - flags.value
            editor("flags").armed = false
        else
            applyAction("flags", key, direction, function(): Op?
                return { kind = "flags", a = flags.id, b = flags.value, label = "flag=" .. hex(flags.id, 4) .. " value=" .. tostring(flags.value) }
            end)
        end
    end,
})

local items = { id = 1, count = 1, keys = false }
definePage({
    key = "items", title = "P08 ITEMS", listed = false,
    closed = function() editor("items").armed = false end,
    rows = function(): RowList
        if not catalog.done then
            return { { key = "loading", label = "READING THE ITEM TABLE", kind = "info",
                value = tostring(catalog.next) .. " / " .. tostring(ITEM_RETAIL) } }
        end
        local current = readItemCount(items.id)
        local kind = catalog.kinds[items.id] or -1
        local rows: RowList = {
            { key = "id", label = "ITEM", kind = "value", value = hex(items.id, 4) .. " " .. itemLabel(items.id),
                help = "Left/right step through the game's real item rows; blank rows are skipped." },
            { key = "jump", label = "JUMP 100", kind = "value", value = idText(items.id, 4),
                help = "Left/right jump 100 IDs, then to the nearest real item." },
            { key = "current", label = "CURRENT COUNT", kind = "info", value = tostring(current) },
            { key = "count", label = "NEW COUNT", kind = "value", value = tostring(items.count), help = "0 to 99." },
            { key = "kind", label = "KIND", kind = "info", value = tostring(kind) },
            { key = "keys", label = "ALLOW KEY ITEMS (ADVANCED)", kind = "toggle", checked = items.keys,
                help = "Key items carry a story flag; changing their count sets or clears it." },
        }
        local warning: string?, detail: string? = nil, nil
        if kind == 4 then
            warning = "KEY ITEM FLAG " .. hex(items.id - 0x200, 4)
            detail = "A key item's count also sets or clears its story flag " .. hex(items.id - 0x200, 4) .. "."
        end
        for _, row in applyRows("items", tostring(current) .. " -> " .. tostring(items.count), warning, detail) do table.insert(rows, row) end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "id" then
            items.id = stepItem(items.id, direction)
            editor("items").armed = false
        elseif key == "jump" then
            if direction ~= 0 then
                local target = clamp(items.id + direction * 100, 0, ITEM_RETAIL - 1)
                items.id = stepItem(target - direction, direction)
            end
            editor("items").armed = false
        elseif key == "keys" then
            items.keys = not items.keys
            editor("items").armed = false
        elseif key == "count" then
            items.count = clamp(items.count + direction, 0, 99)
            editor("items").armed = false
        else
            applyAction("items", key, direction, function(): Op?
                if not catalog.names[items.id] then
                    results.items = "REFUSED: NOT A REAL ITEM ROW"
                    return nil
                end
                if catalog.kinds[items.id] == 4 and not items.keys then
                    results.items = "REFUSED: KEY ITEM - TURN ON ALLOW KEY ITEMS"
                    return nil
                end
                return { kind = "items", a = items.id, b = items.count, label = "item=" .. hex(items.id, 4) .. " count=" .. tostring(items.count) }
            end)
        end
    end,
})

local date = { staged = 0, loaded = false }
definePage({
    key = "date", title = "P02 SET DATE", listed = false,
    opened = function() date.staged = readDate(); editor("date").armed = false end,
    closed = function() editor("date").armed = false end,
    rows = function(): RowList
        local current = readDate()
        local rows: RowList = {
            { key = "current", label = "CURRENT", kind = "info", value = dateText(current) },
            { key = "staged", label = "NEW DATE", kind = "value", value = dateText(date.staged),
                help = "Left/right one day; the calendar wraps within April to March." },
        }
        for _, row in applyRows("date", tostring(current) .. " -> " .. tostring(date.staged),
            "NO EVENT REPLAY", "Missed story events are not replayed. The field updates its environment on its next load.") do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "staged" then
            date.staged = (date.staged + direction) % 365
            editor("date").armed = false
        else
            applyAction("date", key, direction, function(): Op?
                return { kind = "date", a = date.staged, b = 0, label = "day=" .. tostring(date.staged) }
            end)
        end
    end,
})

local slot = { staged = 0 }
definePage({
    key = "time", title = "P03 SET TIME SLOT", listed = false,
    opened = function() slot.staged = readTimeSlot(); editor("time").armed = false end,
    closed = function() editor("time").armed = false end,
    rows = function(): RowList
        local current = readTimeSlot()
        local rows: RowList = {
            { key = "current", label = "CURRENT", kind = "info", value = slotText(current) },
            { key = "staged", label = "NEW SLOT", kind = "value", value = slotText(slot.staged),
                help = "The six values the calendar cycles; labels are written by this mod." },
        }
        for _, row in applyRows("time", tostring(current) .. " -> " .. tostring(slot.staged),
            "NO CALENDAR ADVANCE", "The calendar does not advance. Lighting changes on the next field load.") do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "staged" then
            slot.staged = (slot.staged + direction) % 6
            editor("time").armed = false
        else
            applyAction("time", key, direction, function(): Op?
                return { kind = "time", a = slot.staged, b = 0, label = "slot=" .. tostring(slot.staged) }
            end)
        end
    end,
})

local links = { index = 1, rank = 1 }
definePage({
    key = "links", title = "P10 SOCIAL LINK RANKS", listed = false,
    closed = function() editor("links").armed = false end,
    rows = function(): RowList
        local list = readLinks()
        if #list == 0 then
            return { { key = "none", label = "NO SOCIAL LINK RECORDS", kind = "info",
                help = "Only links the save has started can be edited." } }
        end
        links.index = clamp(links.index, 1, #list)
        local link = list[links.index]
        local rows: RowList = {
            { key = "link", label = "LINK", kind = "value", value = "ID " .. tostring(link.id) .. " (" .. tostring(links.index) .. "/" .. tostring(#list) .. ")",
                help = "Existing records only; creating or removing links is not offered." },
            { key = "current", label = "CURRENT RANK", kind = "info", value = tostring(link.rank) },
            { key = "rank", label = "NEW RANK", kind = "value", value = tostring(links.rank), help = "1 to 10." },
        }
        for _, row in applyRows("links", tostring(link.rank) .. " -> " .. tostring(links.rank),
            "NO EVENTS OR REWARDS", "No Social Link events or rewards are played; points are unchanged.") do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        local list = readLinks()
        if key == "link" then
            if #list > 0 then links.index = (links.index - 1 + direction) % #list + 1 end
            editor("links").armed = false
        elseif key == "rank" then
            links.rank = clamp(links.rank + direction, 1, 10)
            editor("links").armed = false
        else
            applyAction("links", key, direction, function(): Op?
                local link = list[links.index]
                if not link then return nil end
                return { kind = "links", a = link.id, b = links.rank, label = "link=" .. tostring(link.id) .. " rank=" .. tostring(links.rank) }
            end)
        end
    end,
})

definePage({
    key = "frames", title = "P16 FRAME COUNTER", listed = false,
    rows = function(): RowList
        return {
            { key = "updates", label = "COMPLETED UPDATES", kind = "info", value = tostring(p4.clock.updates() - counters.baseUpdates) },
            { key = "retail", label = "RETAIL COUNTER", kind = "info", value = tostring(p4.clock.retail()) },
            { key = "frames", label = "PRESENTED FRAMES", kind = "info", value = tostring(counters.frame - counters.baseFrame) },
            { key = "epochs", label = "TIMELINES", kind = "info", value = tostring(counters.resets + 1),
                help = "A load or reset starts a new timeline; counts never run backwards." },
            { key = "reset", label = "RESET SESSION COUNTS", kind = "action" },
        }
    end,
    adjust = function(key: string, direction: number)
        if key == "reset" and direction == 0 then
            counters.baseUpdates = p4.clock.updates()
            counters.baseFrame = counters.frame
            counters.resets += 1
        end
    end,
})

definePage({
    key = "timer", title = "P17 PRACTICE TIMER", listed = false,
    rows = function(): RowList
        local wall, updates = timerValues()
        local rows: RowList = {
            { key = "wall", label = "WALL TIME", kind = "info", value = clockText(wall),
                help = "Host monotonic time, including loading and menus. Not an RTA or loadless time." },
            { key = "updates", label = "GAME UPDATES", kind = "info", value = tostring(updates) },
            { key = "state", label = "STATE", kind = "info", value = timer.running and "RUNNING" or "STOPPED" },
            { key = "start", label = timer.running and "STOP" or "START", kind = "action" },
            { key = "split", label = "SPLIT", kind = "action", disabled = not timer.running },
            { key = "reset", label = "RESET", kind = "action" },
            { key = "auto.field", label = "SPLIT ON FIELD ENTRY", kind = "toggle", checked = splitOn.field },
            { key = "auto.battle", label = "SPLIT ON BATTLE END", kind = "toggle", checked = splitOn.battle },
            { key = "auto.calendar", label = "SPLIT ON CALENDAR ADVANCE", kind = "toggle", checked = splitOn.calendar },
        }
        if next(timer.marks) then
            local marks = {}
            for mark in timer.marks do table.insert(marks, mark) end
            table.sort(marks)
            addRow(rows, { key = "marks", label = "MARKERS", kind = "info", value = table.concat(marks, " ") })
        end
        for index, text in timer.splits do
            addRow(rows, { key = "split." .. tostring(index), label = "SPLIT " .. tostring(index), kind = "info", value = text })
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "start" and direction == 0 then
            if timer.running then timerStop() else timerStart() end
        elseif key == "split" and direction == 0 then
            timerSplit("MANUAL")
        elseif key == "reset" and direction == 0 then
            timerReset()
        elseif key == "auto.field" then
            splitOn.field = not splitOn.field
        elseif key == "auto.battle" then
            splitOn.battle = not splitOn.battle
        elseif key == "auto.calendar" then
            splitOn.calendar = not splitOn.calendar
        end
        updateSplitSubs()
        updatePump()
    end,
})

local OUTCOMES = { [0] = "NONE", [1] = "VICTORY", [2] = "DEFEAT", [3] = "NO PARTY" }
local function requestText(): string
    local state, detail, outcome = p4.scene.status()
    local text = string.upper(state)
    if detail ~= "" then text ..= " " .. string.upper(detail) end
    if outcome then text ..= " " .. (OUTCOMES[outcome] or tostring(outcome)) end
    return text
end

local warp = { field = 0, room = 0 }
definePage({
    key = "warp", title = "P01 WARP", listed = false,
    opened = function()
        local state = p4.scene.state()
        if warp.field == 0 then warp.field, warp.room = math.max(state.field, 1), state.room end
        editor("warp").armed = false
    end,
    closed = function() editor("warp").armed = false end,
    rows = function(): RowList
        local state = p4.scene.state()
        local available = p4.scene.available(warp.field, warp.room)
        local rows: RowList = {
            { key = "current", label = "CURRENT", kind = "info",
                value = string.format("%03d:%03d", state.field, state.room) },
            { key = "field", label = "FIELD", kind = "value", value = string.format("%03d", warp.field),
                help = "Field (major) ID; the disc must have its resource." },
            { key = "room", label = "ROOM", kind = "value", value = string.format("%03d", warp.room) },
            { key = "disc", label = "RESOURCES", kind = "info", value = available and "ON DISC" or "NOT ON DISC" },
            { key = "status", label = "LAST REQUEST", kind = "info", value = requestText() },
        }
        for _, row in applyRows("warp", string.format("%03d:%03d -> %03d:%03d", state.field, state.room, warp.field, warp.room),
            "NO STORY PROGRESS", "Warps go through the game's own field load at the default entry. Flags, events and " ..
            "unlocks are unchanged, so a story-locked field may misbehave. Runs when the Config closes.") do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "field" then
            warp.field = clamp(warp.field + direction, 1, 999)
            editor("warp").armed = false
        elseif key == "room" then
            warp.room = clamp(warp.room + direction, 0, 999)
            editor("warp").armed = false
        else
            applyAction("warp", key, direction, function(): Op?
                local state = p4.scene.state()
                if not p4.scene.available(warp.field, warp.room) then
                    results.warp = "REFUSED: NOT ON DISC"
                    return nil
                end
                stageScene({ kind = "warp", field = warp.field, room = warp.room, encounter = -1,
                    fromField = state.field, fromRoom = state.room, settle = 0 })
                return nil
            end)
        end
    end,
})

local encounter = { id = 0 }
local function stepEncounter(id: number, direction: number): number
    local count = p4.encounter.count()
    if count == 0 then return 0 end
    local candidate = id
    for _ = 1, 64 do
        candidate = (candidate + direction) % count
        if p4.encounter.info(candidate) then return candidate end
    end
    return id
end
definePage({
    key = "encounter", title = "P12 START ENCOUNTER", listed = false,
    closed = function() editor("encounter").armed = false end,
    rows = function(): RowList
        local info = p4.encounter.info(encounter.id)
        local enemies = "NOT A VALID ENCOUNTER"
        if info then
            local ids = {}
            for _, enemy in info.enemies do
                if enemy ~= 0 then table.insert(ids, tostring(enemy)) end
            end
            enemies = table.concat(ids, " ")
        end
        local rows: RowList = {
            { key = "id", label = "ENCOUNTER", kind = "value", value = idText(encounter.id, 4),
                help = "Left/right step through the loaded encounter table; invalid records are skipped." },
            { key = "enemies", label = "ENEMY IDS", kind = "info", value = enemies },
            { key = "formation", label = "FORMATION", kind = "info",
                value = info and (tostring(info.formation) .. ":" .. tostring(info.variant)) or "-" },
            { key = "status", label = "LAST REQUEST", kind = "info", value = requestText() },
        }
        for _, row in applyRows("encounter", "BATTLE " .. tostring(encounter.id),
            "PRACTICE BATTLE", "Starts the encounter with the current party through the game's battle path and returns " ..
            "to this field afterwards. A defeat revives the party at 1 HP instead of a game over. Story " ..
            "prerequisites are not set up. Runs when the Config closes.") do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "id" then
            if direction ~= 0 then encounter.id = stepEncounter(encounter.id, direction) end
            editor("encounter").armed = false
        else
            applyAction("encounter", key, direction, function(): Op?
                local state = p4.scene.state()
                if not p4.encounter.info(encounter.id) then
                    results.encounter = "REFUSED: NOT A VALID ENCOUNTER"
                    return nil
                end
                stageScene({ kind = "encounter", field = state.field, room = state.room, encounter = encounter.id,
                    fromField = state.field, fromRoom = state.room, settle = 0 })
                return nil
            end)
        end
    end,
})

definePage({
    key = "camera", title = "P13 FREE CAMERA", listed = false,
    rows = function(): RowList
        return {
            { key = "enter", label = cameraTool.lease and "FREE CAMERA ACTIVE" or "ENTER FREE CAMERA", kind = "action",
                disabled = cameraTool.lease ~= nil,
                help = "Starts when the Config closes. Left stick move, right stick look, L1/R1 down/up, L2/R2 " ..
                    "field of view, Circle exits. The game ignores the pad meanwhile; only the view changes." },
            { key = "state", label = "STATE", kind = "info", value = cameraTool.state },
            { key = "move", label = "MOVE SPEED", kind = "value", value = tostring(cameraTool.move) },
            { key = "turn", label = "TURN SPEED", kind = "value", value = tostring(cameraTool.turn) },
        }
    end,
    adjust = function(key: string, direction: number)
        if key == "enter" and direction == 0 and not cameraTool.lease then
            local state = p4.scene.state()
            cameraTool.state = "WAITING FOR CONFIG TO CLOSE"
            stageScene({ kind = "camera", field = state.field, room = state.room, encounter = -1,
                fromField = state.field, fromRoom = state.room, settle = 0 })
        elseif key == "move" then
            cameraTool.move = clamp(cameraTool.move + direction, 1, 10)
        elseif key == "turn" then
            cameraTool.turn = clamp(cameraTool.turn + direction, 1, 10)
        end
    end,
})

local HUD_LABELS: { [string]: string } = {
    field_banner = "LOCATION BANNER", field_map = "FIELD MAP", calendar = "DATE AND WEATHER",
    party = "PARTY PANEL", battle_commands = "BATTLE COMMANDS",
}
definePage({
    key = "hud", title = "P14 HIDE HUD", listed = false,
    rows = function(): RowList
        local rows: RowList = {}
        for _, group in p4.hud.groups() do
            addRow(rows, { key = "group." .. group, label = HUD_LABELS[group] or string.upper(group), kind = "toggle",
                checked = hudWanted[group] == true,
                help = "Hidden in every field, floor and battle while P14 is on. Only drawing stops; menus, " ..
                    "prompts, fades and the PRACTICE status stay visible." })
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if string.sub(key, 1, 6) == "group." then
            local group = string.sub(key, 7)
            hudWanted[group] = if hudWanted[group] then nil else true
            updatePump()
        end
    end,
})

local PACE_RATES = { 0.25, 0.5, 1, 2, 4 }
definePage({
    key = "pace", title = "P15 PRACTICE SPEED", listed = false,
    rows = function(): RowList
        local requested, effective, held = p4.pace.state()
        return {
            { key = "speed", label = "SPEED", kind = "value", value = paceText(paceTool.staged),
                help = "0.25x to 4x game speed in real-time play. Every frame runs the same updates; only waiting " ..
                    "between frames changes. Menus, scene changes and movies run at 1x." },
            { key = "set", label = "SET SPEED", kind = "action", disabled = paceTool.staged == requested,
                help = "Takes effect when the Config closes and gameplay is ready. L3 restores 1x." },
            { key = "state", label = "NOW", kind = "info", value = paceText(effective) ..
                (if requested ~= effective then " (ASKED " .. paceText(requested) .. ")" else "") },
            { key = "held", label = "HOLD", kind = "info", value = if held then string.upper(held) else "-" },
            { key = "actual", label = "ACTUAL", kind = "info", value = string.format("%.2fX", paceTool.actual),
                help = "Game frames per second over the last second, against normal speed. A slow host shows less." },
            { key = "audio", label = "AUDIO", kind = "info", value = if effective == 1 then "NORMAL" else "MUTED, FOLLOWS FRAMES" },
            { key = "result", label = "RESULT", kind = "info", value = paceTool.result },
        }
    end,
    adjust = function(key: string, direction: number)
        if key == "speed" then
            local index = table.find(PACE_RATES, paceTool.staged) or 3
            paceTool.staged = PACE_RATES[clamp(index + direction, 1, #PACE_RATES)]
        elseif key == "set" and direction == 0 then
            paceTool.pending = paceTool.staged
            paceTool.result = "WAITING"
            updatePump()
        end
    end,
})

-- P18: a passive skill granted to a party member through p4.passive
-- (capability passive.grant@1). The game's passive checks (battle formulas,
-- the AI, Endure, max HP/SP, costs, the field) see it as one in a slot; the
-- skill list and the save never change. P18 makes and takes back only its own
-- grants, which end at a load or New Game (the save load callback above);
-- other mods' grants are shown and left alone.
local PASSIVES: { P4Ref } = p4.data.find("skill", { kind = "passive" })
local passiveRefs: { [number]: P4Ref } = {}
for _, ref in PASSIVES do passiveRefs[ref.id] = ref end
local passivePage = { member = 1, index = 1 }
local function passiveName(ref: P4Ref): string
    return if ref.name ~= nil then upper(ref.name) else idText(ref.id, 3)
end
-- The member's party grants: P18's own, and other mods'.
local function memberPassives(member: number): (string, string)
    local own: { string }, others: { string } = {}, {}
    for _, grant in p4.passive.list() do
        if grant.kind == "party" and grant.id == member then
            local ref = passiveRefs[grant.skill]
            local name = if ref then passiveName(ref) else idText(grant.skill, 3)
            if grant.owner == p4.id then table.insert(own, name)
            elseif not table.find(others, name) then table.insert(others, name) end
        end
    end
    return if #own > 0 then table.concat(own, ", ") else "NONE",
        if #others > 0 then table.concat(others, ", ") else "NONE"
end
definePage({
    key = "passives", title = "P18 PASSIVE SKILLS", listed = false,
    opened = function() editor("passives").armed = false end,
    closed = function() editor("passives").armed = false end,
    rows = function(): RowList
        local member = MEMBERS[passivePage.member]
        local rows: RowList = {
            { key = "member", label = "MEMBER", kind = "value", value = MEMBER_NAMES[member],
                help = "The member has the skill with every Persona, without using a skill slot." },
        }
        local ref = PASSIVES[passivePage.index]
        if not ref then
            addRow(rows, { key = "none", label = "PASSIVE SKILLS", kind = "info", value = "NONE IN THE LOADED TABLES" })
            return rows
        end
        addRow(rows, { key = "skill", label = "PASSIVE", kind = "value", value = passiveName(ref),
            help = "Every passive skill of the loaded tables. Navigator skills are not passives." })
        local own, others = memberPassives(member)
        addRow(rows, { key = "held", label = "GRANTED HERE", kind = "info", value = own })
        addRow(rows, { key = "others", label = "BY OTHER MODS", kind = "info", value = others,
            help = "Another mod's grants count in the game too; P18 never takes them back." })
        local held = practiceHolds(member, ref.id)
        local change = (if held then "TAKE BACK " else "GRANT ") .. passiveName(ref) .. " - " .. MEMBER_NAMES[member]
        local note = if not held and otherHolds(member, ref.id) then "ANOTHER MOD ALREADY GRANTS IT" else nil
        for _, row in applyRows("passives", change, note,
            note and "The member already has it from another mod; P18's own grant would stay after that mod takes its back.") do
            table.insert(rows, row)
        end
        return rows
    end,
    adjust = function(key: string, direction: number)
        if key == "member" then
            passivePage.member = clamp(passivePage.member + direction, 1, #MEMBERS)
            editor("passives").armed = false
        elseif key == "skill" then
            passivePage.index = clamp(passivePage.index + direction, 1, math.max(#PASSIVES, 1))
            editor("passives").armed = false
        else
            applyAction("passives", key, direction, function(): Op?
                local member = MEMBERS[passivePage.member]
                local ref = PASSIVES[passivePage.index]
                if not ref then return nil end
                local held = practiceHolds(member, ref.id)
                return { kind = "passives", a = member, b = ref.id, field = if held then "revoke" else "grant",
                    label = (if held then "revoke " else "grant ") .. idText(ref.id, 3) .. " member=" .. tostring(member) }
            end)
        end
    end,
})

-- ---------------------------------------------------------------- sections and root

type Section = { key: string, title: string, features: { string } }
local SECTIONS: { Section } = {
    { key = "travel", title = "TRAVEL / CALENDAR", features = { "warp", "date", "time", "weather" } },
    { key = "party", title = "PARTY / INVENTORY", features = { "money", "items", "links", "stats", "levels", "personas", "passives" } },
    { key = "events", title = "EVENTS / BATTLES", features = { "flags", "encounter" } },
    { key = "display", title = "CAMERA / DISPLAY", features = { "camera", "hud" } },
    { key = "timing", title = "TIMING", features = { "frames", "timer", "pace" } },
}

for _, section in SECTIONS do
    definePage({
        key = "section." .. section.key, title = section.title, listed = false,
        rows = function(): RowList
            local rows: RowList = {}
            for _, key in section.features do
                local feature = FEATURES[key]
                table.insert(rows, toggleRow(feature))
                table.insert(rows, linkRow(feature, pages[key].key))
            end
            return rows
        end,
        adjust = function(key: string, direction: number)
            if string.sub(key, 1, 7) == "toggle." then toggleFeature(string.sub(key, 8)) end
        end,
    })
end

local about = p4.ui.detail({
    key = "about", title = "ABOUT PRACTICE TOOLS", listed = false,
    detail = function(): string
        return "Practice tools change the running game for practice only.\n" ..
            "Every tool has its own toggle and is OFF by default. Editors stage a value; APPLY shows the change and " ..
            "CONFIRM applies it at the next frame.\n" ..
            "Using any tool marks the session PRACTICE. A PRACTICE session cannot be saved, so practice changes never " ..
            "reach a normal save. Load a save or start a new game from the title for a clean session.\n" ..
            "Tools that are not listed here are not available in this version."
    end,
})

definePage({
    key = "root", title = "PRACTICE TOOLS",
    rows = function(): RowList
        local rows: RowList = {
            { key = "session", label = "SESSION", kind = "info", value = sessionSummary(),
                help = "PRACTICE sessions cannot be saved." },
            { key = "edits", label = "COMMITTED EDITS", kind = "info", value = tostring(session.edits) },
        }
        for _, section in SECTIONS do
            addRow(rows, { key = "section." .. section.key, label = section.title, kind = "link",
                target = pages["section." .. section.key].key })
        end
        addRow(rows, { key = "about", label = "ABOUT", kind = "link", target = about.key })
        return rows
    end,
})

if enabled("encounter") or enabled("warp") then p4.scene.state() end
updatePump()
log("PRACTICE_READY version=" .. VERSION .. " api=" .. p4.api_version)
