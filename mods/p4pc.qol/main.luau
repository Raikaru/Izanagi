--!strict
-- p4pc.qol: the bundled quality-of-life mod. Every feature has its own
-- [setting] toggle in mod.ini, OFF by default, and does nothing until the
-- player turns it on in MODS > p4pc.qol. Text and data come from the player's
-- own disc at runtime; no retail text or table is shipped here.

local VERSION = "1.0.0"

--------------------------------------------------------------------------
-- Shared helpers
--------------------------------------------------------------------------

local read = p4.mem.read
local function u8(b: buffer, o: number): number return buffer.readu8(b, o) end
local function u16(b: buffer, o: number): number return buffer.readu16(b, o) end
local function i16(b: buffer, o: number): number return buffer.readi16(b, o) end
local function u32(b: buffer, o: number): number return buffer.readu32(b, o) end

-- Features record that they actually ran this session; saves written
-- afterwards carry that list (see the save block at the end).
local touched: {[string]: boolean} = {}
-- The toggles refresh() last applied; pages read these so what they show
-- matches the features that are actually running.
local setting_cache: {[string]: boolean} = {}

-- Work spread over frames: each job is a function returning true when done.
-- Subscriptions cannot be made during a dispatch and a mod has 128 for its
-- lifetime, so this one is made here, disabled, and only switched on and off.
local jobs: {() -> boolean} = {}
local jobFrame: P4Subscription
jobFrame = p4.events.on("frame_start", function(_e)
    local job = jobs[1]
    if job and job() then table.remove(jobs, 1) end
    if #jobs == 0 then jobFrame:disable() end
end)
jobFrame:disable()
local function schedule(job: () -> boolean)
    table.insert(jobs, job)
    jobFrame:enable()
end

--------------------------------------------------------------------------
-- N40 FULL DESCRIPTIONS
--
-- The game draws help text from nine message tables installed at boot
-- (nmMesWindowInstallHandle, 002bc010, into the word array at 0063f2b0).
-- For the skill, item and equipment tables this feature builds a copy whose
-- records point at newly generated text, installs it as an ordinary message
-- table (itfMesWindowInstall, 002774d0) and, while the feature is ON, has
-- itfMesWindowBuildEntry (00279030) read records through that copy. Records
-- the generator cannot describe exactly keep pointing at the game's own text.
--------------------------------------------------------------------------

local HELP_HANDLES = 0x0063F2B0
local WINDOW_OBJECTS = 0x00881808 -- itfMes manager entries, 0x20 bytes each
local BUILD_ENTRY = 0x00279030
local INSTALL_TABLE = 0x002774D0
local ITEM_RECORD = 0x00106820 -- datEntryAt(id): the item's 0x44-byte record
local LIVE_ELEMENTS = 0x007644A4 -- SKILL.TBL elements: two bytes per skill
local LIVE_SKILLS = 0x007644A8 -- SKILL.TBL active records: 0x28 bytes each
local LIVE_BUFF_RATES = 0x007644F8 -- AICALC buff multipliers, percent bytes
local ACTIVE_SKILLS = 0x1B8
local REFERENCE_SKILL = 0x002 -- Agilao: the Simple style's 100%
local LINE_LIMIT = 24 -- characters a help line holds at the game's font width

local HELP_WEAPON, HELP_ARMOR, HELP_ACCESSORY, HELP_ITEM, HELP_SKILL = 0, 1, 2, 4, 6

local ELEMENT_NAMES = {[0] = "Phys", "Fire", "Ice", "Elec", "Wind", "Almighty", "Light", "Dark"}
local AILMENTS = { -- status mask bits, in the order sentences name them
    {0x01, "Dizzy"}, {0x02, "Rage"}, {0x04, "Fear"}, {0x08, "Silence"}, {0x10, "Confusion"},
    {0x20, "Poison"}, {0x40, "Exhaustion"}, {0x80, "Enervation"}, {0x100000, "Down"},
}

local function element_name(element: number): string?
    return ELEMENT_NAMES[element]
end

local function ailment_list(mask: number): string?
    if mask == 0xFF then return "all ailments" end
    local names = {}
    local left = mask
    for _, entry in AILMENTS do
        if bit32.btest(mask, entry[1]) then
            table.insert(names, entry[2])
            left = bit32.band(left, bit32.bnot(entry[1]))
        end
    end
    if left ~= 0 or #names == 0 then return nil end
    return table.concat(names, ", ")
end

-- "1 foe", "all allies", ... from the target class, side mask and filters.
local function target_text(class: number, side: number, filter: number): string?
    if side == 2 then
        if class == 0 and filter == 0 then return "1 foe" end
        if class == 1 and filter == 0 then return "all foes" end
    elseif side == 1 then
        if class == 0 and filter == 0 then return "1 ally" end
        if class == 1 and filter == 0 then return "all allies" end
        if class == 0 and filter == 1 then return "self" end
        if class == 0 and filter == 4 then return "1 fallen ally" end
        if class == 1 and filter == 4 then return "fallen allies" end
    end
    return nil
end

local function capitalized(text: string): string
    return string.upper(string.sub(text, 1, 1)) .. string.sub(text, 2)
end

type Skill = {
    id: number, element: number, damageClass: number, target: string?,
    hit: number, hitsMin: number, hitsMax: number,
    hpKind: number, hpPower: number, spKind: number, spPower: number,
    statusMode: number, statusChance: number, statusMask: number,
    buffMask: number, special: number, crit: number, flags: number,
}

local function read_skill(id: number): Skill?
    if id <= 0 or id >= ACTIVE_SKILLS then return nil end
    local records = read("u32", LIVE_SKILLS)
    local elements = read("u32", LIVE_ELEMENTS)
    if records == 0 or elements == 0 then return nil end
    local b = p4.mem.read_bytes(records + id * 0x28, 0x28)
    return {
        id = id, element = read("u8", elements + id * 2), damageClass = u8(b, 0x02),
        target = target_text(u8(b, 0x08), u8(b, 0x09), u8(b, 0x0A)),
        hit = u8(b, 0x0E), hitsMin = u8(b, 0x0F), hitsMax = u8(b, 0x10),
        hpKind = u8(b, 0x11), hpPower = i16(b, 0x12), spKind = u8(b, 0x14), spPower = i16(b, 0x16),
        statusMode = u8(b, 0x18), statusChance = u8(b, 0x19), statusMask = u32(b, 0x1C),
        buffMask = u32(b, 0x20), special = u8(b, 0x24), crit = u8(b, 0x25), flags = u8(b, 0x00),
    }
end

-- Buff multipliers from the live AICALC table: party bank, levels -1/0/+1.
local function buff_rate(bank: number, level: number): number?
    local rates = read("u32", LIVE_BUFF_RATES)
    if rates == 0 then return nil end
    return read("u8", rates + bank * 3 + level + 1)
end

-- Integer percent as a multiplier, formatted without floating point so the
-- text cannot depend on the rounding mode of the frame that builds it.
local function multiplier(percent: number): string
    return string.format("x%d.%02d", percent // 100, percent % 100)
end

-- 100 * sqrt(power / reference) rounded to the nearest whole percent, ties
-- upward, decided with exact integer comparisons of the squared bounds.
local function relative_strength(power: number, reference: number): number
    local target = 40000 * power
    local n = math.floor(math.sqrt(10000 * power / reference))
    while n > 0 and (2 * n - 1) ^ 2 * reference > target do n -= 1 end
    while (2 * n + 1) ^ 2 * reference <= target do n += 1 end
    return n
end

local function hits_text(s: Skill): string
    if s.hitsMin == s.hitsMax then
        return s.hitsMin == 1 and "1 hit" or string.format("%d hits", s.hitsMin)
    end
    return string.format("%d-%d hits", s.hitsMin, s.hitsMax)
end

local function damage_lines(s: Skill, style: string, reference: number): {string}?
    local element = element_name(s.element)
    if not element or not s.target or s.hpPower <= 0 then return nil end
    if s.hpKind ~= 1 and s.hpKind ~= 12 and s.hpKind ~= 14 then return nil end
    if s.special ~= 0 or s.spKind ~= 0 or s.buffMask ~= 0 then return nil end
    local first
    if style == "Exact" or reference <= 0 then
        first = string.format("%d power %s dmg.", s.hpPower, element)
    else
        local percent = relative_strength(s.hpPower, reference)
        first = string.format("%d%% %s dmg.", percent, element)
    end
    local second = capitalized(s.target) .. ", " .. hits_text(s) .. "."
    local third
    if s.damageClass == 1 then
        third = string.format("%d%% Acc, %d%% Crit.", s.hit, s.crit)
    else
        third = string.format("%d%% base Acc.", s.hit)
    end
    if s.statusMode == 1 and s.statusMask ~= 0 then
        if s.damageClass == 1 then return nil end -- no room for Acc, Crit and an ailment
        local ailment = ailment_list(s.statusMask)
        if not ailment or string.find(ailment, ",") then return nil end
        third = string.format("%d%% Acc, %d%% %s.", s.hit, s.statusChance, ailment)
    elseif s.statusMode ~= 0 then
        return nil
    end
    return {first, second, third}
end

local function heal_lines(s: Skill): {string}?
    if not s.target or s.special ~= 0 or s.buffMask ~= 0 or s.spKind ~= 0 then return nil end
    local first
    if s.statusMode == 2 and s.statusMask == 0x80000 then
        if s.hpKind ~= 11 then return nil end
        first = string.format("Revives with %d%% HP.", s.hpPower)
    elseif s.statusMode ~= 0 then
        return nil
    elseif s.hpKind == 2 then
        first = string.format("Heals %d + Magic bonus.", s.hpPower)
    elseif s.hpKind == 5 then
        first = string.format("Restores %d HP.", s.hpPower)
    elseif s.hpKind == 11 then
        first = string.format("Restores %d%% max HP.", s.hpPower)
    else
        return nil
    end
    local second = capitalized(s.target) .. "."
    if s.hpKind == 2 then return {first, second, "95-105% variation."} end
    return {first, second}
end

local function sp_lines(s: Skill): {string}?
    if not s.target or s.hpKind ~= 0 or s.statusMode ~= 0 or s.buffMask ~= 0 then return nil end
    if s.spKind == 5 then return {string.format("Restores %d SP.", s.spPower), capitalized(s.target) .. "."} end
    if s.spKind == 11 then return {string.format("Restores %d%% max SP.", s.spPower), capitalized(s.target) .. "."} end
    return nil
end

local function cure_lines(s: Skill): {string}?
    if not s.target or s.statusMode ~= 2 or s.hpKind ~= 0 or s.spKind ~= 0 or s.buffMask ~= 0 then return nil end
    local list = ailment_list(s.statusMask)
    if not list then return nil end
    return {"Cures " .. list .. ".", capitalized(s.target) .. "."}
end

local function ailment_lines(s: Skill): {string}?
    if not s.target or s.statusMode ~= 1 or s.hpKind ~= 0 or s.spKind ~= 0 or s.buffMask ~= 0 then return nil end
    if s.statusMask == 0x80000 then
        local element = element_name(s.element)
        if s.element ~= 6 and s.element ~= 7 or not element then return nil end
        return {string.format("%d%% base %s KO.", s.statusChance, element), capitalized(s.target) .. ".",
                "Luck and resists apply."}
    end
    local list = ailment_list(s.statusMask)
    if not list or string.find(list, ",") then return nil end
    return {string.format("%d%% base %s.", s.statusChance, list), capitalized(s.target) .. ".",
            "Luck and resists apply."}
end

-- Buff mask bits: 0x1/0x2 attack up/down, 0x4/0x8 second attack slot,
-- 0x10/0x20 and 0x100/0x200 hit/evasion, 0x40/0x80 defense, 0x400/0x800
-- Power/Mind Charge (x2.5 on the next applicable attack, then consumed).
local function buff_lines(s: Skill): {string}?
    if not s.target or s.hpKind ~= 0 or s.spKind ~= 0 or s.statusMode ~= 0 then return nil end
    local mask = s.buffMask
    if mask == 0x400 then return {"Next Phys dmg x2.5.", "Self. Lasts until used."} end
    if mask == 0x800 then return {"Next Magic dmg x2.5.", "Self. Lasts until used."} end
    local parts = {}
    local attackUp, attackDown = buff_rate(0, 1), buff_rate(0, -1)
    local evadeUp, evadeDown = buff_rate(2, 1), buff_rate(2, -1)
    if not attackUp or not attackDown or not evadeUp or not evadeDown then return nil end
    local known = 0
    if bit32.band(mask, 0x5) == 0x5 then table.insert(parts, "Dmg dealt " .. multiplier(attackUp)); known += 0x5 end
    if bit32.band(mask, 0xA) == 0xA then table.insert(parts, "Dmg dealt " .. multiplier(attackDown)); known += 0xA end
    if bit32.btest(mask, 0x40) then table.insert(parts, "Dmg taken " .. multiplier(attackDown)); known += 0x40 end
    if bit32.btest(mask, 0x80) then table.insert(parts, "Dmg taken " .. multiplier(attackUp)); known += 0x80 end
    if bit32.band(mask, 0x110) == 0x110 then table.insert(parts, "Hit/Evade " .. multiplier(evadeUp)); known += 0x110 end
    if bit32.band(mask, 0x220) == 0x220 then table.insert(parts, "Hit/Evade " .. multiplier(evadeDown)); known += 0x220 end
    if known ~= mask or #parts == 0 or #parts > 2 then return nil end
    local lines = {}
    for _, part in parts do table.insert(lines, part .. ".") end
    table.insert(lines, capitalized(s.target) .. ", 3 turns.")
    return lines
end

-- Passives whose effect is fixed in the game's code, with the code's numbers.
local PASSIVES: {[number]: {string}} = {
    [0x1E7] = {"Fire dmg x1.25.", "Stacks with Amp.", "Element cap x1.875."},
    [0x1E8] = {"Fire dmg x1.50.", "Stacks with Boost.", "Element cap x1.875."},
    [0x1E9] = {"Ice dmg x1.25.", "Stacks with Amp.", "Element cap x1.875."},
    [0x1EA] = {"Ice dmg x1.50.", "Stacks with Boost.", "Element cap x1.875."},
    [0x1EB] = {"Elec dmg x1.25.", "Stacks with Amp.", "Element cap x1.875."},
    [0x1EC] = {"Elec dmg x1.50.", "Stacks with Boost.", "Element cap x1.875."},
    [0x1ED] = {"Wind dmg x1.25.", "Stacks with Amp.", "Element cap x1.875."},
    [0x1EE] = {"Wind dmg x1.50.", "Stacks with Boost.", "Element cap x1.875."},
    [0x1F6] = {"10% chance to reflect", "Phys attacks.", "Best counter applies."},
    [0x1F7] = {"15% chance to reflect", "Phys attacks.", "Best counter applies."},
    [0x1F8] = {"20% chance to reflect", "Phys attacks.", "Best counter applies."},
    [0x1F9] = {"Regain 2% max HP", "each turn, rounded down."},
    [0x1FA] = {"Regain 4% max HP", "each turn, rounded down."},
    [0x1FB] = {"Regain max HP/16.66", "each turn, rounded down."},
    [0x1FC] = {"Regain 3 SP each turn."},
    [0x1FD] = {"Regain 5 SP each turn."},
    [0x1FE] = {"Regain 7 SP each turn."},
    [0x206] = {"Crit chance against", "you x0.5."},
    [0x207] = {"Your crit chance x2."},
    [0x208] = {"Attacker's hit chance", "x0.5 before the floor."},
    [0x209] = {"Dmg taken x0.5.", "You can never dodge."},
    [0x20A] = {"SP costs x0.5.", "Round down, min 1 SP.", "Free skills stay free."},
    [0x20B] = {"HP costs x0.5.", "Round down, min 1 HP."},
    [0x20E] = {"Healing magic x1.5."},
    [0x20F] = {"Survive a fatal hit", "with 1 HP. Once per", "battle."},
    [0x210] = {"Survive a fatal hit", "with full HP. Once per", "battle."},
    [0x211] = {"50% chance to survive", "a Light KO with 1 HP."},
    [0x212] = {"50% chance to survive", "a Dark KO with 1 HP."},
    [0x223] = {"After a win, regain 8%", "of max HP and SP."},
    [0x224] = {"After a win, fully", "restore HP and SP."},
}

local function skill_lines(id: number, style: string, reference: number): {string}?
    local passive = PASSIVES[id]
    if passive then return passive end
    local s = read_skill(id)
    if not s then return nil end
    if s.hpKind ~= 0 and s.hpKind ~= 2 and s.hpKind ~= 5 and s.hpKind ~= 11 then
        return damage_lines(s, style, reference)
    end
    return heal_lines(s) or sp_lines(s) or cure_lines(s) or ailment_lines(s) or buff_lines(s)
end

local function item_record(id: number): buffer?
    local address = p4.call(ITEM_RECORD, "ph", id) :: number
    if address == 0 then return nil end
    return p4.mem.read_bytes(address, 0x44)
end

local function weapon_lines(id: number): {string}?
    local r = item_record(id)
    if not r or u8(r, 0) ~= 0 then return nil end
    return {string.format("Attack %d, Hit %d.", u16(r, 0x08), u16(r, 0x0A)),
            string.format("Base Crit %d%%.", u8(r, 0x0E))}
end

local function armor_lines(id: number): {string}?
    local r = item_record(id)
    if not r or u8(r, 0) ~= 1 then return nil end
    return {string.format("Defense %d.", u16(r, 0x10)), string.format("Evasion %d.", u16(r, 0x12))}
end

local VOW_ELEMENTS = {[0xBD] = "Fire", [0xBE] = "Ice", [0xBF] = "Elec", [0xC0] = "Wind"}

local function accessory_lines(id: number, style: string, reference: number): {string}?
    local r = item_record(id)
    if not r or u8(r, 0) ~= 2 then return nil end
    local granted, property, percent = u16(r, 0x24), u8(r, 0x28), u16(r, 0x2A)
    if granted ~= 0 and property == 0 then return skill_lines(granted, style, reference) end
    local element = VOW_ELEMENTS[property]
    if element and granted == 0 and percent > 0 then
        return {string.format("%s dmg %s.", element, multiplier(100 + percent)), "Stacks with Boost/Amp.",
                "Element cap x1.875."}
    end
    return nil
end

local function consumable_lines(id: number, style: string, reference: number): {string}?
    local r = item_record(id)
    if not r or u8(r, 0) ~= 3 then return nil end
    local granted = u16(r, 0x24)
    if granted == 0 then return nil end
    return skill_lines(granted, style, reference)
end

type HelpTable = {slot: number, handle: number, record: number, count: number, first: number,
                  lines: (number, string, number) -> {string}?,
                  copy: P4GuestBuffer?, copyHandle: number?, textBase: number}

local DIALOG_BYTES = 0x40 -- name[24], pages, speaker, one text pointer, text size
local TEXT_BYTES = 96

local function help_table(slot: number, first: number, lines: (number, string, number) -> {string}?): HelpTable?
    local handle = read("i32", HELP_HANDLES + slot * 4)
    if handle < 0 then return nil end
    local window = read("u32", WINDOW_OBJECTS + handle * 0x20)
    if window == 0 then return nil end
    local record = read("u32", window + 4)
    if record == 0 or read("u32", record + 8) ~= 0x3147534D or read("u8", record + 0x1C) == 0 then return nil end
    return {slot = slot, handle = handle, record = record, count = read("u32", record + 0x18), first = first,
            lines = lines, copy = nil, copyHandle = nil, textBase = 0}
end

-- One generated record: MESSAGE bytes are the record's own leading font
-- commands (copied from the game's record), ASCII lines, LF after each, NUL.
local function encode_record(t: HelpTable, index: number, text: {string}, slotAddress: number): buffer?
    local dialog = read("u32", t.record + 0x24 + index * 8)
    if dialog == 0 or read("i16", dialog + 0x18) < 1 then return nil end
    local original = read("u32", dialog + 0x1C)
    local prefix = {}
    local at = original
    while true do
        local token = read("u8", at)
        if token < 0xF0 then break end
        for i = 0, (token % 16) * 2 - 1 do table.insert(prefix, read("u8", at + i)) end
        at += (token % 16) * 2
        if #prefix > 16 then return nil end
    end
    local body = table.concat(text, "\n") .. "\n"
    local size = #prefix + #body + 1
    if DIALOG_BYTES + size > DIALOG_BYTES + TEXT_BYTES then return nil end
    local out = buffer.create(DIALOG_BYTES + TEXT_BYTES)
    buffer.copy(out, 0, p4.mem.read_bytes(dialog, 0x1C))
    buffer.writei16(out, 0x18, 1)
    buffer.writeu32(out, 0x1C, slotAddress + DIALOG_BYTES)
    buffer.writeu32(out, 0x20, size)
    for i, byte in prefix do buffer.writeu8(out, DIALOG_BYTES + i - 1, byte) end
    buffer.writestring(out, DIALOG_BYTES + #prefix, body)
    return out
end

local function valid_lines(text: {string}?): boolean
    if not text or #text == 0 or #text > 3 then return false end
    for _, line in text do
        if #line == 0 or #line > LINE_LIMIT or string.find(line, "[^ -~]") then return false end
    end
    return true
end

local descriptions = {
    tables = {} :: {HelpTable},
    swap = {} :: {[number]: number}, -- retail handle -> generated handle
    style = "",
    generating = false,
    generated = 0,
    kept = 0,
}

local function description_reference(): number
    local agilao = read_skill(REFERENCE_SKILL)
    if not agilao or agilao.hpKind ~= 1 or agilao.hpPower <= 0 then return 0 end
    return agilao.hpPower
end

-- Header 0x20, entry table, the three tail words, then fixed dialog slots.
local function build_table(t: HelpTable, style: string, reference: number)
    local size = 0x20 + t.count * 8 + 12
    local slots = size
    size += t.count * (DIALOG_BYTES + TEXT_BYTES)
    local copy = t.copy
    if not copy then
        copy = p4.mem.alloc(size)
        t.copy = copy
    end
    assert(copy)
    local base = copy.address
    local header = p4.mem.read_bytes(t.record, 0x20)
    buffer.writeu32(header, 0x04, size)
    buffer.writeu32(header, 0x10, 0)
    buffer.writeu32(header, 0x14, 0)
    buffer.writeu8(header, 0x1C, 1)
    p4.mem.write_bytes(base, header)
    p4.mem.write_bytes(base + 0x20 + t.count * 8, p4.mem.read_bytes(t.record + 0x20 + t.count * 8, 12))
    local index = 0
    return function(): boolean
        local stop = math.min(t.count, index + 12)
        local entries = p4.mem.read_bytes(t.record + 0x20 + index * 8, (stop - index) * 8)
        for i = index, stop - 1 do
            local text = t.lines(t.first + i, style, reference)
            local slot = base + slots + i * (DIALOG_BYTES + TEXT_BYTES)
            local encoded = valid_lines(text) and u32(entries, (i - index) * 8) == 0 and
                encode_record(t, i, text :: {string}, slot) or nil
            if encoded then
                p4.mem.write_bytes(slot, encoded)
                buffer.writeu32(entries, (i - index) * 8 + 4, slot)
                descriptions.generated += 1
            else
                descriptions.kept += 1
            end
        end
        p4.mem.write_bytes(base + 0x20 + index * 8, entries)
        index = stop
        return index >= t.count
    end
end

local function description_hook(context: P4HookContext)
    local handle = context.args[8] :: number
    local generated = descriptions.swap[handle]
    if generated then
        context.args[8] = generated
        touched.full_descriptions = true
    end
end

local function generate_descriptions(style: string)
    if descriptions.generating then return end
    descriptions.generating = true
    descriptions.generated, descriptions.kept = 0, 0
    if #descriptions.tables == 0 then
        local specs = {
            {HELP_SKILL, 0, function(id, st, ref) return skill_lines(id, st, ref) end},
            {HELP_ITEM, 0x300, consumable_lines},
            {HELP_WEAPON, 0x000, function(id) return weapon_lines(id) end},
            {HELP_ARMOR, 0x100, function(id) return armor_lines(id) end},
            {HELP_ACCESSORY, 0x200, accessory_lines},
        }
        for _, spec in specs do
            local t = help_table(spec[1], spec[2], spec[3])
            if t then table.insert(descriptions.tables, t) end
        end
    end
    local reference = description_reference()
    for _, t in descriptions.tables do
        local step = nil :: (() -> boolean)?
        schedule(function(): boolean
            if not step then step = build_table(t, style, reference) end
            return (step :: () -> boolean)()
        end)
    end
    schedule(function(): boolean
        for _, t in descriptions.tables do
            if not t.copyHandle then
                local copy = t.copy :: P4GuestBuffer
                t.copyHandle = p4.call(INSTALL_TABLE, "ip", copy.address) :: number
            end
            descriptions.swap[t.handle] = t.copyHandle :: number
        end
        descriptions.style = style
        descriptions.generating = false
        p4.log(string.format("QOL_DESCRIPTIONS style=%s generated=%d kept=%d reference=%d tables=%d",
            style, descriptions.generated, descriptions.kept, reference, #descriptions.tables))
        return true
    end)
end

-- Hooks are registered once at startup (never during a dispatch) and stay
-- disabled until their feature is turned on.
local descriptionHook = p4.hook(BUILD_ENTRY, "pfffiiiiii", "before", description_hook)
descriptionHook:disable()

local function apply_descriptions(on: boolean, style: string)
    if on then
        if descriptions.style ~= style then generate_descriptions(style) end
        descriptionHook:enable()
    else
        descriptionHook:disable()
    end
end

--------------------------------------------------------------------------
-- Q2 DIALOGUE BACKLOG
--
-- itfMesWindowBindRecord (002778c0) binds each message of a conversation to
-- its window. While the toggle is ON the hook copies the bound message's
-- speaker and pages into a session-only ring of the last messages; the
-- package page's DIALOGUE BACKLOG detail lists them, newest first. Nothing is
-- written to game memory or the save.
--------------------------------------------------------------------------

local BIND_RECORD = 0x002778C0
local BACKLOG_LIMIT = 64
local BACKLOG_BYTES = 30000 -- the native detail page holds 32768

-- MESSAGE bytes as the RAW text native pages draw: printable ASCII stays,
-- line breaks become spaces, command tokens are skipped and two-byte glyphs
-- (symbols outside ASCII) are left out rather than guessed.
local function decode_bytes(bytes: buffer): string
    -- String searches run in C, so the loop below steps over the few tokens
    -- and glyphs rather than every byte; it gives up on runs of more than 128.
    local s = buffer.readstring(bytes, 0, buffer.len(bytes))
    local stop = string.find(s, "\0", 1, true)
    if stop then s = string.sub(s, 1, stop - 1) end
    local out = {}
    local at = 1
    for _ = 1, 128 do
        local i = string.find(s, "[\128-\255]", at)
        if not i then
            table.insert(out, string.sub(s, at))
            break
        end
        table.insert(out, string.sub(s, at, i - 1))
        local c = string.byte(s, i)
        at = i + (c >= 0xF0 and math.max(2, (c % 16) * 2) or 2)
        if at > #s then break end
    end
    local text = string.gsub(string.gsub(table.concat(out), "%c", " "), "%s+", " ")
    return (string.gsub(text, "^ ", ""))
end

local backlog = {
    entries = {} :: {{speaker: string, text: string}},
    on = false,
    revision = 0,
}

local function speaker_bytes(record: number, count: number, dialog: number): buffer?
    local speaker = read("u16", dialog + 0x1A)
    if speaker == 0xFFFF then return nil end
    local names = read("u32", record + 0x20 + count * 8)
    if names == 0 then return nil end
    local name = read("u32", names + speaker * 4)
    return name ~= 0 and p4.mem.read_bytes(name, 48) or nil
end

-- The hook only copies the message's bytes (its pages may be unloaded soon
-- after); decoding waits for later frames, one page per frame, so a long or
-- token-heavy message cannot exceed a frame's budget.
local pending: {{speaker: buffer?, pages: {buffer}, parts: {string}}} = {}
local function decode_pending(): boolean
    local message = pending[1]
    if not message then return true end
    local page = table.remove(message.pages, 1)
    if page then
        local text = decode_bytes(page)
        if text ~= "" then table.insert(message.parts, text) end
        return false
    end
    table.remove(pending, 1)
    if #message.parts > 0 then
        table.insert(backlog.entries, {speaker = message.speaker and decode_bytes(message.speaker :: buffer) or "",
                                       text = table.concat(message.parts, " ")})
        if #backlog.entries > BACKLOG_LIMIT then table.remove(backlog.entries, 1) end
        backlog.revision += 1
    end
    return #pending == 0
end

local function backlog_hook(context: P4HookContext)
    local handle, index = context.args[1] :: number, context.args[2] :: number
    if handle < 0 or handle >= 0x40 then return end
    local window = read("u32", WINDOW_OBJECTS + handle * 0x20)
    if window == 0 then return end
    local record = read("u32", window + 4)
    if record == 0 or index < 0 then return end
    local count = read("u32", record + 0x18)
    if index >= count or read("u32", record + 0x20 + index * 8) ~= 0 then return end
    local dialog = read("u32", record + 0x24 + index * 8)
    local pages = read("i16", dialog + 0x18)
    if pages <= 0 or pages > 32 then return end
    local copies = {}
    for page = 0, pages - 1 do
        table.insert(copies, p4.mem.read_bytes(read("u32", dialog + 0x1C + page * 4), 512))
    end
    if #pending == 0 then schedule(decode_pending) end
    table.insert(pending, {speaker = speaker_bytes(record, count, dialog), pages = copies, parts = {}})
    if #pending > BACKLOG_LIMIT then table.remove(pending, 1) end
    touched.backlog = true
end

p4.ui.detail({
    key = "backlog",
    title = "DIALOGUE BACKLOG",
    detail = function(): string
        if not backlog.on then return "Turn on DIALOGUE BACKLOG to keep the lines of each conversation here." end
        p4.log("QOL_BACKLOG entries=" .. tostring(#backlog.entries))
        if #backlog.entries == 0 then return "No dialogue yet in this session." end
        local lines, size = {}, 0
        for i = #backlog.entries, 1, -1 do
            local entry = backlog.entries[i]
            local line = (entry.speaker ~= "" and entry.speaker .. ": " or "") .. entry.text
            size += #line + 2
            if size > BACKLOG_BYTES then break end
            table.insert(lines, line)
        end
        return table.concat(lines, "\n\n")
    end,
})

local backlogHook = p4.hook(BIND_RECORD, "iiii", "after", backlog_hook)
backlogHook:disable()

local function apply_backlog(on: boolean)
    if on == backlog.on then return end
    backlog.on = on
    if on then
        backlogHook:enable()
    else
        backlogHook:disable()
        backlog.entries = {}
    end
end

--------------------------------------------------------------------------
-- Shared roster/compendium readers. Persona records are 0x30 bytes: flags
-- (bit 0 present) +0, id +2, level +4, eight skills +0x0C, stats +0x1C.
--------------------------------------------------------------------------

local LIVE_PERSONAS = 0x007644C4 -- PERSONA.TBL base records, 0x0E bytes: arcana +2, level +3
local LIVE_SKILL_NAMES = 0x00764538 -- MSG.TBL skill names, 0x13 bytes each
local LIVE_PERSONA_NAMES = 0x00764530 -- MSG.TBL persona names, 17 bytes each
local COMPENDIUM = 0x007981CC -- 256 registered persona records
local ROSTER = 0x00797F8C -- the protagonist's 12 persona slots
local ROSTER_ACTIVE = 0x00797F88 -- s16 index of the equipped roster slot
local PARTY_PERSONA = 0x00796D94 -- + member id * 0x88: a party member's Persona
local ARCANA: {[number]: string} = {"Fool", "Magician", "Priestess", "Empress", "Emperor", "Hierophant",
    "Lovers", "Chariot", "Justice", "Hermit", "Fortune", "Strength", "Hanged Man", "Death", "Temperance", "Devil",
    "Tower", "Star", "Moon", "Sun", "Judgement"}
ARCANA[24] = "World"
local HERO_LEVEL = 0x007973CA -- the protagonist's unit record level byte
local STAT_NAMES = {"St", "Ma", "En", "Ag", "Lu"}

local function raw(text: string): string
    return (string.gsub(text, "[^ -~]", "?"))
end
local function skill_name(id: number): string
    local names = read("u32", LIVE_SKILL_NAMES)
    if names == 0 or id <= 0 or id >= 576 then return "-" end
    return raw(p4.mem.read_string(names + id * 0x13, 0x13))
end
local function persona_name_raw(id: number): string
    local names = read("u32", LIVE_PERSONA_NAMES)
    if names == 0 or id <= 0 or id >= 256 then return "" end
    return p4.mem.read_string(names + id * 17, 17)
end
local function persona_name(id: number): string
    local name = persona_name_raw(id)
    return name ~= "" and raw(name) or "?"
end
local function persona_base(id: number): (number, number)
    local table_ = read("u32", LIVE_PERSONAS)
    if table_ == 0 or id <= 0 or id >= 256 then return 0, 0 end
    return read("u8", table_ + id * 0x0E + 2), read("u8", table_ + id * 0x0E + 3)
end
-- Names, arcana and base levels do not change after boot: read them once,
-- with both list orders, so UI callbacks stay within their budget. A real
-- Persona has a name and a base level; placeholder rows are named by their
-- hexadecimal ID.
local personas = {byLevel = {} :: {number}, byName = {} :: {number},
    info = {} :: {[number]: {name: string, arcana: number, level: number}}}
do
    for id = 1, 255 do
        local name = persona_name_raw(id)
        local arcana, level = persona_base(id)
        if level > 0 and name ~= "" and not string.find(name, "[^ -~]") and not string.match(name, "^%x%x%x$") then
            personas.info[id] = {name = name, arcana = arcana, level = level}
            table.insert(personas.byLevel, id)
            table.insert(personas.byName, id)
        end
    end
    table.sort(personas.byLevel, function(a, b)
        local x, y = personas.info[a], personas.info[b]
        return x.level < y.level or x.level == y.level and a < b
    end)
    table.sort(personas.byName, function(a, b) return personas.info[a].name < personas.info[b].name end)
end
type PersonaRecord = {id: number, level: number, skills: {number}, stats: {number}}
local function persona_record(address: number): PersonaRecord?
    local b = p4.mem.read_bytes(address, 0x30)
    if not bit32.btest(u16(b, 0), 1) or u16(b, 2) == 0 then return nil end
    local skills, stats = {}, {}
    for i = 0, 7 do
        local skill = u16(b, 0x0C + i * 2)
        if skill ~= 0 then table.insert(skills, skill) end
    end
    for i = 0, 4 do table.insert(stats, u8(b, 0x1C + i) + buffer.readi8(b, 0x21 + i) + buffer.readi8(b, 0x26 + i)) end
    return {id = u16(b, 2), level = u8(b, 4), skills = skills, stats = stats}
end
local function roster(): {{slot: number, record: PersonaRecord}}
    local out = {}
    for slot = 0, 11 do
        local record = persona_record(ROSTER + slot * 0x30)
        if record then table.insert(out, {slot = slot, record = record}) end
    end
    return out
end
local function skill_cost(id: number): string
    if id >= ACTIVE_SKILLS then return "Passive" end
    local records = read("u32", LIVE_SKILLS)
    if records == 0 then return "" end
    local b = p4.mem.read_bytes(records + id * 0x28, 8)
    local class, percent, flat = u8(b, 3), u16(b, 4), u16(b, 6)
    if class == 1 then return flat > 0 and string.format("%d%%+%d HP", percent, flat) or string.format("%d%% HP", percent) end
    if class == 2 then return string.format("%d SP", percent + flat) end
    return ""
end
local function skill_facts(id: number): string
    local style = p4.settings.get("description_style") :: string
    local lines = skill_lines(id, style, description_reference())
    return lines and table.concat(lines, " ") or "(the game's own description)"
end

--------------------------------------------------------------------------
-- N29 SKILL COMPARE: each member's eight skills in their own slot order,
-- with cost; pick two (A, B) and read their facts side by side. Read-only:
-- no skill order or game record is written.
--------------------------------------------------------------------------

-- UI callbacks have a small safepoint budget, so the rows are formatted in
-- the background, one member per frame, whenever the party's Personas or
-- skills change; the page's callbacks only index these tables.
type CompareRow = {id: number, name: string, cost: string, facts: string}
local compare = {member = 0, picks = {} :: {CompareRow}, key = "", busy = false,
    members = {} :: {{name: string, rows: {CompareRow}}}}
local function compare_addresses(): {number}
    local addresses = {}
    local active = read("i16", ROSTER_ACTIVE)
    if active >= 0 and active < 12 then table.insert(addresses, ROSTER + active * 0x30) end
    for member = 2, 8 do table.insert(addresses, PARTY_PERSONA + member * 0x88) end
    return addresses
end
local function compare_refresh()
    local addresses = compare_addresses()
    local parts = {}
    for _, address in addresses do
        table.insert(parts, buffer.tostring(p4.mem.read_bytes(address, 0x1C)))
    end
    local key = table.concat(parts) .. (p4.settings.get("description_style") :: string)
    if key == compare.key or compare.busy then return end
    compare.busy = true
    local members = {}
    local next_ = 1
    schedule(function(): boolean
        local record = persona_record(addresses[next_])
        if record and #record.skills > 0 then
            local rows = {}
            for _, id in record.skills do
                table.insert(rows, {id = id, name = skill_name(id), cost = skill_cost(id), facts = skill_facts(id)})
            end
            table.insert(members, {name = persona_name(record.id), rows = rows})
        end
        next_ += 1
        if next_ <= #addresses then return false end
        compare.members, compare.key, compare.busy = members, key, false
        if compare.member >= #members then compare.member = 0 end
        compare.picks = {}
        p4.log(string.format("QOL_SKILL_COMPARE members=%d", #members))
        return true
    end)
end
local function compare_rows(): {CompareRow}
    local member = compare.members[compare.member + 1]
    return member and member.rows or {}
end
local compareDetail = p4.ui.detail({
    key = "skills.compare", title = "SKILL A VS B",
    detail = function(): string
        if #compare.picks == 0 then return "Pick a skill (A), then another (B), on SKILL COMPARE." end
        local parts = {}
        for i, row in compare.picks do
            table.insert(parts, string.format("%s: %s (%s)\n%s", i == 1 and "A" or "B", row.name, row.cost, row.facts))
        end
        return table.concat(parts, "\n\n")
    end,
})
local comparePage: P4UIPage? = nil
comparePage = p4.ui.page({
    key = "skills", title = "SKILL COMPARE",
    count = function(): number
        if not setting_cache.skill_compare then return 1 end
        return 2 + #compare_rows()
    end,
    row = function(index: number): P4UIRow
        if not setting_cache.skill_compare then
            return {key = "off", label = "TURN ON SKILL COMPARE", kind = "info"}
        end
        if index == 1 then
            local member = compare.members[compare.member + 1]
            return {key = "member", label = "PERSONA", kind = "value", value = member and member.name or "-",
                    help = "Left or right: the Persona of each party member."}
        end
        local rows = compare_rows()
        if index == 2 + #rows then
            return {key = "compare", label = "COMPARE A AND B", kind = "link", target = compareDetail.key}
        end
        local row = rows[index - 1]
        local mark = compare.picks[1] == row and "A " or compare.picks[2] == row and "B " or ""
        return {key = "skill" .. tostring(index - 1), label = row.name, kind = "action",
                value = mark .. row.cost, help = row.facts}
    end,
    find = function(key: string): number?
        if key == "member" then return 1 end
        local slot = tonumber(string.match(key, "^skill(%d)$"))
        if slot and slot <= #compare_rows() then return slot + 1 end
        if key == "compare" then return 2 + #compare_rows() end
        return nil
    end,
    adjust = function(key: string, direction: number)
        if key == "member" and direction ~= 0 then
            compare.member = (compare.member + direction) % math.max(1, #compare.members)
        else
            local slot = tonumber(string.match(key, "^skill(%d)$"))
            local row = slot and compare_rows()[slot]
            if row and direction == 0 then
                table.insert(compare.picks, row)
                while #compare.picks > 2 do table.remove(compare.picks, 1) end
                touched.skill_compare = true
            end
        end
        local page = comparePage
        if page then page:refresh() end
    end,
})

--------------------------------------------------------------------------
-- N22 COMPENDIUM FILTER: every Persona by arcana, registration and level or
-- name, and a registered-versus-carried comparison. Registering and summoning
-- stay in the Velvet Room; nothing here writes the compendium.
--------------------------------------------------------------------------

local SHOW = {"All", "Registered", "Not registered", "Carried"}
local compendium = {arcana = 0, show = 1, sortName = false, list = {} :: {number}, selected = 0}
local function arcana_choices(): {number}
    local out = {0}
    for id = 1, 24 do if ARCANA[id] then table.insert(out, id) end end
    return out
end
local function compendium_list(): {number}
    local carried = {}
    for _, entry in roster() do carried[entry.record.id] = true end
    local show = SHOW[compendium.show]
    local out = {}
    for _, id in (compendium.sortName and personas.byName or personas.byLevel) do
        local info = personas.info[id]
        if compendium.arcana == 0 or info.arcana == compendium.arcana then
            local keep = show == "All"
            if show == "Carried" then
                keep = carried[id] == true
            elseif show ~= "All" then
                keep = bit32.btest(read("u16", COMPENDIUM + id * 0x30), 1) == (show == "Registered")
            end
            if keep then table.insert(out, id) end
        end
    end
    return out
end
local function record_text(record: PersonaRecord?): string
    if not record then return "none" end
    local stats = {}
    for i, value in record.stats do table.insert(stats, STAT_NAMES[i] .. " " .. tostring(value)) end
    local skills = {}
    for _, id in record.skills do table.insert(skills, skill_name(id)) end
    return string.format("Lv %d. %s.\nSkills: %s.", record.level, table.concat(stats, ", "),
        #skills > 0 and table.concat(skills, ", ") or "none")
end
local compendiumDetail = p4.ui.detail({
    key = "compendium.compare", title = "REGISTERED VS CARRIED",
    detail = function(): string
        local id = compendium.selected
        if id == 0 then return "Pick a Persona on COMPENDIUM FILTER." end
        local arcana, level = persona_base(id)
        local registered = persona_record(COMPENDIUM + id * 0x30)
        local carried = nil
        for _, entry in roster() do if entry.record.id == id then carried = entry.record end end
        local lines = {string.format("%s (%s, base Lv %d)", persona_name(id), ARCANA[arcana] or "?", level),
            "", "Registered: " .. record_text(registered), "", "Carried: " .. record_text(carried)}
        if registered and carried then
            local gained, lost = {}, {}
            local had = {}
            for _, s in registered.skills do had[s] = true end
            for _, s in carried.skills do if not had[s] then table.insert(gained, skill_name(s)) end end
            local has = {}
            for _, s in carried.skills do has[s] = true end
            for _, s in registered.skills do if not has[s] then table.insert(lost, skill_name(s)) end end
            table.insert(lines, "")
            table.insert(lines, string.format("Registering the carried one changes Lv %d to %d, adds %s and drops %s.",
                registered.level, carried.level, #gained > 0 and table.concat(gained, ", ") or "no skills",
                #lost > 0 and table.concat(lost, ", ") or "no skills"))
        end
        return table.concat(lines, "\n")
    end,
})
local compendiumPage: P4UIPage? = nil
compendiumPage = p4.ui.page({
    key = "compendium", title = "COMPENDIUM FILTER",
    opened = function() compendium.list = compendium_list() end,
    count = function(): number
        if not setting_cache.compendium_filter then return 1 end
        return 4 + #compendium.list
    end,
    row = function(index: number): P4UIRow
        if not setting_cache.compendium_filter then
            return {key = "off", label = "TURN ON COMPENDIUM FILTER", kind = "info"}
        end
        if index == 1 then
            return {key = "arcana", label = "ARCANA", kind = "value", value = ARCANA[compendium.arcana] or "All"}
        elseif index == 2 then
            return {key = "show", label = "SHOW", kind = "value", value = SHOW[compendium.show]}
        elseif index == 3 then
            return {key = "sort", label = "SORT", kind = "value", value = compendium.sortName and "Name" or "Level"}
        elseif index == 4 then
            return {key = "matches", label = string.format("%d PERSONAS", #compendium.list), kind = "info"}
        end
        local id = compendium.list[index - 4]
        local info = personas.info[id]
        local arcana, level = info.arcana, info.level
        local registered = bit32.btest(read("u16", COMPENDIUM + id * 0x30), 1)
        return {key = "p" .. tostring(id), label = string.format("Lv%d %s", level, persona_name(id)), kind = "action",
                value = registered and "REG" or "-", help = (ARCANA[arcana] or "?") .. ". Confirm: compare records."}
    end,
    find = function(key: string): number?
        if key == "arcana" then return 1 elseif key == "show" then return 2 elseif key == "sort" then return 3 end
        local id = tonumber(string.match(key, "^p(%d+)$"))
        if id then
            for i, entry in compendium.list do if entry == id then return 4 + i end end
        end
        return nil
    end,
    adjust = function(key: string, direction: number)
        local step = direction == 0 and 1 or direction
        if key == "arcana" then
            local choices = arcana_choices()
            local at = table.find(choices, compendium.arcana) or 1
            compendium.arcana = choices[(at - 1 + step) % #choices + 1]
        elseif key == "show" then
            compendium.show = (compendium.show - 1 + step) % #SHOW + 1
        elseif key == "sort" then
            compendium.sortName = not compendium.sortName
        else
            local id = tonumber(string.match(key, "^p(%d+)$"))
            if id and direction == 0 then
                compendium.selected = id
                touched.compendium_filter = true
                compendiumDetail:open()
            end
            return
        end
        compendium.list = compendium_list()
        touched.compendium_filter = true
        local page = compendiumPage
        if page then page:refresh() end
    end,
})

--------------------------------------------------------------------------
-- Q7 FUSION RESULTS: what the carried Personas fuse into, results first.
-- The game's own fusion calculators fill a mod-owned scratch record: pairs
-- through fclFusePair (003124a0) and triangles through fclFuseTriple
-- (003127e0) with each Persona as the first pick, as the fusion menu does.
-- Recomputed when the roster changes; fusing stays in the Velvet Room.
--------------------------------------------------------------------------

local FUSE_PAIR, FUSE_TRIPLE = 0x003124A0, 0x003127E0
type Fusion = {result: number, level: number, recipe: {number}, special: boolean, blocked: string?}
local fusion = {key = "", results = {} :: {Fusion}, scratch = nil :: P4GuestBuffer?, busy = false}
local function roster_key(entries: {{slot: number, record: PersonaRecord}}): string
    local ids = {}
    for _, entry in entries do table.insert(ids, tostring(entry.slot) .. ":" .. tostring(entry.record.id)) end
    return table.concat(ids, ",")
end
local function recompute_fusions()
    local entries = roster()
    local key = roster_key(entries)
    if key == fusion.key or fusion.busy then return end
    fusion.busy = true
    if not fusion.scratch then fusion.scratch = p4.mem.alloc(0x30) end
    local out = (fusion.scratch :: P4GuestBuffer).address
    local results = {}
    local tasks = {}
    for a = 1, #entries do
        for b = a + 1, #entries do table.insert(tasks, {a, b}) end
    end
    for first = 1, #entries do
        for b = 1, #entries do
            for c = b + 1, #entries do
                if b ~= first and c ~= first then table.insert(tasks, {first, b, c}) end
            end
        end
    end
    local next_ = 1
    schedule(function(): boolean
        local stop = math.min(#tasks, next_ + 23)
        for i = next_, stop do
            local task = tasks[i]
            local kind
            if #task == 2 then
                kind = p4.call(FUSE_PAIR, "ipHH", out, entries[task[1]].record.id, entries[task[2]].record.id) :: number
            else
                kind = p4.call(FUSE_TRIPLE, "ipppp", out, ROSTER + entries[task[1]].slot * 0x30,
                    ROSTER + entries[task[2]].slot * 0x30, ROSTER + entries[task[3]].slot * 0x30) :: number
            end
            local result = read("u16", out + 2)
            if kind ~= 0 and result ~= 0 then
                local recipe = {}
                for _, at in task do table.insert(recipe, entries[at].record.id) end
                table.insert(results, {result = result, level = read("u8", out + 4), recipe = recipe, special = kind == 2})
            end
        end
        next_ = stop + 1
        if next_ <= #tasks then return false end
        -- Results the protagonist can fuse now come first; the fusion menu
        -- refuses a level above his and a Persona he already carries.
        local hero = read("u8", HERO_LEVEL)
        local carried = {}
        for _, entry in entries do carried[entry.record.id] = true end
        for _, f in results do
            f.blocked = f.level > hero and string.format("Needs Lv %d.", f.level) or
                carried[f.result] and "Already carried." or nil
        end
        table.sort(results, function(x, y)
            if (x.blocked == nil) ~= (y.blocked == nil) then return x.blocked == nil end
            if x.level ~= y.level then return x.level > y.level end
            if x.result ~= y.result then return x.result < y.result end
            return #x.recipe < #y.recipe
        end)
        fusion.results, fusion.key, fusion.busy = results, key, false
        touched.fusion_results = true
        p4.log(string.format("QOL_FUSIONS roster=%d combinations=%d results=%d", #entries, #tasks, #results))
        return true
    end)
end
local function recipe_text(recipe: {number}): string
    local names = {}
    for _, id in recipe do table.insert(names, persona_name(id)) end
    return table.concat(names, " + ")
end
p4.ui.page({
    key = "fusion", title = "FUSION RESULTS",
    count = function(): number
        if not setting_cache.fusion_results then return 1 end
        return 1 + #fusion.results
    end,
    row = function(index: number): P4UIRow
        if not setting_cache.fusion_results then
            return {key = "off", label = "TURN ON FUSION RESULTS", kind = "info"}
        end
        if index == 1 then
            return {key = "summary", kind = "info", label = string.format("%d RESULTS", #fusion.results),
                    help = "From the Personas you carry. Fuse in the Velvet Room."}
        end
        local f = fusion.results[index - 1]
        return {key = string.format("f%d", index - 1), kind = "info",
                label = string.format("Lv%d %s", f.level, persona_name(f.result)),
                value = f.blocked and "LOCKED" or f.special and "SPECIAL" or (#f.recipe == 2 and "PAIR" or "TRIO"),
                help = recipe_text(f.recipe) .. (f.blocked and ". " .. (f.blocked :: string) or "")}
    end,
    find = function(key: string): number?
        if key == "summary" then return 1 end
        local i = tonumber(string.match(key, "^f(%d+)$"))
        return i and i <= #fusion.results and i + 1 or nil
    end,
})

--------------------------------------------------------------------------
-- N30 GLOSSARY: short definitions of the game's terms, written for this mod.
--------------------------------------------------------------------------

local GLOSSARY = {
    {"Persona", "A second self you call into battle. It sets your skills, affinities and stats. The protagonist can carry several and switch between them in battle; each party member has one."},
    {"Shadow", "The enemies inside the TV world. Touching one on the map starts a battle; striking it first gives your side the first move."},
    {"Arcana", "The tarot category of a Persona or a Social Link. Arcana decide fusion results, and a Persona fused under an arcana whose Social Link you have formed gains bonus EXP."},
    {"Social Link", "A bond with a person, ranked by spending time together. Each rank adds fusion EXP for its arcana; some unlock abilities."},
    {"HP and SP", "HP runs out and the character falls. SP pays for magic; physical skills cost a share of the user's max HP instead."},
    {"Affinities", "How a target takes an element: weak, normal, resist (reduced damage), null (no damage), repel (sent back) or absorb (heals it). Analyze shows what you have learned."},
    {"One More", "Hitting a weakness or landing a critical knocks the target down and gives the attacker another action."},
    {"Down and Dizzy", "A knocked-down unit loses its next action and takes more damage. Hitting it again can leave it dizzy, which keeps it from acting."},
    {"All-Out Attack", "When every foe is down, the party can rush them together for heavy damage that cannot miss."},
    {"Critical", "A strong physical hit that ignores some defenses and knocks the target down. Luck and skills raise the chance."},
    {"Guard", "Halves damage taken until your next turn and keeps you from being knocked down."},
    {"Ailments", "Statuses such as Fear, Rage, Silence, Poison and Confusion. They wear off over turns or are cured by skills and items."},
    {"Buffs", "The -kaja skills raise and the -nda skills lower attack, defense or hit and evasion for a few turns. Re-casting refreshes them; they do not stack."},
    {"Fusion", "Combine two or three Personas in the Velvet Room to create a new one. The result inherits some of its ingredients' skills."},
    {"Compendium", "The Velvet Room's record of Personas. Register a Persona to save its level and skills, then summon it again later for a fee."},
    {"Shuffle Time", "A card game after some battles. Picking a card can grant a Persona, money, EXP or a recovery."},
    {"Social stats", "Courage, Knowledge, Diligence, Understanding and Expression. Activities raise them, and some conversations and Links need a rank."},
    {"Free time", "Daytime and evening slots you choose how to spend: Links, part-time jobs, studying, or the TV world."},
}
local glossary = {selected = 1}
local glossaryDetail = p4.ui.detail({
    key = "glossary.entry", title = "GLOSSARY ENTRY",
    detail = function(): string
        local entry = GLOSSARY[glossary.selected]
        return entry[1] .. "\n\n" .. entry[2]
    end,
})
p4.ui.page({
    key = "glossary", title = "GLOSSARY",
    count = function(): number return setting_cache.glossary and #GLOSSARY or 1 end,
    row = function(index: number): P4UIRow
        if not setting_cache.glossary then return {key = "off", label = "TURN ON GLOSSARY", kind = "info"} end
        return {key = "g" .. tostring(index), label = string.upper(GLOSSARY[index][1]), kind = "action", value = "READ"}
    end,
    find = function(key: string): number?
        local i = tonumber(string.match(key, "^g(%d+)$"))
        return i and i <= #GLOSSARY and i or nil
    end,
    adjust = function(key: string, direction: number)
        local i = tonumber(string.match(key, "^g(%d+)$"))
        if i and direction == 0 then
            glossary.selected = i
            touched.glossary = true
            glossaryDetail:open()
        end
    end,
})

--------------------------------------------------------------------------
-- Settings: the native hub owns the rows; features follow them whenever a
-- menu closes (the only place they can change) and at startup.
--------------------------------------------------------------------------

local function setting(key: string): boolean
    return p4.settings.get(key) == true
end

local function refresh_settings()
    for _, key in {"skill_compare", "compendium_filter", "fusion_results", "glossary"} do
        setting_cache[key] = setting(key)
    end
end

local function refresh()
    refresh_settings()
    apply_descriptions(setting("full_descriptions"), p4.settings.get("description_style") :: string)
    apply_backlog(setting("backlog"))
    if setting_cache.fusion_results then recompute_fusions() end
    if setting_cache.skill_compare then compare_refresh() end
end

-- menu_close is the event to follow; builds whose Camp/Config seams are not
-- hookable refuse it, and then a check every 30 frames stands in for it.
if not pcall(function() p4.events.on("menu_close", function(_e) refresh() end) end) then
    local frames = 0
    p4.events.on("frame_start", function(_e)
        frames += 1
        if frames >= 30 then
            frames = 0
            refresh()
        end
    end)
end
p4.events.on("field_enter", function(_e) refresh() end)

--------------------------------------------------------------------------
-- Save provenance: a save written after a feature ran records which ones,
-- with this version and the description style.
--------------------------------------------------------------------------

p4.save.register({
    schema = 1,
    max_bytes = 1024,
    snapshot = function(): P4SaveValue
        local used = {}
        for name in touched do table.insert(used, name) end
        if #used == 0 then return nil end
        table.sort(used)
        return {version = VERSION, features = used, style = descriptions.style}
    end,
    load = function(_value: P4SaveValue) end,
})

refresh()
p4.log("QOL_READY version=" .. VERSION)
