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
local AILMENTS: {{bit: number, name: string}} = { -- status mask bits, in the order sentences name them
    {bit = 0x01, name = "Dizzy"}, {bit = 0x02, name = "Rage"}, {bit = 0x04, name = "Fear"},
    {bit = 0x08, name = "Silence"}, {bit = 0x10, name = "Confusion"}, {bit = 0x20, name = "Poison"},
    {bit = 0x40, name = "Exhaustion"}, {bit = 0x80, name = "Enervation"}, {bit = 0x100000, name = "Down"},
}

-- With an element mod active, p4.elements (API 1.5) knows each skill's effective
-- element: retail leaves keep the help text's short names and added ones use their
-- registry short name. Weapon-derived and unknown skills keep the game's own text.
local RETAIL_ELEMENT_KEYS = {["p4:physical"] = 0, ["p4:fire"] = 1, ["p4:ice"] = 2, ["p4:electric"] = 3,
    ["p4:wind"] = 4, ["p4:almighty"] = 5, ["p4:light"] = 6, ["p4:dark"] = 7}
local registry = (p4 :: any).elements
local function skill_element(id: number, retail: number): (number, string?)
    if registry == nil or not registry.active() then return retail, ELEMENT_NAMES[retail] end
    local info = registry.of_skill(id)
    if info == nil or info.weapon then return -1, nil end
    local leaf = RETAIL_ELEMENT_KEYS[info.key]
    if leaf then return leaf, ELEMENT_NAMES[leaf] end
    return -1, info.short
end

local function ailment_list(mask: number): string?
    if mask == 0xFF then return "all ailments" end
    local names = {}
    local left = mask
    for _, entry in AILMENTS do
        if bit32.btest(mask, entry.bit) then
            table.insert(names, entry.name)
            left = bit32.band(left, bit32.bnot(entry.bit))
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
    id: number, element: number, label: string?, damageClass: number, target: string?,
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
    local element, label = skill_element(id, read("u8", elements + id * 2))
    return {
        id = id, element = element, label = label, damageClass = u8(b, 0x02),
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
    local element = s.label
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
        local element = s.label
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
        local specs: {{slot: number, first: number, lines: (number, string, number) -> {string}?}} = {
            {slot = HELP_SKILL, first = 0, lines = skill_lines},
            {slot = HELP_ITEM, first = 0x300, lines = consumable_lines},
            {slot = HELP_WEAPON, first = 0x000, lines = function(id: number, _s: string, _r: number) return weapon_lines(id) end},
            {slot = HELP_ARMOR, first = 0x100, lines = function(id: number, _s: string, _r: number) return armor_lines(id) end},
            {slot = HELP_ACCESSORY, first = 0x200, lines = accessory_lines},
        }
        for _, spec in specs do
            local t = help_table(spec.slot, spec.first, spec.lines)
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
        p4.log(string.format("QOL_DESCRIPTIONS style=%s generated=%d kept=%d reference=%d tables=%d elements=%s",
            style, descriptions.generated, descriptions.kept, reference, #descriptions.tables,
            if registry ~= nil and registry.active() then "registry" else "retail"))
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
    table.insert(pending, {speaker = speaker_bytes(record, count, dialog), pages = copies, parts = {} :: {string}})
    if #pending > BACKLOG_LIMIT then table.remove(pending, 1) end
    touched.backlog = true
end

-- Q6 INHERITANCE's page, registered first for the same reason as PARTY HEAL's;
-- the Q6 section below lists the skills and applies the choice at fusion.
type InheritRow = {id: number, name: string, from: string}
local inherit = {on = false, rows = {} :: {InheritRow}, wanted = {} :: {number}, dirty = false, last = "",
    key = "", busy = false}
do
    local function wanted_rank(id: number): number?
        for i, wanted in inherit.wanted do if wanted == id then return i end end
        return nil
    end
    local inheritPage: P4UIPage
    inheritPage = p4.ui.page({
        key = "inherit", title = "INHERITANCE",
        count = function(): number
            if not inherit.on then return 1 end
            return 2 + #inherit.rows
        end,
        row = function(index: number): P4UIRow
            if not inherit.on then return {key = "off", label = "TURN ON CHOSEN INHERITANCE", kind = "info"} end
            if index == 1 then
                return {key = "last", kind = "info",
                    label = inherit.last ~= "" and inherit.last or "MARK THE SKILLS A FUSION SHOULD INHERIT",
                    help = "A marked skill the ingredients carry and the result can inherit takes the place of one the game picked at random, in the order marked. How many are inherited stays the game's."}
            end
            if index == 2 then
                return {key = "clear", kind = "action", label = "CLEAR MARKS", value = tostring(#inherit.wanted),
                    help = "Unmarks every skill."}
            end
            local row = inherit.rows[index - 2]
            local rank = wanted_rank(row.id)
            return {key = "k" .. tostring(row.id), kind = "action", label = row.name,
                value = rank and string.format("WANTED %d", rank) or "MARK", help = "Carried by " .. row.from .. "."}
        end,
        find = function(key: string): number?
            if key == "last" then return 1 end
            if key == "clear" then return 2 end
            local id = tonumber(string.match(key, "^k(%d+)$"))
            for i, row in inherit.rows do if row.id == id then return i + 2 end end
            return nil
        end,
        adjust = function(key: string, direction: number)
            if direction ~= 0 then return end
            if key == "clear" then
                inherit.wanted, inherit.dirty = {}, true
            else
                local id = tonumber(string.match(key, "^k(%d+)$"))
                if id then
                    local rank = wanted_rank(id)
                    if rank then table.remove(inherit.wanted, rank)
                    elseif #inherit.wanted < 16 then table.insert(inherit.wanted, id) end
                    inherit.dirty = true
                end
            end
            inheritPage:refresh()
        end,
    })
end

-- N21 RELEARN SKILLS' page, registered first for the same reason as PARTY
-- HEAL's; the N21 section below finds the skills and relearns them.
type RelearnSkill = {id: number, name: string, level: number, cost: string}
type RelearnMember = {id: number, label: string, skills: {number}, names: {string}, candidates: {RelearnSkill}}
local relearn = {on = false, members = {} :: {RelearnMember}, member = 0, pick = 0, replace = 1, pending = false,
    last = "", key = "", busy = false}
do
    local function relearn_current(): RelearnMember?
        return relearn.members[relearn.member + 1]
    end
    local function relearn_candidates_shown(): {RelearnSkill}
        local member = relearn_current()
        return member and member.candidates or {}
    end
    local relearnPage: P4UIPage
    relearnPage = p4.ui.page({
        key = "relearn", title = "RELEARN SKILLS",
        count = function(): number
            if not relearn.on then return 1 end
            return 4 + #relearn_candidates_shown()
        end,
        row = function(index: number): P4UIRow
            if not relearn.on then return {key = "off", label = "TURN ON RELEARN SKILLS", kind = "info"} end
            local member = relearn_current()
            local candidates = relearn_candidates_shown()
            if index == 1 then
                return {key = "member", kind = "value", label = "MEMBER",
                    value = member and member.label or "NOBODY HAS A FORGOTTEN SKILL",
                    help = "Left or right: each party member whose Persona has forgotten a skill it learned by its level."}
            end
            if index == 2 then
                local value = "-"
                if member then value = #member.skills < 8 and "EMPTY SLOT" or member.names[relearn.replace] or "-" end
                return {key = "replace", kind = "value", label = "REPLACES", value = value,
                    help = "Left or right: the skill the relearned one replaces when all eight slots are full."}
            end
            if index == 3 then
                return {key = "last", kind = "info", label = relearn.last ~= "" and relearn.last or "NOTHING RELEARNED YET"}
            end
            if index == 4 + #candidates then
                local skill = candidates[relearn.pick]
                return {key = "go", kind = "action", label = relearn.pending and "RELEARNS WHEN THIS MENU CLOSES" or "RELEARN",
                    value = relearn.pending and "QUEUED" or skill and skill.name or "PICK A SKILL",
                    help = "Queues the picked skill; the Persona relearns it when the menus close."}
            end
            local skill = candidates[index - 3]
            return {key = "c" .. tostring(skill.id), kind = "action", label = skill.name,
                value = string.format("%sLV %d  %s", relearn.pick == index - 3 and "> " or "", skill.level, skill.cost),
                help = "Learned at this level and since forgotten. Pick it, then RELEARN."}
        end,
        find = function(key: string): number?
            local candidates = relearn_candidates_shown()
            if key == "member" then return 1 end
            if key == "replace" then return 2 end
            if key == "last" then return 3 end
            if key == "go" then return 4 + #candidates end
            local id = tonumber(string.match(key, "^c(%d+)$"))
            for i, skill in candidates do if skill.id == id then return i + 3 end end
            return nil
        end,
        adjust = function(key: string, direction: number)
            local member = relearn_current()
            if key == "member" and direction ~= 0 and #relearn.members > 0 then
                relearn.member = (relearn.member + direction) % #relearn.members
                relearn.pick, relearn.replace, relearn.pending = 0, 1, false
            elseif key == "replace" and direction ~= 0 and member and #member.skills >= 8 then
                relearn.replace = (relearn.replace - 1 + direction) % #member.skills + 1
                relearn.pending = false
            elseif key == "go" and direction == 0 then
                relearn.pending = not relearn.pending and relearn.pick > 0
            elseif direction == 0 and member then
                local id = tonumber(string.match(key, "^c(%d+)$"))
                for i, skill in member.candidates do
                    if skill.id == id then relearn.pick, relearn.pending = i, false end
                end
            end
            relearnPage:refresh()
        end,
    })
end

-- N39 PLAN SUMMONS' page, registered first for the same reason as PARTY HEAL's;
-- the summoning is in the N39 section below.
type SummonRow = {plan: number, label: string, value: string, help: string}
local planner_rows: () -> {SummonRow} = function(): {SummonRow} return {} end
local summonQueue = {on = false, plan = 0, pending = false, last = ""}
local summonPage: P4UIPage
summonPage = p4.ui.page({
    key = "summons", title = "PLAN SUMMONS",
    count = function(): number return 2 + #planner_rows() end,
    row = function(index: number): P4UIRow
        local rows = planner_rows()
        if index == 1 then
            return {key = "info", kind = "info", label = #rows > 0 and "RECIPES WITH REGISTERED PERSONAS" or
                "PLAN A PERSONA ON FUSION PLANNER FIRST", help = "A queued recipe's Personas are summoned at the compendium's own price when you next reach the Velvet Room's menu."}
        end
        if index == 2 then
            return {key = "last", kind = "info", label = summonQueue.last ~= "" and summonQueue.last or "NOTHING SUMMONED YET"}
        end
        local row = rows[index - 2]
        local queued = summonQueue.pending and summonQueue.plan == row.plan
        return {key = "s" .. tostring(row.plan), kind = "action", label = row.label,
            value = queued and "QUEUED" or row.value, help = row.help}
    end,
    find = function(key: string): number?
        if key == "info" then return 1 end
        if key == "last" then return 2 end
        local n = tonumber(string.match(key, "^s(%d+)$"))
        if not n then return nil end
        for i, row in planner_rows() do if row.plan == n then return i + 2 end end
        return nil
    end,
    adjust = function(key: string, direction: number)
        local n = tonumber(string.match(key, "^s(%d+)$"))
        if n and direction == 0 then
            if summonQueue.pending and summonQueue.plan == n then
                summonQueue.pending = false
            else
                summonQueue.plan, summonQueue.pending = n, true
            end
            summonPage:refresh()
        end
    end,
})

-- Q13 PARTY HEAL's page is registered ahead of the others so the hub's other
-- page links keep their places; the PARTY HEAL section below fills `heal`.
type HealRow = {label: string, value: string}
local heal = {on = false, members = {} :: {string}, plan = {} :: {HealRow}, total = "", pending = false,
    last = "", field = false, previewed = false, finding = false}
local healPage: P4UIPage
healPage = p4.ui.page({
    key = "heal", title = "PARTY HEAL",
    count = function(): number
        if not heal.on then return 1 end
        return #heal.members + #heal.plan + 3
    end,
    row = function(index: number): P4UIRow
        if not heal.on then return {key = "off", label = "TURN ON PARTY HEAL", kind = "info"} end
        if index <= #heal.members then return {key = "m" .. tostring(index), label = heal.members[index], kind = "info"} end
        index -= #heal.members
        if index <= #heal.plan then
            local row = heal.plan[index]
            return {key = "p" .. tostring(index), label = row.label, value = row.value, kind = "info"}
        end
        index -= #heal.plan
        if index == 1 then return {key = "total", label = heal.total, kind = "info"} end
        if index == 2 then
            if #heal.plan == 0 then return {key = "heal", label = "NOTHING TO HEAL", kind = "info"} end
            return {key = "heal", label = heal.pending and "CASTS WHEN THIS MENU CLOSES" or "HEAL THE PARTY",
                value = heal.pending and "QUEUED" or "CAST", kind = "action"}
        end
        return {key = "last", label = heal.last ~= "" and heal.last or "NO HEAL CAST YET", kind = "info"}
    end,
    find = function(key: string): number?
        local m, p = #heal.members, #heal.plan
        local kind, n = string.match(key, "^([mp])(%d+)$")
        if kind == "m" then return tonumber(n) end
        if kind == "p" then return m + (tonumber(n) :: number) end
        if key == "total" then return m + p + 1 end
        if key == "heal" then return m + p + 2 end
        if key == "last" then return m + p + 3 end
        return nil
    end,
    adjust = function(key: string, direction: number)
        if key == "heal" and direction == 0 and #heal.plan > 0 then
            heal.pending = not heal.pending
            healPage:refresh()
        end
    end,
})

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
    local order: {number} = if compendium.sortName then personas.byName else personas.byLevel
    for _, id in order do
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
    local results: {Fusion} = {}
    local tasks: {{number}} = {}
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
                local recipe: {number} = {}
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
        table.sort(results, function(x: Fusion, y: Fusion): boolean
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
-- Q8 FUSION PLANNER: pick any Persona and see which pairs of the Personas
-- you carry or have registered fuse into it, through the same fclFusePair
-- the fusion menu uses. Read-only: nothing is summoned, bought or fused.
--------------------------------------------------------------------------

type Summon = {id: number, price: number}
type Plan = {recipe: {number}, registered: number, summons: {Summon}}
local planner = {target = 0, requested = 0, plans = {} :: {Plan}, busy = false, page = nil :: P4UIPage?}
-- N39: the compendium's own summon quote (fclCombineMenuPriceInfo, state 0x9e):
-- the square of the record's five buffed stats plus 2000, after the game's
-- fclCombineApplyDiscount. Native calls: frame jobs and events only.
local BUFFED_STAT, APPLY_DISCOUNT = 0x001099F0, 0x003026C0
local function summon_price(id: number): number
    local record, sum = COMPENDIUM + id * 0x30, 0
    for stat = 0, 4 do sum += p4.call(BUFFED_STAT, "Bpi", record, stat) :: number end
    return p4.call(APPLY_DISCOUNT, "iHi", id, sum * sum + 2000) :: number
end
local function plan_target(target: number)
    planner.busy = true
    local carried, ids = {}, {}
    for _, entry in roster() do
        carried[entry.record.id] = true
        table.insert(ids, entry.record.id)
    end
    for _, id in personas.byLevel do
        if not carried[id] and bit32.btest(read("u16", COMPENDIUM + id * 0x30), 1) then table.insert(ids, id) end
    end
    if not fusion.scratch then fusion.scratch = p4.mem.alloc(0x30) end
    local out = (fusion.scratch :: P4GuestBuffer).address
    local plans: {Plan} = {}
    local a, b = 1, 2
    schedule(function(): boolean
        for _ = 1, 64 do
            if a >= #ids then
                table.sort(plans, function(x: Plan, y: Plan): boolean return x.registered < y.registered end)
                for _, plan in plans do
                    for _, id in plan.recipe do
                        if not carried[id] then table.insert(plan.summons, {id = id, price = summon_price(id)}) end
                    end
                end
                planner.plans, planner.target, planner.busy = plans, target, false
                touched.fusion_planner = true
                p4.log(string.format("QOL_PLAN target=%d candidates=%d recipes=%d", target, #ids, #plans))
                local page = planner.page
                if page then page:refresh() end
                return true
            end
            if ids[a] ~= target and ids[b] ~= target and
                p4.call(FUSE_PAIR, "ipHH", out, ids[a], ids[b]) ~= 0 and read("u16", out + 2) == target then
                local registered = (carried[ids[a]] and 0 or 1) + (carried[ids[b]] and 0 or 1)
                table.insert(plans, {recipe = {ids[a], ids[b]}, registered = registered, summons = {} :: {Summon}})
            end
            b += 1
            if b > #ids then
                a += 1
                b = a + 1
            end
        end
        return false
    end)
end
local plannerDetail = p4.ui.detail({
    key = "planner.plan", title = "FUSION PLAN",
    detail = function(): string
        local target = planner.requested
        if target == 0 then return "Pick a Persona on FUSION PLANNER." end
        if planner.busy or planner.target ~= target then return persona_name(target) .. "\n\nWorking out the recipes..." end
        local lines = {persona_name(target) .. string.format(" (%s, base Lv %d)",
            ARCANA[personas.info[target].arcana] or "?", personas.info[target].level), ""}
        if #planner.plans == 0 then
            table.insert(lines, "No pair of the Personas you carry or have registered fuses into it.")
        end
        for i, plan in planner.plans do
            if i > 40 then
                table.insert(lines, string.format("...and %d more.", #planner.plans - 40))
                break
            end
            table.insert(lines, recipe_text(plan.recipe) .. (plan.registered == 0 and " (both carried)" or
                plan.registered == 1 and " (summon 1)" or " (summon 2)"))
        end
        return table.concat(lines, "\n")
    end,
})
planner.page = plannerDetail
p4.ui.page({
    key = "planner", title = "FUSION PLANNER",
    count = function(): number return setting_cache.fusion_planner and #personas.byLevel or 1 end,
    row = function(index: number): P4UIRow
        if not setting_cache.fusion_planner then
            return {key = "off", label = "TURN ON FUSION PLANNER", kind = "info"}
        end
        local id = personas.byLevel[index]
        local info = personas.info[id]
        return {key = "t" .. tostring(id), label = string.format("Lv%d %s", info.level, info.name), kind = "action",
                value = "PLAN", help = (ARCANA[info.arcana] or "?") .. ". Confirm: pairs that fuse into it."}
    end,
    find = function(key: string): number?
        local id = tonumber(string.match(key, "^t(%d+)$"))
        return id and table.find(personas.byLevel, id) or nil
    end,
    adjust = function(key: string, direction: number)
        local id = tonumber(string.match(key, "^t(%d+)$"))
        if id and direction == 0 then
            planner.requested = id
            -- Native calls wait for the next frame; this callback only queues them.
            if planner.target ~= id and not planner.busy then
                schedule(function(): boolean
                    if not planner.busy then plan_target(planner.requested) end
                    return true
                end)
            end
            plannerDetail:open()
        end
    end,
})

--------------------------------------------------------------------------
-- Q12 FORECAST: today and the next seven days as the game's own calendar
-- reports them. calGetPackedDayEntry (00110c50) is what the day-change
-- calendar draws: exact for three days, the game's forecast beyond. The low
-- nibble is the weather it shows, the high nibble the one before it. Codes
-- were matched to the field HUD's icons: 0 sun, 1 umbrella, 2 cloud.
--------------------------------------------------------------------------

local DAY_ENTRY, DATE_INDEX, SPLIT_DATE = 0x00110C50, 0x001060B0, 0x001104D0
local WEATHER = {[0] = "Sunny", "Rain", "Cloudy"}
local WEEKDAYS = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}
local forecast = {today = -1, rows = {} :: {string}, scratch = nil :: P4GuestBuffer?}
local function weather_name(code: number): string
    return WEATHER[code] or string.format("Weather %d", code)
end
local function forecast_refresh()
    local today = p4.call(DATE_INDEX, "h") :: number
    if today == forecast.today or today < 0 then return end
    if not forecast.scratch then forecast.scratch = p4.mem.alloc(8) end
    local out = (forecast.scratch :: P4GuestBuffer).address
    local rows = {}
    for day = today, today + 7 do
        local entry = p4.call(DAY_ENTRY, "iii", day, today) :: number
        p4.call(SPLIT_DATE, "vipp", day, out, out + 4)
        local month, date = read("i32", out), read("i32", out + 4)
        -- 04/01/2011, the calendar's day 0, was a Friday.
        local weekday = WEEKDAYS[(day + 5) % 7 + 1]
        local now, before = entry % 16, (entry // 16) % 16
        local text = before ~= now and weather_name(before) .. ", then " .. weather_name(now) or weather_name(now)
        table.insert(rows, string.format("%s %02d/%02d: %s", weekday, month, date, text))
    end
    forecast.today, forecast.rows = today, rows
    touched.forecast = true
    p4.log(string.format("QOL_FORECAST today=%d %s", today, table.concat(rows, "; ")))
end
p4.ui.page({
    key = "forecast", title = "FORECAST",
    count = function(): number return setting_cache.forecast and math.max(1, #forecast.rows) or 1 end,
    row = function(index: number): P4UIRow
        if not setting_cache.forecast then return {key = "off", label = "TURN ON FORECAST", kind = "info"} end
        local row = forecast.rows[index]
        if not row then return {key = "empty", label = "NO DATE YET", kind = "info"} end
        return {key = "d" .. tostring(index), label = (index == 1 and "Today, " or "") .. row, kind = "info"}
    end,
    find = function(key: string): number?
        local i = tonumber(string.match(key, "^d(%d+)$"))
        return i and i <= #forecast.rows and i or nil
    end,
})

--------------------------------------------------------------------------
-- Q9 AFFINITY HINTS: the target markers the navigator's Third Eye (skill
-- 0x10C) gives -- an attack will be reflected, or will have no effect --
-- from the first battle, for affinities the party has already discovered.
-- btlSelectionMarkTargets (0020b6d0) sets each target's row only when the
-- navigator has Third Eye; this runs the same test after it, through the
-- game's own queries, and never reveals an affinity not yet discovered.
--------------------------------------------------------------------------

local MARK_TARGETS, SET_ROW_STATE, SET_ROW_IDLE = 0x0020B6D0, 0x0020BFC0, 0x0020BFD0
local SKILL_ELEMENT, ELEMENT_BIT, ELEMENT_AFFINITY = 0x0023D8E0, 0x0023D6E0, 0x00242800
local KNOWLEDGE_BIT, GET_FLAG, BUFF_LEVEL, SKILL_PROPERTY = 0x0010F420, 0x00106330, 0x00233A90, 0x0010F8C0
local ENEMY_ROWS = 0x007644B4 -- UNIT.TBL enemy rows, 0x3C bytes, flags at +0
local hints = {on = false, evaluated = 0, marked = 0}

-- btlTargetTestUnitFlag (001f0950): some encounters force every affinity
-- known or unknown; otherwise the knowledge bitmap decides.
local function affinity_known(unit: number, element: number): boolean
    local rows = read("u32", ENEMY_ROWS)
    if rows ~= 0 and bit32.band(read("u16", rows + unit * 0x3C), 0x4004) == 0 and
        p4.call(GET_FLAG, "Ii", 0x1403) ~= 0 then
        if p4.call(GET_FLAG, "Ii", 0x1408) ~= 0 then return true end
        if p4.call(GET_FLAG, "Ii", 0x1409) ~= 0 then return false end
    end
    return p4.call(KNOWLEDGE_BIT, "III", unit, element) ~= 0
end
local function signed16(value: number): number
    return value >= 0x8000 and value - 0x10000 or value
end
local function hint_hook(context: P4HookContext)
    local targets, skill = context.args[3] :: number, context.args[4] :: number
    if skill < 0 or skill == 0x110 or skill == 0x10B then return end
    local special = p4.call(SKILL_PROPERTY, "Hi", skill) ~= 0
    for i = 0, math.min(read("u16", targets + 0x38), 12) - 1 do
        local unit = read("u32", read("u32", targets + i * 4) + 0x30)
        local data = read("u32", unit + 0xA64)
        hints.evaluated += 1
        local element = signed16(p4.call(SKILL_ELEMENT, "ipH", data, skill) :: number)
        local mask = p4.call(ELEMENT_BIT, "ih", element) :: number
        local state = 0
        if not bit32.btest(mask, 0xE0001) then
            if bit32.btest(mask, 2) then
                if (p4.call(BUFF_LEVEL, "bpi", data, 0x10) :: number) > 0 then state = 1 end
            elseif not bit32.btest(mask, 0x40) and (p4.call(BUFF_LEVEL, "bpi", data, 0x11) :: number) > 0 then
                state = 1
            end
        end
        if state == 0 then
            local affinity = p4.call(ELEMENT_AFFINITY, "ipi", data, element) :: number
            if affinity_known(read("u16", unit + 0xA4), bit32.band(element, 0xFFFF)) then
                if bit32.btest(affinity, 0x02000000) then state = 1
                elseif bit32.btest(affinity, 0x05000000) then state = 2 end
            end
        end
        if state ~= 0 then
            p4.call(SET_ROW_STATE, "vph", unit + 0xA48, state)
            if special then p4.call(SET_ROW_IDLE, "vp", unit + 0xA48) end
            hints.marked += 1
            touched.affinity_hints = true
            p4.log(string.format("QOL_HINT skill=%d unit=%d element=%d state=%d", skill,
                read("u16", unit + 0xA4), element, state))
        end
    end
end
local hintHook = p4.hook(MARK_TARGETS, "vipph", "after", hint_hook)
hintHook:disable()
-- One summary line per battle: how many targets were tested and marked.
local hintBattle = p4.events.on("battle_end", function(e)
    p4.log(string.format("QOL_HINTS battle_end outcome=%d evaluated=%d marked=%d", e.outcome, hints.evaluated, hints.marked))
    hints.evaluated, hints.marked = 0, 0
end)
hintBattle:disable()
local function apply_hints(on: boolean)
    if on == hints.on then return end
    hints.on = on
    if on then
        hintHook:enable()
        hintBattle:enable()
    else
        hintHook:disable()
        hintBattle:disable()
    end
end

--------------------------------------------------------------------------
-- N34 CAMERA SPEED: the free field camera's held turns (L1/R1 and the right
-- stick) scaled. Player control (FldEvent_StepMoveToward) applies them through
-- FldCameraSetDistance in the free camera modes 0/6/7 only; the camera task's
-- own easing and recentering call it from outside player control, and the
-- 45-degree turns are rotation tasks, so only held turns change. Fixed authored
-- cameras never reach the setter.
--------------------------------------------------------------------------

local STEP_MOVE = 0x00174E10   -- FldEvent_StepMoveToward(task)
local CAMERA_TURN = 0x0016E8E0 -- FldCameraSetDistance(cameraTask, degrees)
local camera = {on = false, inStep = false, percent = 100, turns = 0, game = 0, applied = 0}
local CAMERA_PERCENT = {["50%"] = 50, ["75%"] = 75, ["125%"] = 125, ["150%"] = 150, ["200%"] = 200}
local function turn_hook(c: P4HookContext)
    if not camera.inStep then return end
    local degrees = c.args[2] :: number
    if degrees == 0 then return end
    local scaled = degrees * camera.percent / 100
    c.args[2] = scaled
    touched.camera_speed = true
    camera.turns += 1
    camera.game += math.abs(degrees)
    camera.applied += math.abs(scaled)
    -- Witness line every 30 held frames: the game's own turn and the applied one.
    if camera.turns % 30 == 0 then
        p4.log(string.format("QOL_CAMERA percent=%d turns=%d game_degrees=%d applied_degrees=%d", camera.percent,
            camera.turns, camera.game // 1, camera.applied // 1))
    end
end
-- Player control is a port-overridden function; builds whose catalog cannot
-- hook it refuse the registration, and CAMERA SPEED then stays inert.
local cameraHooks: {P4Subscription}? = nil
local hooked, refusal = pcall(function()
    cameraHooks = {
        p4.hook(STEP_MOVE, "ip", "before", function(_c: P4HookContext) camera.inStep = true end),
        p4.hook(STEP_MOVE, "ip", "after", function(_c: P4HookContext) camera.inStep = false end),
        p4.hook(CAMERA_TURN, "vpf", "before", turn_hook),
    }
end)
if not hooked then p4.log("QOL_CAMERA unavailable: " .. tostring(refusal)) end
if cameraHooks then for _, hook in cameraHooks do hook:disable() end end
local function apply_camera(on: boolean, choice: string)
    camera.percent = CAMERA_PERCENT[choice] or 100
    local hooks = cameraHooks
    if on == camera.on or not hooks then return end
    camera.on = on
    camera.inStep = false
    for _, hook in hooks do
        if on then hook:enable() else hook:disable() end
    end
end

--------------------------------------------------------------------------
-- N33 AUDIO VOLUMES: music, effects, voice and movie volume through p4.audio.
-- The game streams ADX on four handles and every start names its file:
-- BGM: (BGM.CVM) is music, .AFS archives are spoken lines, and the other
-- streamed .ADX files (SOUND/EVENT, SOUND/BATTLE) are short sound effects.
-- Sample voices are effects and movie audio is movie without any tagging.
--------------------------------------------------------------------------

local ADXT_START = 0x004D40A0 -- adxt_start_stm(adxt, fname, dir, offset, length)
local ADXT_HANDLES = 0x007188C0 -- the game's ADXT handles, 0xC8 bytes apart
-- The handles in the order the game's sound channels create them: BGM, streamed
-- effects, then two voice channels (battle navigator and members, events).
local HANDLE_ROLES = {"music", "effects", "voice", "voice"}
local VOLUME_KEYS = {music = "music_volume", effects = "effects_volume", voice = "voice_volume", movie = "movie_volume"}
local mixer = (p4 :: any).audio
local sound = {on = false, tagged = 0}
local function stream_category(name: string): string
    if string.sub(name, 1, 4) == "BGM:" then return "music" end
    if string.upper(string.sub(name, -4)) == ".AFS" then return "voice" end
    return "effects"
end
local function stream_name(address: number): string
    if address == 0 then return "" end
    local text = buffer.tostring(p4.mem.read_bytes(address, 48))
    return string.match(text, "^[^%z]*") :: string
end
local audioHook: P4Subscription? = nil
if mixer ~= nil then
    audioHook = p4.hook(ADXT_START, "vpppii", "before", function(c: P4HookContext)
        local category = stream_category(stream_name(c.args[2] :: number))
        if mixer.tag_adxt(c.args[1] :: number, category) then sound.tagged += 1 end
    end)
    audioHook:disable()
end
local function apply_audio(on: boolean)
    local hook = audioHook
    if mixer == nil or hook == nil then return end
    if on then
        local levels = {}
        for _, category in {"music", "effects", "voice", "movie"} do
            local percent = tonumber(string.match(p4.settings.get(VOLUME_KEYS[category]) :: string, "^(%d+)%%$")) or 100
            mixer.set_volume(category, percent)
            table.insert(levels, string.format("%s=%d", category, percent))
        end
        if not sound.on then
            -- Streams already playing were started before the hook: tag them by handle.
            for index, category in HANDLE_ROLES do mixer.tag_adxt(ADXT_HANDLES + (index - 1) * 0xC8, category) end
            hook:enable()
            touched.audio_volumes = true
        end
        p4.log("QOL_AUDIO " .. table.concat(levels, " "))
    elseif sound.on then
        hook:disable()
        for category in VOLUME_KEYS do mixer.set_volume(category, 100) end
        p4.log("QOL_AUDIO off")
    end
    sound.on = on
end

--------------------------------------------------------------------------
-- Q1 FAST TEXT. The game already fast-forwards dialogue while Triangle is held
-- (itfMesWindowAdvanceRow: a revealed row advances once it has waited five
-- frames). This adds a START latch that advances each revealed row by itself,
-- and drops the five-frame pause under the held Triangle. Both go through the
-- game's own row state machine: a row it marks as waiting for input (row
-- flags +0x25) is let through exactly as its own no-wait rows are. Windows
-- whose input is locked (flags 0x4000000) and choices are never touched, and
-- no button is ever synthesized, so nothing leaks into the next choice.
--------------------------------------------------------------------------

local ADVANCE_ROW = 0x0027A970 -- itfMesWindowAdvanceRow(window)
local ADXT_GET_STAT = 0x004D4430 -- ADXT_GetStat(adxt): 1..4 while a stream is decoding or playing
-- The handles whose current stream is a spoken line: the last start on them
-- named an .AFS voice archive (the field also loops ambience on the same
-- channels, which must not hold dialogue forever).
local voiceHandles: {[number]: boolean} = {}
local ROW_STATE, ROW_FLAGS, ROW_INDEX, ROW_ELAPSED = 0x24, 0x25, 0x2C, 0x30
local AUTO_FRAMES = {["1s"] = 60, ["2s"] = 120, ["3s"] = 180, ["5s"] = 300}
local text = {on = false, fast = false, auto = false, delay = 120, latched = false, advanced = 0, window = 0,
    index = -1, ours = false, frame = 0, seen = -10, fields = false}
-- AUTO DIALOGUE: per window, the waiting row and the frame it was first seen.
local waiting: {[number]: {index: number, frame: number}} = {}
-- START is read once a frame, and only while dialogue was on screen in the last
-- frames after the first field of the session (the title and load screens use
-- START and show message windows of their own).
local textFrame = p4.events.on("frame_start", function(_e)
    text.frame += 1
    if not text.fast then return end
    local _, pressed = p4.input.action("start")
    if pressed and text.fields and text.frame - text.seen <= 2 then
        text.latched = not text.latched
        p4.log("QOL_FAST_TEXT latch=" .. (text.latched and "on" or "off"))
    end
end)
textFrame:disable()
local function voice_playing(): boolean
    for handle, voice in voiceHandles do
        if voice then
            local stat = p4.call(ADXT_GET_STAT, "ip", handle) :: number
            if stat >= 1 and stat <= 4 then return true end
            voiceHandles[handle] = false
        end
    end
    return false
end
local voiceStart = p4.hook(ADXT_START, "vpppii", "before", function(c: P4HookContext)
    voiceHandles[c.args[1] :: number] = stream_category(stream_name(c.args[2] :: number)) == "voice"
end)
voiceStart:disable()
local function let_through(window: number)
    if read("u8", window + ROW_FLAGS) ~= 0 then
        p4.mem.write("u8", window + ROW_FLAGS, 0)
        text.ours = true
    end
end
local function text_before(c: P4HookContext)
    local window = c.args[1] :: number
    text.seen = text.frame
    local flags = read("u32", window)
    -- A choice ends the latch: the player answers it and starts again if wanted.
    if text.latched and bit32.btest(flags, 0xC00) then
        text.latched = false
        p4.log("QOL_FAST_TEXT latch=off choice")
    end
    -- Only a revealed row waiting in a window that takes input.
    if read("u8", window + ROW_STATE) ~= 1 or bit32.band(flags, 7) ~= 4 or bit32.btest(flags, 0x4000000) then
        return
    end
    local index = read("u16", window + ROW_INDEX)
    text.window, text.index, text.ours = window, index, false
    if text.latched then
        let_through(window)
    elseif text.fast and (p4.input.action("triangle")) then
        if read("u32", window + ROW_ELAPSED) < 5 then
            p4.mem.write("u32", window + ROW_ELAPSED, 5)
            text.ours = true
        end
    elseif text.auto then
        -- AUTO DIALOGUE: the reading delay runs from when this row was first
        -- seen waiting, and a voice line still playing always finishes first.
        local row = waiting[window]
        if row == nil or row.index ~= index then
            waiting[window] = {index = index, frame = text.frame}
        elseif text.frame - row.frame >= text.delay and not voice_playing() then
            let_through(window)
        end
    end
end
local function text_after(c: P4HookContext)
    local window = c.args[1] :: number
    if text.ours and window == text.window and read("u16", window + ROW_INDEX) ~= text.index then
        waiting[window] = nil
        text.advanced += 1
        if text.fast then touched.fast_text = true end
        if text.auto then touched.auto_dialogue = true end
        if text.advanced <= 10 or text.advanced % 50 == 0 then
            p4.log(string.format("QOL_TEXT advanced=%d latched=%s auto=%s frame=%d", text.advanced,
                tostring(text.latched), tostring(text.auto), text.frame))
        end
    end
    text.window, text.index, text.ours = 0, -1, false
end
local textBefore = p4.hook(ADVANCE_ROW, "vp", "before", text_before)
local textAfter = p4.hook(ADVANCE_ROW, "vp", "after", text_after)
textBefore:disable()
textAfter:disable()
-- FAST TEXT and AUTO DIALOGUE share the row hooks; both off removes them.
local function apply_text(fast: boolean, auto: boolean, delay: string)
    text.delay = AUTO_FRAMES[delay] or 120
    if not fast then text.latched = false end
    text.fast, text.auto = fast, auto
    if auto then voiceStart:enable() else voiceStart:disable() end
    local on = fast or auto
    if on == text.on then return end
    text.on = on
    if on then
        textFrame:enable()
        textBefore:enable()
        textAfter:enable()
    else
        textAfter:disable()
        textBefore:disable()
        textFrame:disable()
    end
end

--------------------------------------------------------------------------
-- Q4 SKIP SEEN MOVIES. The game's movie task (movieNativeTaskUpdate) already
-- skips any movie but the staff roll on START by moving to its end state 6;
-- the fade, cleanup and script continuation after that are the task's own.
-- With the feature on, a movie watched to its natural end before is moved to
-- state 6 the same way as soon as it plays. Watched movies are kept in the
-- mod's settings file by table index and SFD path; a movie ended by a button,
-- by this feature or in its first second never counts as watched.
--------------------------------------------------------------------------

local MOVIE_TASK = 0x00468FF0 -- movieNativeTaskUpdate(context, work)
local MOVIE_PATHS = 0x00712C54 -- the 0x28-byte movie definitions' SFD path pointers
local STAFF_ROLL = 0x11 -- its frame numbers drive the credit scroll; never skipped
local MOVIE_STATE, MOVIE_INDEX = 0x1E4, 0x1EA
local SKIP_BUTTONS = {"start", "cross", "circle", "triangle", "square", "l1", "r1", "l2", "r2", "select"}
local movies = {on = false}
local movieWorks: {[number]: {state: number, frames: number, pressed: boolean, skipped: boolean}} = {}
local function movie_path(index: number): string
    local address = read("u32", MOVIE_PATHS + index * 0x28)
    if address == 0 then return "" end
    return string.match(buffer.tostring(p4.mem.read_bytes(address, 40)), "^[^%z]*") :: string
end
local function movie_key(index: number): string
    return "movie." .. tostring(index)
end
local movieBefore = p4.hook(MOVIE_TASK, "vpp", "before", function(c: P4HookContext)
    local work = c.args[2] :: number
    local state, index = read("i16", work + MOVIE_STATE), read("i16", work + MOVIE_INDEX)
    local entry = movieWorks[work]
    if entry == nil then
        entry = {state = state, frames = 0, pressed = false, skipped = false}
        movieWorks[work] = entry
    end
    if state >= 3 and state <= 5 then
        for _, name in SKIP_BUTTONS do
            local _, pressed = p4.input.action(name :: any)
            if pressed then entry.pressed = true end
        end
    end
    if state == 4 and index ~= STAFF_ROLL and not entry.skipped then
        local path = movie_path(index)
        if path ~= "" and p4.settings.get(movie_key(index)) == path then
            p4.mem.write("i16", work + MOVIE_STATE, 6)
            entry.skipped = true
            touched.skip_seen_movies = true
            p4.log(string.format("QOL_MOVIE skip index=%d path=%s", index, path))
        end
    end
end)
local movieAfter = p4.hook(MOVIE_TASK, "vpp", "after", function(c: P4HookContext)
    local work = c.args[2] :: number
    local entry = movieWorks[work]
    if entry == nil then return end
    local state, index = read("i16", work + MOVIE_STATE), read("i16", work + MOVIE_INDEX)
    if state == 4 or state == 5 then entry.frames += 1 end
    if state == 6 then
        -- A natural end: playing ran out (or faded out) with no button, no
        -- skip of ours and more than a second of picture.
        if (entry.state == 4 or entry.state == 5) and not entry.pressed and not entry.skipped and entry.frames > 60 then
            local path = movie_path(index)
            if path ~= "" and p4.settings.get(movie_key(index)) ~= path then
                local stored, why = p4.settings.set(movie_key(index), path)
                p4.log(string.format("QOL_MOVIE seen index=%d path=%s frames=%d stored=%s%s", index, path,
                    entry.frames, tostring(stored), why and (" " .. why) or ""))
            end
        end
        movieWorks[work] = nil
        return
    end
    entry.state = state
end)
movieBefore:disable()
movieAfter:disable()
local function apply_movies(on: boolean)
    if on == movies.on then return end
    movies.on = on
    table.clear(movieWorks)
    if on then
        movieBefore:enable()
        movieAfter:enable()
    else
        movieAfter:disable()
        movieBefore:disable()
    end
end

--------------------------------------------------------------------------
-- Q13 PARTY HEAL: one confirmed action that casts the party's own healing
-- skills the way the Camp skill menu does (cmpSkillApplySelected 0013f720):
-- the applicability test, the effect and the SP deduction are the game's
-- routines, and the protagonist's other Personas are lent for a cast and
-- given back, as the Camp menu does. Policy, recomputed after every cast:
-- lowest SP cost first, then the caster's party order, then skill ID; a
-- single-target heal goes to the member missing the most HP. The fallen are
-- never targets (no revival), a cast that heals nothing is never repeated,
-- and casting happens only when the menu closes, in the field.
--------------------------------------------------------------------------

local PARTY_IDS = 0x00353B50 -- datBuildPartyIdList(out s16[4]) -> count
local GET_HP, MAX_HP, GET_SP, MAX_SP = 0x00104CE0, 0x00104DC0, 0x00104D50, 0x00104E30
local SET_SP = 0x00105730 -- datSetUnitSp(id, sp)
local SKILL_COST = 0x0010F600 -- datCalcSkillCostById(id, skill)
local EFFECT_FLAGS = 0x0010F930 -- datCalcSkillEffectFlagsById(source, target, skill, 0): 0 = applies
local APPLY_SKILL = 0x0010F770 -- datCalcApplySkillEffectById(source, target, skill, 0)
local GET_LEAD, SET_LEAD = 0x0010B510, 0x0010B3B0 -- the protagonist's equipped roster slot
local HEAL_CASTS = 20
local healScratch = p4.mem.alloc(16)
type Healer = {id: number, order: number, slot: number, skill: number, cost: number, all: boolean, name: string}
type Member = {id: number, order: number, hp: number, maxHp: number, sp: number}

-- The healing skills (HP only, no status, revival or buff part; one ally or
-- the whole party) with their kind and power, found once in the background:
-- menu callbacks then only look them up.
local healSkills: {[number]: {all: boolean, kind: number, power: number}}? = nil
local function find_heal_skills()
    local found: {[number]: {all: boolean, kind: number, power: number}} = {}
    local next = 1
    schedule(function(): boolean
        if read("u32", LIVE_SKILLS) == 0 then return false end
        for id = next, math.min(next + 39, ACTIVE_SKILLS - 1) do
            local s = read_skill(id)
            if s and s.statusMode == 0 and s.special == 0 and s.buffMask == 0 and s.spKind == 0 and
                (s.hpKind == 2 or s.hpKind == 5 or s.hpKind == 11) and (s.target == "1 ally" or s.target == "all allies") then
                found[id] = {all = s.target == "all allies", kind = s.hpKind, power = s.hpPower}
            end
        end
        next += 40
        if next < ACTIVE_SKILLS then return false end
        healSkills = found
        return true
    end)
end
local function party(): {Member}
    local count = p4.call(PARTY_IDS, "hp", healScratch.address) :: number
    local out = {}
    for i = 0, math.min(count, 4) - 1 do
        local id = read("i16", healScratch.address + i * 2)
        table.insert(out, {id = id, order = i, hp = p4.call(GET_HP, "Ih", id) :: number,
            maxHp = p4.call(MAX_HP, "Hh", id) :: number, sp = p4.call(GET_SP, "Ih", id) :: number})
    end
    return out
end
-- Every healing skill a living member could cast now: the protagonist's from
-- each carried Persona (its cost with that Persona equipped), the others' from
-- their own Persona.
local function healers(members: {Member}): {Healer}
    local out = {}
    local lead = p4.call(GET_LEAD, "i") :: number
    for _, member in members do
        if member.hp > 0 then
            local sources = {}
            if member.id == 1 then
                for _, entry in roster() do table.insert(sources, {slot = entry.slot, record = entry.record}) end
            else
                local record = persona_record(PARTY_PERSONA + member.id * 0x88)
                if record then table.insert(sources, {slot = -1, record = record}) end
            end
            for _, source in sources do
                if source.slot >= 0 then p4.call(SET_LEAD, "ii", source.slot) end
                for _, skill in source.record.skills do
                    local info = (healSkills :: any)[skill]
                    if info then
                        local cost = p4.call(SKILL_COST, "Ihi", member.id, skill) :: number
                        table.insert(out, {id = member.id, order = member.order, slot = source.slot, skill = skill,
                            cost = cost, all = info.all, name = persona_name(source.record.id)})
                    end
                end
            end
        end
    end
    p4.call(SET_LEAD, "ii", lead)
    return out
end
local function choose(members: {Member}, casters: {Healer}, spent: {[number]: number}, useless: {[string]: boolean}):
    (Healer?, Member?)
    local target: Member? = nil
    local wounded = false
    for _, member in members do
        if member.hp > 0 and member.hp < member.maxHp then
            wounded = true
            if not target or member.maxHp - member.hp > target.maxHp - target.hp then target = member end
        end
    end
    if not wounded then return nil, nil end
    local best: Healer? = nil
    for _, caster in casters do
        local sp = 0
        for _, member in members do if member.id == caster.id then sp = member.sp - (spent[caster.id] or 0) end end
        local key = string.format("%d:%d:%d", caster.id, caster.skill, caster.all and 0 or (target :: Member).id)
        if caster.cost > 0 and caster.cost <= sp and not useless[key] then
            if not best or caster.cost < best.cost or caster.cost == best.cost and (caster.order < best.order or
                caster.order == best.order and caster.skill < best.skill) then
                best = caster
            end
        end
    end
    return best, target
end
-- Party member unit IDs, matched to their Personas in a June save (2 Jiraiya,
-- 3 Tomoe, 4 Konohana Sakuya, 5 Himiko, 6 Take-Mikazuchi, 7 Sukuna-Hikona,
-- 8 Kintoki-Douji); 1 is the protagonist.
local MEMBER_NAMES = {"LEADER", "YOSUKE", "CHIE", "YUKIKO", "RISE", "KANJI", "NAOTO", "TEDDIE"}
local function member_name(id: number): string
    return MEMBER_NAMES[id] or "MEMBER " .. tostring(id)
end
-- The preview: the same policy over estimated amounts (fixed heals exactly,
-- percentage heals of max HP, Magic-scaled heals at their base power), so it
-- never calls the game's outcome routine and never touches its random numbers.
local function heal_preview()
    if not heal.field or not healSkills then
        heal.members, heal.plan = {}, {}
        heal.total = healSkills and "ONLY IN THE FIELD" or "STILL READING THE SKILL TABLE"
        return
    end
    local members = party()
    local casters = healers(members)
    heal.members = {}
    for _, member in members do
        table.insert(heal.members, string.format("%s  HP %d/%d  SP %d", member_name(member.id), member.hp, member.maxHp,
            member.sp))
    end
    local plan: {HealRow} = {}
    local spent: {[number]: number} = {}
    local total = 0
    for _ = 1, HEAL_CASTS do
        local caster, target = choose(members, casters, spent, {})
        if not caster or not target then break end
        local info = (healSkills :: any)[caster.skill]
        local targets = caster.all and members or {target}
        for _, member in targets do
            if member.hp > 0 then
                local amount = info.kind == 11 and member.maxHp * info.power // 100 or info.power
                member.hp = math.min(member.maxHp, member.hp + amount)
            end
        end
        spent[caster.id] = (spent[caster.id] or 0) + caster.cost
        total += caster.cost
        table.insert(plan, {label = string.format("%s (%s) > %s", skill_name(caster.skill), caster.name,
            caster.all and "PARTY" or member_name(target.id)), value = string.format("%d SP", caster.cost)})
    end
    heal.plan = plan
    heal.total = #plan > 0 and string.format("ABOUT %d CASTS, %d SP", #plan, total) or "NO HEALING NEEDED OR AVAILABLE"
end
-- Casting runs one cast per frame in the background (each step reads the party
-- again), so no callback exceeds its budget and each cast is the game's own.
local function heal_cast()
    heal.pending = false
    if not heal.field or not healSkills then
        heal.last = "NOT CAST: ONLY IN THE FIELD"
        return
    end
    local useless: {[string]: boolean} = {}
    local casts, sp, healed, steps = 0, 0, 0, 0
    local function finish()
        heal.last = string.format("LAST: %d CASTS, %d SP, %d HP", casts, sp, healed)
        p4.log(string.format("QOL_HEAL done casts=%d sp=%d healed=%d", casts, sp, healed))
    end
    schedule(function(): boolean
        steps += 1
        if steps > HEAL_CASTS or not heal.field then
            finish()
            return true
        end
        local members = party()
        local caster, target = choose(members, healers(members), {}, useless)
        if not caster or not target then
            finish()
            return true
        end
        local lead = p4.call(GET_LEAD, "i") :: number
        if caster.slot >= 0 then p4.call(SET_LEAD, "ii", caster.slot) end
        local before = 0
        for _, member in members do before += member.hp end
        local applied = 0
        for _, member in (caster.all and members or {target}) do
            if member.hp > 0 and (p4.call(EFFECT_FLAGS, "ihhII", caster.id, member.id, caster.skill, 0) :: number) == 0 then
                p4.call(APPLY_SKILL, "vhhII", caster.id, member.id, caster.skill, 0)
                applied += 1
            end
        end
        local after = 0
        for _, member in members do after += p4.call(GET_HP, "Ih", member.id) :: number end
        if applied > 0 then
            local left = (p4.call(GET_SP, "Ih", caster.id) :: number) - caster.cost
            p4.call(SET_SP, "vhh", caster.id, left)
            casts += 1
            sp += caster.cost
            healed += after - before
            touched.party_heal = true
            p4.log(string.format("QOL_HEAL cast=%s caster=%d target=%s sp=%d healed=%d", skill_name(caster.skill),
                caster.id, caster.all and "party" or tostring(target.id), caster.cost, after - before))
        end
        p4.call(SET_LEAD, "ii", lead)
        if applied == 0 or after == before then
            useless[string.format("%d:%d:%d", caster.id, caster.skill, caster.all and 0 or target.id)] = true
        end
        return false
    end)
end
-- The preview is made when a menu opens (menu_open) and casting waits for it
-- to close (menu_close). Builds without those events check every 30 frames
-- whether a native Config page is up instead.
local function apply_heal(on: boolean, closing: boolean)
    heal.on = on
    if on and healSkills == nil and not heal.finding then
        heal.finding = true
        find_heal_skills()
    end
    if not on then
        heal.pending, heal.members, heal.plan, heal.total, heal.previewed = false, {}, {}, "", false
        return
    end
    local open = not closing and p4.ui.active() ~= nil
    if heal.pending and not open then heal_cast() end
    if open and not heal.previewed then
        heal_preview()
        heal.previewed = true
    elseif not open then
        heal.previewed = false
    end
end

--------------------------------------------------------------------------
-- N18 DIRECT COMMANDS and N16 WEAKNESS ASSIST, through p4.battle (battle.menu@1):
-- both act only on the party member's own command panel, while it waits.
-- A direct command button opens Guard, Item or Persona exactly as choosing it
-- on the circle and confirming would (the game checks it can be used). The
-- assist button opens Skill with the cursor on the first usable skill that
-- hits a weakness the party has already discovered, and that enemy as the
-- target selection's starting point; the player still confirms both, or backs
-- out. Nothing new is revealed: the test is Q9's discovered-affinity test.
--------------------------------------------------------------------------

local THRESHOLD_TRIGGER = 0x0023DDC0 -- datCalcThresholdTrigger(data, skill): 0 when usable now
local battleApi = (p4 :: any).battle
local BUTTONS = {Square = "square", L2 = "l2", R2 = "r2", R1 = "r1"}
local DIRECT: {{string}} = {{"guard_button", "guard"}, {"item_button", "item"}, {"persona_button", "persona"}}
local direct = {on = false, assist = false, buttons = {} :: {[string]: string}, assistButton = "",
    pending = false, checkAction = 0, checkFrames = 0}
type BattleUnits = {party: {P4BattleUnit}, enemies: {P4BattleUnit}}
local function weakness_pick(menu: P4BattleMenu): (number?, number?, number?)
    local units: BattleUnits? = battleApi.units()
    if not units then return nil, nil, nil end
    local data = read("u32", menu.unit + 0xA64)
    for row, skill in menu.skills or {} do
        if skill > 0 and skill < ACTIVE_SKILLS and (p4.call(THRESHOLD_TRIGGER, "ipi", data, skill) :: number) == 0 then
            local element = signed16(p4.call(SKILL_ELEMENT, "ipH", data, skill) :: number)
            if element >= 1 and element <= 7 then
                for _, enemy in units.enemies do
                    local affinity = p4.call(ELEMENT_AFFINITY, "ipi", enemy.data, element) :: number
                    if bit32.btest(affinity, 0x08000000) and affinity_known(enemy.id, element) then
                        return row, skill, enemy.unit
                    end
                end
            end
        end
    end
    return nil, nil, nil
end
local battleFrame = p4.events.on("frame_start", function(_e)
    -- A few frames after a direct command, the action's command kind (the
    -- game's own: 7 Guard, 3 Item list, 5 Persona list) for the witness log.
    if direct.checkAction ~= 0 then
        direct.checkFrames -= 1
        if direct.checkFrames <= 0 then
            p4.log(string.format("QOL_COMMAND action kind=%d", read("u16", direct.checkAction + 0x6C)))
            direct.checkAction = 0
        end
    end
    local menu: P4BattleMenu? = battleApi.menu()
    if not menu then return end
    if menu.panel == "commands" then
        for button, command in direct.buttons do
            local _, pressed = p4.input.action(button :: any)
            if pressed then
                local ok: boolean, why: string? = battleApi.choose(command)
                touched.direct_commands = true
                if ok then direct.checkAction, direct.checkFrames = menu.action, 3 end
                p4.log(string.format("QOL_COMMAND %s unit=%d ok=%s%s", command, menu.id, tostring(ok),
                    why and (" " .. why) or ""))
                return
            end
        end
        if direct.assist and direct.assistButton ~= "" then
            local _, pressed = p4.input.action(direct.assistButton :: any)
            if pressed then
                local row, skill = weakness_pick(menu)
                if row then
                    direct.pending = battleApi.choose("skill")
                    p4.log(string.format("QOL_ASSIST skill=%s unit=%d", skill_name(skill :: number), menu.id))
                else
                    p4.log(string.format("QOL_ASSIST none unit=%d", menu.id))
                end
            end
        end
    elseif menu.panel == "skills" and direct.pending then
        direct.pending = false
        local row, skill, enemy = weakness_pick(menu)
        if row then
            local pointed = battleApi.point(row)
            local aimed = battleApi.aim(enemy)
            touched.weakness_assist = true
            p4.log(string.format("QOL_ASSIST point=%d skill=%s aimed=%s pointed=%s", row, skill_name(skill :: number),
                tostring(aimed), tostring(pointed)))
        end
    end
end)
battleFrame:disable()
-- The target selection the confirmed skill starts (btlActionTargetInit): which
-- skill and which unit it starts on, for the witness.
local TARGET_INIT = 0x001A1C80
local targetInit = p4.hook(TARGET_INIT, "vp", "after", function(c: P4HookContext)
    local action = c.args[1] :: number
    local skill = read("u16", action + 0x6E)
    local selected = read("u16", action + 0xD2)
    local entry = selected < 12 and read("u32", action + 0x98 + selected * 4) or 0
    local unit = entry ~= 0 and read("u32", entry + 0x30) or 0
    p4.log(string.format("QOL_ASSIST targeting skill=%s target=%d", skill_name(skill),
        unit ~= 0 and read("u16", unit + 0xA4) or -1))
end)
targetInit:disable()
local function apply_battle(directOn: boolean, assistOn: boolean)
    if battleApi == nil then return end
    direct.buttons = {}
    if directOn then
        for _, entry in DIRECT do
            local button = BUTTONS[p4.settings.get(entry[1]) :: string]
            if button and direct.buttons[button] == nil then direct.buttons[button] = entry[2] end
        end
    end
    direct.assist = assistOn
    direct.assistButton = BUTTONS[p4.settings.get("assist_button") :: string] or ""
    if direct.buttons[direct.assistButton] then direct.assistButton = "" end
    if assistOn then targetInit:enable() else targetInit:disable() end
    local on = directOn or assistOn
    if on == direct.on then return end
    direct.on, direct.pending = on, false
    if on then battleFrame:enable() else battleFrame:disable() end
end

--------------------------------------------------------------------------
-- Q11a HALF SP COST and Q11b DOUBLE BATTLE MONEY (balance assistance).
-- HALF SP: datCalcSkillCost (0023d9b0) is the one cost every party SP check,
-- list and deduction reads (battle and Camp); for a party member's SP-cost
-- skill its result becomes ceil(cost / 2), at least 1, after the game's own
-- equipment and passive changes. HP costs, enemies and items keep theirs.
-- DOUBLE MONEY: btlXpApplyAwards (0021e9a0) commits a victory's money once
-- through datAddMoney (00106020), which clamps to the game's own limit; while
-- it runs that one award is doubled. Script income, shops and refunds, which
-- reach datAddMoney elsewhere, are untouched.
--------------------------------------------------------------------------

local SKILL_COST_CALC = 0x0023D9B0 -- datCalcSkillCost(unit, skill) -> cost
local APPLY_AWARDS = 0x0021E9A0 -- btlXpApplyAwards(awards, reward)
local ADD_MONEY, GET_MONEY = 0x00106020, 0x00105ED0
local assist = {half = false, double = false, inAward = false, logged = 0}
local halfHook = p4.hook(SKILL_COST_CALC, "Ipi", "after", function(c: P4HookContext)
    local unit, skill = c.args[1] :: number, c.args[2] :: number
    local cost = c.result :: number
    if cost <= 0 or unit == 0 or skill <= 0 or skill >= ACTIVE_SKILLS then return end
    if bit32.btest(read("u16", unit), 4) then return end -- an enemy's unit record
    local records = read("u32", LIVE_SKILLS)
    if records == 0 or read("u8", records + skill * 0x28 + 3) ~= 2 then return end -- SP costs only
    local half = (cost + 1) // 2
    c.result = half
    touched.half_sp = true
    if assist.logged < 20 then
        assist.logged += 1
        p4.log(string.format("QOL_SP skill=%s cost=%d half=%d", skill_name(skill), cost, half))
    end
end)
local awardEnter = p4.hook(APPLY_AWARDS, "vpp", "before", function(_c: P4HookContext) assist.inAward = true end)
local awardLeave = p4.hook(APPLY_AWARDS, "vpp", "after", function(_c: P4HookContext)
    assist.inAward = false
    p4.log(string.format("QOL_MONEY committed after=%d", p4.call(GET_MONEY, "I") :: number))
end)
local moneyHook = p4.hook(ADD_MONEY, "ii", "before", function(c: P4HookContext)
    if not assist.inAward then return end
    local award = c.args[1] :: number
    if award <= 0 then return end
    c.args[1] = award * 2
    touched.double_money = true
    p4.log(string.format("QOL_MONEY award=%d doubled=%d before=%d", award, award * 2, p4.call(GET_MONEY, "I") :: number))
end)
for _, hook in {halfHook, awardEnter, awardLeave, moneyHook} do hook:disable() end
local function apply_assist(half: boolean, double: boolean)
    if half ~= assist.half then
        assist.half = half
        if half then halfHook:enable() else halfHook:disable() end
    end
    if double ~= assist.double then
        assist.double, assist.inAward = double, false
        for _, hook in {awardEnter, awardLeave, moneyHook} do
            if double then hook:enable() else hook:disable() end
        end
    end
end

--------------------------------------------------------------------------
-- Q10 DUNGEON DASH: while the dash button is held (or always, by choice), the
-- player's own run in a dungeon is 1.25x. FldEvent_StepMoveToward (player
-- control) hands its speed to FldFrameTask_StartMove (00168cb0), which also
-- moves NPCs and scripted actors; only calls made inside player control are
-- scaled, and only while a dungeon floor's step task (00764334) exists.
-- Movement keeps the game's own collision and encounter checks.
--------------------------------------------------------------------------

local START_MOVE = 0x00168CB0 -- FldFrameTask_StartMove(handle, speed)
local STEP_TASK = 0x00764334 -- the dungeon floor step task (FldStepIndex)
local DASH_BUTTONS: {[string]: string} = {Square = "square", Circle = "circle", L3 = "l3", R3 = "r3"}
local dash = {on = false, inStep = false, button = "square", always = false, logged = 0}
local dashEnter = p4.hook(STEP_MOVE, "ip", "before", function(_c: P4HookContext) dash.inStep = true end)
local dashLeave = p4.hook(STEP_MOVE, "ip", "after", function(_c: P4HookContext) dash.inStep = false end)
local dashMove = p4.hook(START_MOVE, "vpf", "before", function(c: P4HookContext)
    if not dash.inStep or read("u32", STEP_TASK) == 0 then return end
    if not dash.always and not (p4.input.action(dash.button :: any)) then return end
    local speed = c.args[2] :: number
    if speed <= 0 then return end
    c.args[2] = speed * 1.25
    touched.dungeon_dash = true
    if dash.logged < 5 then
        dash.logged += 1
        p4.log(string.format("QOL_DASH speed=%d dashed=%d", speed * 100 // 1, speed * 125 // 1))
    end
end)
for _, hook in {dashEnter, dashLeave, dashMove} do hook:disable() end
local function apply_dash(on: boolean, choice: string)
    dash.always = choice == "Always"
    dash.button = DASH_BUTTONS[string.match(choice, "^Hold (%w+)$") or ""] or "square"
    if on == dash.on then return end
    dash.on, dash.inStep = on, false
    for _, hook in {dashEnter, dashLeave, dashMove} do
        if on then hook:enable() else hook:disable() end
    end
end

--------------------------------------------------------------------------
-- N39 PLAN SUMMONS: for the planned Persona, the recipes that need registered
-- Personas the protagonist does not carry, with the compendium's own quote;
-- choosing one queues it (Config does not open inside the Velvet Room), and
-- when the Velvet Room's menu next runs its root, each missing Persona is
-- summoned the way the compendium does (fclCombineMenuPriceInfo state 0x9e): not already carried,
-- room in the roster, enough money, then datPersonaInsertRosterRecord and the
-- price paid through fclAddMoney. Nothing is fused; the player fuses as usual.
--------------------------------------------------------------------------

local FIND_CARRIED, INSERT_ROSTER = 0x0010AC10, 0x0010B190
local ROSTER_CAPACITY, ROSTER_COUNT = 0x0010B5B0, 0x0010B6F0
local SET_MONEY_FCL = 0x002E7A80 -- fclAddMoney(new total), the compendium's own payment
local COMBINE_CREATE, COMBINE_DESTROY = 0x002E8410, 0x002E82B0
local COMBINE_UPDATE, COMBINE_ROOT = 0x002E7AC0, 0x002E90D0
local velvet = {active = false, atRoot = false}
planner_rows = function(): {SummonRow}
    local rows = {}
    if not summonQueue.on or planner.busy or planner.target == 0 then return rows end
    for i, plan in planner.plans do
        if #plan.summons > 0 and #rows < 40 then
            local total, names = 0, {}
            for _, s in plan.summons do
                total += s.price
                table.insert(names, string.format("%s %d yen", persona_name(s.id), s.price))
            end
            table.insert(rows, {plan = i, label = recipe_text(plan.recipe), value = string.format("%d YEN", total),
                help = "Summon " .. table.concat(names, ", ") .. ". At the Velvet Room's menu."})
        end
    end
    return rows
end
local summon_plan: () -> ()
local velvetHooks = {
    p4.hook(COMBINE_CREATE, "ip", "after", function(_c: P4HookContext) velvet.active = true end),
    p4.hook(COMBINE_DESTROY, "vp", "before", function(_c: P4HookContext) velvet.active, velvet.atRoot = false, false end),
    p4.hook(COMBINE_UPDATE, "ip", "before", function(_c: P4HookContext) velvet.atRoot = false end),
    p4.hook(COMBINE_ROOT, "vp", "before", function(_c: P4HookContext)
        velvet.atRoot = true
        if summonQueue.pending then summon_plan() end
    end),
}
for _, hook in velvetHooks do hook:disable() end
summon_plan = function()
    summonQueue.pending = false
    local plan = planner.plans[summonQueue.plan]
    if not plan or planner.target == 0 then
        summonQueue.last = "NOT SUMMONED: PLAN AGAIN"
        return
    end
    if not velvet.active or not velvet.atRoot then
        summonQueue.last = "NOT SUMMONED: ONLY AT THE VELVET ROOM'S MENU"
        p4.log("QOL_SUMMON refused outside the Velvet Room menu")
        return
    end
    local done, paid = {}, 0
    for _, s in plan.summons do
        local record = COMPENDIUM + s.id * 0x30
        local money = p4.call(GET_MONEY, "I") :: number
        local price = summon_price(s.id)
        if (p4.call(FIND_CARRIED, "pi", s.id) :: number) ~= 0 then
            table.insert(done, persona_name(s.id) .. " already carried")
        elseif (p4.call(ROSTER_CAPACITY, "i") :: number) <= (p4.call(ROSTER_COUNT, "H") :: number) then
            table.insert(done, "no room for " .. persona_name(s.id))
            break
        elseif money < price then
            table.insert(done, "not enough money for " .. persona_name(s.id))
            break
        else
            p4.call(INSERT_ROSTER, "ip", record)
            p4.call(SET_MONEY_FCL, "vi", money - price)
            paid += price
            touched.plan_summons = true
            table.insert(done, "summoned " .. persona_name(s.id))
            p4.log(string.format("QOL_SUMMON id=%d name=%s price=%d money=%d->%d", s.id, persona_name(s.id), price, money,
                p4.call(GET_MONEY, "I") :: number))
        end
    end
    summonQueue.last = string.upper(table.concat(done, ", ")) .. string.format(" (%d YEN)", paid)
    p4.log("QOL_SUMMON done " .. table.concat(done, ", "))
end
local function apply_summons(on: boolean)
    if on ~= summonQueue.on then
        summonQueue.on = on
        for _, hook in velvetHooks do
            if on then hook:enable() else hook:disable() end
        end
        if not on then summonQueue.pending = false end
    end
end

--------------------------------------------------------------------------
-- N36 DAMAGE AND EXP ASSISTANCE, each with its own toggle and factor.
-- Damage dealt and taken go through p4.damage (stage.damage@1): the game's final
-- damage stage composes every mod's factors once after the whole retail
-- equation, so heals, drains, fixed and scripted damage and instant death are
-- never scaled, and other mods' factors (hard mode) multiply with these.
-- Battle EXP: btlXpComputeAward (0021de90) is each Persona's base award while
-- btlXpPrepareAwards (0021e110) builds the result screen, so the level-ups it
-- shows and the EXP it applies agree.
--------------------------------------------------------------------------

local apply_damage: (boolean, string, boolean, string, boolean, string) -> ()
local apply_reserve: (boolean) -> ()
local apply_relearn: (boolean, boolean) -> ()
local apply_inherit: (boolean, boolean) -> ()
-- A function, not a block: the chunk's 200 local registers are nearly all in
-- use, and a function has its own.
local function battle_and_party_features()
    local COMPUTE_AWARD, PREPARE_AWARDS = 0x0021DE90, 0x0021E110
    local FACTORS = {["0.5x"] = 0.5, ["2x"] = 2}
    local dmg = {dealt = false, taken = false, exp = false, dealtFactor = 1, takenFactor = 1, expFactor = 1,
        dealtHits = 0, takenHits = 0, firstDealt = 0, firstTaken = 0, inPrepare = false, expLogged = 0}
    local damageApi = (p4 :: any).damage
    local dealtSub, takenSub = nil, nil
    if damageApi ~= nil then
        -- Pure functions of the hit and the settings; they only count for the log.
        dealtSub = damageApi.on({kind = "multiply", fn = function(hit: any): number?
            if hit.source_side ~= "party" or hit.target_side ~= "enemy" then return nil end
            if hit.resolved then
                dmg.dealtHits += 1
                if dmg.firstDealt == 0 then dmg.firstDealt = hit.stock end
            end
            return dmg.dealtFactor
        end})
        takenSub = damageApi.on({kind = "multiply", fn = function(hit: any): number?
            if hit.target_side ~= "party" then return nil end
            if hit.resolved then
                dmg.takenHits += 1
                if dmg.firstTaken == 0 then dmg.firstTaken = hit.stock end
            end
            return dmg.takenFactor
        end})
        dealtSub:disable()
        takenSub:disable()
    end
    local damageReport = p4.events.on("battle_end", function(_e)
        p4.log(string.format("QOL_DAMAGE dealt hits=%d first_stock=%d factor=%s taken hits=%d first_stock=%d factor=%s",
            dmg.dealtHits, dmg.firstDealt, tostring(dmg.dealtFactor), dmg.takenHits, dmg.firstTaken, tostring(dmg.takenFactor)))
        dmg.dealtHits, dmg.takenHits, dmg.firstDealt, dmg.firstTaken = 0, 0, 0, 0
    end)
    damageReport:disable()
    local prepareEnter = p4.hook(PREPARE_AWARDS, "vpp", "before", function(_c: P4HookContext) dmg.inPrepare = true end)
    local prepareLeave = p4.hook(PREPARE_AWARDS, "vpp", "after", function(_c: P4HookContext) dmg.inPrepare = false end)
    local awardHook = p4.hook(COMPUTE_AWARD, "iip", "after", function(c: P4HookContext)
        if not dmg.inPrepare then return end
        local award = c.result :: number
        if award <= 0 then return end
        local scaled = math.max(1, (award * dmg.expFactor) // 1)
        c.result = scaled
        touched.exp_assistance = true
        if dmg.expLogged < 8 then
            dmg.expLogged += 1
            p4.log(string.format("QOL_EXP level=%d award=%d scaled=%d", c.args[1] :: number, award, scaled))
        end
    end)
    for _, hook in {prepareEnter, prepareLeave, awardHook} do hook:disable() end
    function apply_damage(dealt: boolean, dealtChoice: string, taken: boolean, takenChoice: string, exp: boolean,
        expChoice: string)
        dmg.dealtFactor = FACTORS[dealtChoice] or 1
        dmg.takenFactor = FACTORS[takenChoice] or 1
        dmg.expFactor = FACTORS[expChoice] or 1
        dealt, taken = dealt and dealtSub ~= nil, taken and takenSub ~= nil
        if dealt ~= dmg.dealt and dealtSub then
            if dealt then dealtSub:enable() else dealtSub:disable() end
            if dealt then touched.damage_dealt = true end
        end
        if taken ~= dmg.taken and takenSub then
            if taken then takenSub:enable() else takenSub:disable() end
            if taken then touched.damage_taken = true end
        end
        dmg.dealt, dmg.taken = dealt, taken
        if dealt or taken then damageReport:enable() else damageReport:disable() end
        if exp ~= dmg.exp then
            dmg.exp, dmg.inPrepare = exp, false
            for _, hook in {prepareEnter, prepareLeave, awardHook} do
                if exp then hook:enable() else hook:disable() end
            end
        end
    end

    --------------------------------------------------------------------------
    -- N19 RESERVE EXP: members who have joined but sat the battle out earn what
    -- they would have as part of it. When btlXpAddPartyExp (0021ec40) applies the
    -- party's EXP, each unlocked member outside the battle's party (Rise, the
    -- navigator, keeps her own rule) gets the party formula of btlXpPrepareAwards:
    -- btlXpComputeAward for its level plus half again per Growth item equipped,
    -- then the game's own datPersonaPreviewLevelUp, datPersonaAddExp and
    -- datPersonaApplyLevelUp, so its level-ups and skills are the game's. BATTLE EXP,
    -- when on, scales it as it scales the party's.
    --------------------------------------------------------------------------

    local ADD_PARTY_EXP, MEMBER_LIST = 0x0021EC40, 0x00353CE0 -- datBuildMemberIdListExcluding5(out)
    local PERSONA_RECORD, PERSONA_LEVEL = 0x0010A900, 0x00109390
    local PREVIEW_LEVEL, ADD_EXP, CAN_LEVEL, APPLY_LEVEL = 0x0010BE60, 0x0010C980, 0x0010C6F0, 0x0010C5A0
    local EQUIPPED, ITEM_VALUE = 0x00106CD0, 0x001069D0
    local GROWTH_ITEM = 0x86 -- the equipment value btlXpPrepareAwards gives +50% EXP for
    local reserve = {on = false, reward = 0}
    local reserveScratch = p4.mem.alloc(0x100)
    local reservePrepare = p4.hook(PREPARE_AWARDS, "vpp", "before", function(c: P4HookContext)
        reserve.reward = c.args[2] :: number
    end)
    local reserveApply = p4.hook(ADD_PARTY_EXP, "vi", "after", function(_c: P4HookContext)
        local reward = reserve.reward
        reserve.reward = 0
        if reward == 0 then return end
        local inBattle = {}
        for i = 0, math.min(read("i32", reward + 0x1C), 5) - 1 do inBattle[read("u16", reward + 0x12 + i * 2)] = true end
        local list = reserveScratch.address
        local count = p4.call(MEMBER_LIST, "hp", list) :: number
        for i = 0, math.min(count, 8) - 1 do
            local id = read("i16", list + i * 2)
            if id > 1 and id ~= 5 and not inBattle[id] then
                local persona = p4.call(PERSONA_RECORD, "pH", id) :: number
                local level = persona ~= 0 and (p4.call(PERSONA_LEVEL, "Bi", persona) :: number) or 99
                if level < 99 then
                    local base = p4.call(COMPUTE_AWARD, "iip", level, reward) :: number
                    if dmg.exp then base = math.max(1, (base * dmg.expFactor) // 1) end
                    local award = base
                    for slot = 0, 1 do
                        local item = p4.call(EQUIPPED, "hhh", id, slot) :: number
                        if (p4.call(ITEM_VALUE, "Hh", item) :: number) == GROWTH_ITEM then award += base // 2 end
                    end
                    if award > 0 then
                        local preview = reserveScratch.address + 0x20
                        local before = read("i32", persona + 0x08) -- DatPersonaRecord.exp
                        p4.call(PREVIEW_LEVEL, "vppi", persona, preview, award)
                        p4.call(ADD_EXP, "vpi", persona, award)
                        local leveled = (p4.call(CAN_LEVEL, "ip", persona) :: number) ~= 0
                        if leveled then p4.call(APPLY_LEVEL, "vpp", persona, preview) end
                        touched.reserve_exp = true
                        p4.log(string.format("QOL_RESERVE id=%d level=%d award=%d exp=%d->%d leveled=%s now=%d", id, level,
                            award, before, read("i32", persona + 0x08), tostring(leveled),
                            p4.call(PERSONA_LEVEL, "Bi", persona) :: number))
                    end
                end
            end
        end
    end)
    reservePrepare:disable()
    reserveApply:disable()
    function apply_reserve(on: boolean)
        if on == reserve.on then return end
        reserve.on, reserve.reward = on, 0
        if on then
            reservePrepare:enable()
            reserveApply:enable()
        else
            reserveApply:disable()
            reservePrepare:disable()
        end
    end

    --------------------------------------------------------------------------
    -- N21 RELEARN SKILLS: a party member's Persona relearns a skill its own
    -- learnset taught it at or below its current level and it has since
    -- forgotten (the eight-slot replacement at a level-up). Only the game's party
    -- learnsets are read (the table at 007644d4, the one datPersonaPreviewLevelUp
    -- uses); skills the learnset itself removes, inherited or foreign skills are
    -- never offered. The change is the game's own datPersonaRemoveSkill
    -- (0010ccc0) and datPersonaAddSkill (0010cc20) on the member's record, made
    -- when the menus close, after checking the skills again. It costs nothing and
    -- takes no time.
    --------------------------------------------------------------------------

    local LEARNSETS = 0x007644D4 -- party Persona learnsets (0xC0-0xD7), 0x26E bytes each, 32 events from +4
    local ADD_SKILL, REMOVE_SKILL = 0x0010CC20, 0x0010CCC0
    local relearnScratch = p4.mem.alloc(0x20)
    local function relearn_scan(persona: number): ({number}, {RelearnSkill})
        local id, level = read("u16", persona + 2), read("u8", persona + 4)
        local skills, known = {}, {}
        for i = 0, 7 do
            local skill = read("u16", persona + 0x0C + i * 2)
            if skill ~= 0 then
                table.insert(skills, skill)
                known[skill] = true
            end
        end
        local out = {}
        local base = read("u32", LEARNSETS)
        if base == 0 or id < 0xC0 or id > 0xD7 then return skills, out end
        local learned, removed = {}, {}
        for i = 0, 31 do
            local event = base + (id - 0xC0) * 0x26E + 4 + i * 4
            local kind = read("i8", event + 1)
            if kind == 0 then break end
            local at, skill = read("u8", event), read("u16", event + 2)
            if skill ~= 0 and at <= level then
                if kind == 1 then table.insert(learned, {skill = skill, level = at})
                elseif kind == 2 then removed[skill] = true end
            end
        end
        for _, event in learned do
            if not known[event.skill] and not removed[event.skill] then
                known[event.skill] = true
                table.insert(out, {id = event.skill, name = skill_name(event.skill), level = event.level,
                    cost = skill_cost(event.skill)})
            end
        end
        return skills, out
    end
    -- One member per frame, whenever the menus close; it starts by checking
    -- whether any joined member's Persona changed since the last scan.
    local function relearn_refresh()
        if relearn.busy then return end
        relearn.busy = true
        local ids, members, key = {} :: {number}, {} :: {RelearnMember}, nil :: string?
        local nextMember = 1
        schedule(function(): boolean
            if key == nil then
                local count = p4.call(MEMBER_LIST, "hp", relearnScratch.address) :: number
                local parts = {}
                for i = 0, math.min(count, 8) - 1 do
                    local id = read("i16", relearnScratch.address + i * 2)
                    if id > 1 and id ~= 5 then
                        table.insert(ids, id)
                        table.insert(parts, buffer.tostring(p4.mem.read_bytes(PARTY_PERSONA + id * 0x88, 0x1C)))
                    end
                end
                key = table.concat(parts)
                if key == relearn.key then
                    relearn.busy = false
                    return true
                end
                return false
            end
            local id = ids[nextMember]
            if id then
                local persona = PARTY_PERSONA + id * 0x88
                local skills, candidates = relearn_scan(persona)
                if #candidates > 0 then
                    local names = {}
                    for _, skill in skills do table.insert(names, skill_name(skill)) end
                    table.insert(members, {id = id, skills = skills, names = names, candidates = candidates,
                        label = string.format("%s  %s LV %d", member_name(id), persona_name(read("u16", persona + 2)),
                            read("u8", persona + 4))})
                end
                nextMember += 1
                return false
            end
            local shown = relearn.members[relearn.member + 1]
            relearn.members, relearn.key, relearn.busy = members, key :: string, false
            relearn.member, relearn.pick, relearn.replace, relearn.pending = 0, 0, 1, false
            for i, member in members do
                if shown and member.id == shown.id then relearn.member = i - 1 end
            end
            p4.log(string.format("QOL_RELEARN_SCAN members=%d", #members))
            return true
        end)
    end
    local function relearn_commit()
        relearn.pending = false
        local member = relearn.members[relearn.member + 1]
        local skill = member and member.candidates[relearn.pick]
        if not member or not skill then return end
        local id, wanted = member.id, skill.id
        local replaced = #member.skills >= 8 and member.skills[relearn.replace] or 0
        schedule(function(): boolean
            local persona = p4.call(PERSONA_RECORD, "pH", id) :: number
            if persona == 0 then return true end
            local skills, candidates = relearn_scan(persona)
            local legal, present = false, replaced == 0 and #skills < 8
            for _, candidate in candidates do if candidate.id == wanted then legal = true end end
            for _, current in skills do if replaced ~= 0 and current == replaced then present = true end end
            if not legal or not present then
                relearn.last = "NOT RELEARNED: THE SKILLS CHANGED"
                p4.log(string.format("QOL_RELEARN member=%d skill=%d refused", id, wanted))
                return true
            end
            if replaced ~= 0 then p4.call(REMOVE_SKILL, "ipI", persona, replaced) end
            local ok = (p4.call(ADD_SKILL, "ipH", persona, wanted) :: number) == 1
            touched.relearn_skills = true
            local now = {}
            for i = 0, 7 do table.insert(now, tostring(read("u16", persona + 0x0C + i * 2))) end
            relearn.last = ok and string.format("LAST: %s RELEARNED %s", member_name(id), skill_name(wanted)) or
                "NOT RELEARNED: NO FREE SLOT"
            p4.log(string.format("QOL_RELEARN member=%d skill=%d replaced=%d ok=%s skills=%s", id, wanted, replaced,
                tostring(ok), table.concat(now, ",")))
            return true
        end)
    end
    function apply_relearn(on: boolean, closing: boolean)
        relearn.on = on
        if not on then
            relearn.pending, relearn.members, relearn.key = false, {}, ""
            return
        end
        if not closing and p4.ui.active() ~= nil then return end
        if relearn.pending then relearn_commit() end
        relearn_refresh()
    end

    --------------------------------------------------------------------------
    -- Q6 CHOSEN INHERITANCE: the player marks skills on the INHERITANCE page
    -- (the Velvet Room has no Config), and every fusion roll honours them. After
    -- fclFuseRollPersona (00312c60) has drawn the game's own number of skills
    -- from the ingredients' pool, each marked skill that is in that pool, which
    -- the result lacks and which the game's own inheritance weight allows
    -- (nonzero: the result's inheritance class against the skill's element,
    -- scaled by the skill's rate, as the roll computes it) replaces one of the
    -- random picks, in the order marked, through datPersonaRemoveSkill and
    -- datPersonaAddSkill. The draw itself, its count and the slots it filled are
    -- unchanged, so the preview the player confirms is the fusion they get.
    --------------------------------------------------------------------------

    local ROLL_PERSONA = 0x00312C60
    local FUSE_SOURCES = 0x002F9C30 -- fclCombineFuseSourceRecords(result, six sources, count, list slot, entry)
    local ROSTER_INSERT = 0x0010B190 -- datPersonaInsertRosterRecord: a fused (or summoned) Persona joins
    local INHERIT_BASE = 0x00641C40 -- s16 weight [inheritance class][19 element columns]
    local INHERIT_RATE = 0x00641E60 -- s8 per-skill rate; 0 never inherits
    local WANTED_KEY = "inherit.wanted"
    local function inherit_weight(persona: number, skill: number): number
        local elements, bases = read("u32", LIVE_ELEMENTS), read("u32", LIVE_PERSONAS)
        if elements == 0 or bases == 0 then return 0 end
        local element = read("i8", elements + skill * 2)
        if element == -1 then element = 0x12 end
        local rate = read("i8", INHERIT_RATE + skill)
        if rate == 0 or element < 0 or element > 0x12 then return 0 end
        local class = read("u16", bases + read("u16", persona + 2) * 0x0E + 0x0A)
        return read("i16", INHERIT_BASE + (class * 19 + element) * 2) * 10 // rate
    end
    -- The roll reads copies of the ingredients on the caller's stack, which
    -- mods cannot read; their records are read here, where the copies come from.
    local sources: {number} = {}
    local marks: {[number]: string} = {} -- result Persona -> "n of m" marked picks, from its latest roll
    local sourcesHook = p4.hook(FUSE_SOURCES, "vpppppppibb", "before", function(c: P4HookContext)
        sources = {}
        for i = 1, math.min(c.args[8] :: number, 6) do table.insert(sources, c.args[i + 1] :: number) end
    end)
    sourcesHook:disable()
    local rollHook = p4.hook(ROLL_PERSONA, "bppi", "after", function(c: P4HookContext)
        local persona = c.args[1] :: number
        local ingredients = sources
        sources = {}
        local mask = bit32.band(c.result :: number, 0xFF)
        if mask == 0 or persona == 0 or #ingredients == 0 then return end
        local picked, base = {}, {}
        for slot = 0, 7 do
            local skill = read("u16", persona + 0x0C + slot * 2)
            if bit32.btest(mask, bit32.lshift(1, slot)) then table.insert(picked, skill)
            elseif skill ~= 0 then base[skill] = true end
        end
        local pool, pooled = {}, {}
        for _, source in ingredients do
            for j = 0, 7 do
                local skill = read("u16", source + 0x0C + j * 2)
                if skill ~= 0 and not base[skill] and not pool[skill] then
                    pool[skill] = true
                    table.insert(pooled, tostring(skill))
                end
            end
        end
        local wanted, keep = {}, {}
        for _, id in inherit.wanted do wanted[id] = true end
        local random = {}
        for _, skill in picked do
            if wanted[skill] then keep[skill] = true else table.insert(random, skill) end
        end
        local swapped = {}
        for _, id in inherit.wanted do
            if #random == 0 then break end
            if pool[id] and not keep[id] and inherit_weight(persona, id) > 0 then
                local victim = table.remove(random) :: number
                p4.call(REMOVE_SKILL, "ipI", persona, victim)
                p4.call(ADD_SKILL, "ipH", persona, id)
                keep[id] = true
                table.insert(swapped, string.format("%d>%d", victim, id))
            end
        end
        local now = {}
        for slot = 0, 7 do table.insert(now, tostring(read("u16", persona + 0x0C + slot * 2))) end
        local marked = #picked - #random
        marks[read("u16", persona + 2)] = marked > 0 and string.format("%d of %d", marked, #picked) or nil
        local from = {}
        for _, source in ingredients do table.insert(from, tostring(read("u16", source + 2))) end
        p4.log(string.format("QOL_INHERIT persona=%d from=%s pool=%s picked=%d marked=%d swapped=%s skills=%s",
            read("u16", persona + 2), table.concat(from, "+"), table.concat(pooled, ","), #picked, marked,
            #swapped > 0 and table.concat(swapped, ",") or "none", table.concat(now, ",")))
    end)
    rollHook:disable()
    -- A fused Persona that inherited marked skills is reported when it joins.
    local insertHook = p4.hook(ROSTER_INSERT, "ip", "after", function(c: P4HookContext)
        local record = c.args[1] :: number
        local id = read("u16", record + 2)
        local marked = marks[id]
        if not marked then return end
        marks = {}
        touched.chosen_inheritance = true
        inherit.last = string.format("LAST FUSION: %s %s INHERITED AS MARKED", persona_name(id), marked)
        local now = {}
        for slot = 0, 7 do table.insert(now, tostring(read("u16", record + 0x0C + slot * 2))) end
        p4.log(string.format("QOL_INHERIT_FUSED persona=%d marked=%s skills=%s", id, marked, table.concat(now, ",")))
    end)
    insertHook:disable()
    -- The page's rows: every skill the carried Personas have, formatted one
    -- Persona per frame whenever the roster changes.
    local function inherit_refresh()
        if inherit.busy then return end
        local parts = {}
        for slot = 0, 11 do table.insert(parts, buffer.tostring(p4.mem.read_bytes(ROSTER + slot * 0x30, 0x1C))) end
        local key = table.concat(parts)
        if key == inherit.key then return end
        inherit.busy = true
        local rows, index, slot = {} :: {InheritRow}, {} :: {[number]: number}, 0
        schedule(function(): boolean
            local record = persona_record(ROSTER + slot * 0x30)
            if record then
                local name = persona_name(record.id)
                for _, skill in record.skills do
                    local at = index[skill]
                    if at then
                        rows[at].from ..= ", " .. name
                    else
                        table.insert(rows, {id = skill, name = skill_name(skill), from = name})
                        index[skill] = #rows
                    end
                end
            end
            slot += 1
            if slot < 12 then return false end
            inherit.rows, inherit.key, inherit.busy = rows, key, false
            return true
        end)
    end
    -- The marks persist as a data key, written when the menus close (UI
    -- callbacks cannot write settings).
    function apply_inherit(on: boolean, closing: boolean)
        if on ~= inherit.on then
            inherit.on = on
            if on then
                inherit.wanted = {}
                for id in string.gmatch(tostring(p4.settings.get(WANTED_KEY) or ""), "%d+") do
                    table.insert(inherit.wanted, tonumber(id) :: number)
                end
                p4.log(string.format("QOL_INHERIT_MARKS n=%d stored=%s", #inherit.wanted, tostring(p4.settings.get(WANTED_KEY))))
                sourcesHook:enable()
                rollHook:enable()
                insertHook:enable()
            else
                insertHook:disable()
                rollHook:disable()
                sourcesHook:disable()
                sources, marks = {}, {}
                inherit.rows, inherit.key, inherit.dirty = {}, "", false
            end
        end
        if not on or (not closing and p4.ui.active() ~= nil) then return end
        if inherit.dirty then
            inherit.dirty = false
            local ids = {}
            for _, id in inherit.wanted do table.insert(ids, tostring(id)) end
            local ok = p4.settings.set(WANTED_KEY, #ids > 0 and table.concat(ids, ",") or nil)
            p4.log(string.format("QOL_INHERIT_SAVED marks=%s ok=%s", #ids > 0 and table.concat(ids, ",") or "none",
                tostring(ok)))
        end
        inherit_refresh()
    end
end
battle_and_party_features()

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
    for _, key in {"skill_compare", "compendium_filter", "fusion_results", "fusion_planner", "forecast", "glossary"} do
        setting_cache[key] = setting(key)
    end
end

local function refresh(closing: boolean?)
    refresh_settings()
    apply_descriptions(setting("full_descriptions"), p4.settings.get("description_style") :: string)
    apply_backlog(setting("backlog"))
    apply_hints(setting("affinity_hints"))
    apply_camera(setting("camera_speed"), p4.settings.get("camera_speed_percent") :: string)
    apply_audio(setting("audio_volumes"))
    apply_text(setting("fast_text"), setting("auto_dialogue"), p4.settings.get("auto_dialogue_delay") :: string)
    apply_movies(setting("skip_seen_movies"))
    apply_heal(setting("party_heal"), closing == true)
    apply_battle(setting("direct_commands"), setting("weakness_assist"))
    apply_assist(setting("half_sp"), setting("double_money"))
    apply_dash(setting("dungeon_dash"), p4.settings.get("dash_mode") :: string)
    apply_summons(setting("plan_summons"))
    apply_damage(setting("damage_dealt"), p4.settings.get("damage_dealt_factor") :: string, setting("damage_taken"),
        p4.settings.get("damage_taken_factor") :: string, setting("battle_exp"), p4.settings.get("battle_exp_factor") :: string)
    apply_reserve(setting("reserve_exp"))
    apply_relearn(setting("relearn_skills"), closing == true)
    apply_inherit(setting("chosen_inheritance"), closing == true)
    if setting_cache.fusion_results then recompute_fusions() end
    if setting_cache.skill_compare then compare_refresh() end
    if setting_cache.forecast then forecast_refresh() end
end

-- menu_close is the event to follow; builds whose Camp/Config seams are not
-- hookable refuse it, and then a check every 30 frames stands in for it.
pcall(function()
    p4.events.on("menu_open", function(_e)
        if heal.on and not heal.previewed then
            heal_preview()
            heal.previewed = true
        end
    end)
end)
if not pcall(function() p4.events.on("menu_close", function(_e) refresh(true) end) end) then
    local frames = 0
    p4.events.on("frame_start", function(_e)
        frames += 1
        if frames >= 30 then
            frames = 0
            refresh()
        end
    end)
end
p4.events.on("field_enter", function(_e)
    text.fields = true
    heal.field = true
    refresh()
end)
p4.events.on("field_leave", function(_e) heal.field = false end)

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
