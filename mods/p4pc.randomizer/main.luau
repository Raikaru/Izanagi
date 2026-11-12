--!strict
-- Bundled seeded randomizer (settings in mod.ini). Off by default: the
-- RANDOMIZE NEW GAMES setting and every feature toggle start OFF, and with them
-- off no callback runs and no game byte changes.
--
-- A run is defined by (algorithm version, 128-bit seed, feature options, the
-- fingerprint of the game data its features read, catalog version). The
-- same definition generates byte-identical tables on every platform: all
-- randomness comes from SHA-256-derived xoshiro128** streams computed with
-- exact integer arithmetic on bit32/doubles (SHA-256 by the native p4.sha256),
-- never the game RNG, the clock,
-- host entropy or math.random. Generation runs as a coroutine across
-- frame_start callbacks (paced by p4.budget(), which never changes a result),
-- validates its own output, then publishes the run as one p4.tables overlay at
-- the first field after a New Game reset. The run's definition and generated
-- banks travel in the save's block; every load and New Game reset ends the
-- previous run's overlay, and a loaded randomized save re-applies its own.
-- A spoiler log and manifest are written under the mod's artifact folder.

local ALGORITHM = 1
local CATALOG = 1
local SAVE_SCHEMA = 1
local TWO32 = 4294967296

local band, bxor = bit32.band, bit32.bxor
local lrot, lsh = bit32.lrotate, bit32.lshift

-- Features: stable numeric IDs (option bit = id - 1) and their setting keys.
type Feature = {id: number, key: string, code: string, name: string}
local FEATURES: {Feature} = {
    {id = 1, key = "r1_skills", code = "R1", name = "Persona skills"},
    {id = 2, key = "r2_stats", code = "R2", name = "Persona stats"},
    {id = 3, key = "r3_affinities", code = "R3", name = "Persona affinities"},
    {id = 4, key = "r4_fusion", code = "R4", name = "Fusion results"},
    {id = 5, key = "r5_enemies", code = "R5", name = "Enemy placement"},
    {id = 6, key = "r6_enemy_stats", code = "R6", name = "Enemy stats"},
    {id = 7, key = "r7_drops", code = "R7", name = "Enemy drops"},
    {id = 8, key = "r8_chests", code = "R8", name = "Chest contents"},
    {id = 9, key = "r9_shops", code = "R9", name = "Shop offers"},
    {id = 10, key = "r10_shuffle", code = "R10", name = "Shuffle Time cards"},
    {id = 11, key = "r11_starter", code = "R11", name = "Starting Persona"},
    {id = 12, key = "r12_music", code = "R12", name = "Battle music"},
}

-------------------------------------------------------------------------------
-- Exact 32-bit arithmetic, SHA-256 and the run's random streams
-------------------------------------------------------------------------------

-- Cooperative pacing: inside the generation coroutine, yield when the
-- interpreter budget runs low. Outputs never depend on where work was split.
local PACE_MARGIN = 2048
local function pace()
    if coroutine.isyieldable() and p4.budget() < PACE_MARGIN then
        coroutine.yield()
    end
end

-- SHA-256 through the shared native digest (sha256@1): one native
-- call per message, so hashing never spends interpreter budget per block.
local function sha256(message: string): string
    return p4.sha256(message)
end

-- Lowercase hexadecimal, 512 bytes per native format call.
local HEX_CHUNK = 512
local HEX_FORMAT = string.rep("%02x", HEX_CHUNK)
local function hex(bytes: string): string
    local parts: {string} = {}
    for at = 1, #bytes, HEX_CHUNK do
        local size = math.min(HEX_CHUNK, #bytes - at + 1)
        local format = if size == HEX_CHUNK then HEX_FORMAT else string.rep("%02x", size)
        table.insert(parts, string.format(format, string.byte(bytes, at, at + size - 1)))
    end
    return table.concat(parts)
end

local function unhex(text: string): string?
    if #text % 2 ~= 0 or string.find(text, "[^0-9a-fA-F]") then
        return nil
    end
    return (string.gsub(text, "..", function(pair: string): string
        return string.char(tonumber(pair, 16) :: number)
    end))
end

local function le32(value: number): string
    assert(value >= 0 and value < TWO32 and value % 1 == 0, "le32 needs an unsigned 32-bit integer")
    return string.pack("<I4", value)
end

-- xoshiro128** over four unsigned 32-bit words.
export type Rng = {s0: number, s1: number, s2: number, s3: number}

local function rngNext(r: Rng): number
    local s1 = r.s1
    local result = (lrot((s1 * 5) % TWO32, 7) * 9) % TWO32
    local t = lsh(s1, 9)
    local s2 = bxor(r.s2, r.s0)
    local s3 = bxor(r.s3, s1)
    r.s1 = bxor(s1, s2)
    r.s0 = bxor(r.s0, s3)
    r.s2 = bxor(s2, t)
    r.s3 = lrot(s3, 11)
    return result
end

-- Uniform integer in 0..n-1 by rejection, for integral 1 <= n <= 2^32.
local function below(r: Rng, n: number): number
    if n % 1 ~= 0 or n < 1 or n > TWO32 then
        error("below() needs an integer bound in 1..2^32, got " .. tostring(n))
    end
    local limit = math.floor(TWO32 / n) * n
    while true do
        local x = rngNext(r)
        if x < limit then
            return x - math.floor(x / n) * n
        end
    end
end

-- Fisher-Yates from the end of a one-indexed list.
local function shuffle<T>(r: Rng, list: {T})
    for i = #list, 2, -1 do
        pace()
        local j = below(r, i) + 1
        list[i], list[j] = list[j], list[i]
    end
end

-- An independent stream per stable ASCII label: the SHA-256 of
-- "P4PC-RANDO" 0x00 LE32(algorithm) seed LE32(#label) label; digest bytes
-- 0..15 are the four little-endian state words (all zero becomes s0 = 1).
local function stream(seed: string, label: string): Rng
    assert(#seed == 16, "seeds are 16 bytes")
    local digest = sha256("P4PC-RANDO\0" .. le32(ALGORITHM) .. seed .. le32(#label) .. label)
    local s0 = string.unpack("<I4", digest, 1) :: number
    local s1 = string.unpack("<I4", digest, 5) :: number
    local s2 = string.unpack("<I4", digest, 9) :: number
    local s3 = string.unpack("<I4", digest, 13) :: number
    if s0 == 0 and s1 == 0 and s2 == 0 and s3 == 0 then
        s0 = 1
    end
    return {s0 = s0, s1 = s1, s2 = s2, s3 = s3}
end

-- Unsigned 64-bit counters as (hi, lo) words; serialized LE32(lo) LE32(hi).
local function counterAdd(hi: number, lo: number, amount: number): (number, number)
    assert(amount >= 0 and amount < TWO32 and amount % 1 == 0, "counter steps are 32-bit")
    local sum = lo + amount
    local carry = math.floor(sum / TWO32)
    hi += carry
    if hi >= TWO32 then
        error("randomizer counter overflow")
    end
    return hi, sum - carry * TWO32
end

local function counterHex(hi: number, lo: number): string
    return string.format("%08x%08x", hi, lo)
end

-------------------------------------------------------------------------------
-- The run definition
-------------------------------------------------------------------------------

export type Definition = {
    algorithm: number,
    catalog: number,
    seed: string, -- 16 bytes
    options: number, -- feature bit set
    fingerprint: string, -- SHA-256 of the input banks (32 bytes)
}

local function optionsText(options: number): string
    local names = {}
    for _, feature in FEATURES do
        if band(options, lsh(1, feature.id - 1)) ~= 0 then
            table.insert(names, feature.code)
        end
    end
    return #names > 0 and table.concat(names, ",") or "none"
end

-- Canonical serialization: fixed widths, little-endian, explicit tag.
local function canonical(def: Definition): string
    return "P4PC-RANDO-RUN\0" .. le32(def.algorithm) .. le32(def.catalog) .. def.seed
        .. le32(def.options) .. def.fingerprint
end

local function runDigest(def: Definition): string
    return sha256(canonical(def))
end

-- The copyable run code: version, seed, options and a short input fingerprint.
local function runCode(def: Definition): string
    return string.format("P4R%d-%s-%03x-%s", def.algorithm, hex(def.seed), def.options,
        string.sub(hex(def.fingerprint), 1, 8))
end

-------------------------------------------------------------------------------
-- Game data: banks, eligibility and the safety catalog
-------------------------------------------------------------------------------

-- Banks a run reads, in fingerprint order. GATES is not a bank: the fusion gate
-- list in ELF data (0x006420a0, 23 x 8 bytes); ITEMS neither: the item table
-- behind the 0x007642a0 pointer.
local INPUT_BANKS: {string} = {"SKILL0", "SKILL1", "PERSONA0", "PERSONA1", "UNIT2", "MSG1", "MSG3", "FUSION0",
                               "FUSION1", "FUSION2", "FUSION3", "GATES", "UNIT0", "MSG2", "ENCOUNT0", "ENCOUNT3",
                               "ENCOUNT4", "ITEMS"}
-- Every run reads the base banks; a feature adds its own. A run's input
-- fingerprint covers exactly the banks its features read, so a run keeps its
-- code and verifies unchanged when later features add inputs.
local BASE_INPUTS: {[string]: boolean} = {SKILL0 = true, SKILL1 = true, PERSONA0 = true, PERSONA1 = true,
                                          UNIT2 = true, MSG1 = true, MSG3 = true}
local FEATURE_INPUTS: {[number]: {string}} = {
    [4] = {"FUSION0", "FUSION1", "FUSION2", "FUSION3", "GATES"},
    [5] = {"UNIT0", "MSG2", "ENCOUNT0", "ENCOUNT3"},
    [6] = {"UNIT0", "MSG2", "ENCOUNT0", "ENCOUNT3"},
    [7] = {"UNIT0", "MSG2", "ENCOUNT0", "ENCOUNT3", "ITEMS"},
    [8] = {"ENCOUNT4", "ITEMS"},
    [9] = {"ITEMS"},
    [12] = {"UNIT0", "ENCOUNT0", "ENCOUNT3"},
}
local function inputsFor(options: number): {string}
    local wanted = table.clone(BASE_INPUTS)
    for feature, keys in FEATURE_INPUTS do
        if bit32.btest(options, bit32.lshift(1, feature - 1)) then
            for _, key in keys do
                wanted[key] = true
            end
        end
    end
    local keys: {string} = {}
    for _, key in INPUT_BANKS do
        if wanted[key] then
            table.insert(keys, key)
        end
    end
    return keys
end
local GATES_ADDRESS, GATES_BYTES = 0x006420a0, 23 * 8
export type Stock = {[string]: buffer}

local PERSONA_ROWS = 256
local META_STRIDE = 14 -- PERSONA0: flags u16, arcana u8 +2, base level u8 +3, St..Lu u8 +4..+8
local GROWTH_STRIDE = 70 -- PERSONA1: growth weights u8 +0..+4, 16 learn events (level, kind, skill u16) from +6
local AFFINITY_STRIDE = 32 -- UNIT2: sixteen u16 affinity words
local SKILL_ELEMENT_STRIDE = 2 -- SKILL0: element s8
local SKILL_STRIDE = 40 -- SKILL1: active skill rows; effect kind u8 +0x11
local ACTIVE_SKILLS = 0x1b8
local PERSONA_NAME_STRIDE = 17
local SKILL_NAME_STRIDE = 19
local LEARN_EVENTS = 16

-- Safety catalog version 1. Protected personas keep every stock byte:
--   0x01 Izanagi: the protagonist's starter, its forced tutorial actions and
--        weakness relationships stay stock until a starter feature replaces it.
--   0xC0..0xFF: party, evolution, enemy-side and unused rows (fixed growth or
--        not obtainable by the player).
-- Rows whose flags carry 0x01/0x04/0x08/0x10/0x20, or that have no arcana or a
-- base level outside 1..98, are placeholders or special rows and are protected
-- as well; the eligible set is derived from the player's own disc data.
local PROTECTED_PERSONAS: {[number]: string} = {[0x01] = "protagonist starter and tutorial"}
local SPECIAL_FLAGS = 0x3d

local function eligiblePersona(stock: Stock, id: number): boolean
    if id < 1 or id >= 0xc0 or PROTECTED_PERSONAS[id] then
        return false
    end
    local row = id * META_STRIDE
    local flags = buffer.readu16(stock.PERSONA0, row)
    local arcana = buffer.readu8(stock.PERSONA0, row + 2)
    local level = buffer.readu8(stock.PERSONA0, row + 3)
    return band(flags, SPECIAL_FLAGS) == 0 and arcana ~= 0 and level >= 1 and level <= 98
end

local function readName(bank: buffer?, index: number, stride: number): string
    if bank == nil or (index + 1) * stride > buffer.len(bank) then
        return "#" .. tostring(index)
    end
    local text = buffer.readstring(bank, index * stride, stride)
    local zero = string.find(text, "\0", 1, true)
    if zero then
        text = string.sub(text, 1, zero - 1)
    end
    text = string.gsub(text, "[^\32-\126]", "?")
    return text
end

-- Skill roles from the stock skill definitions, which never change:
-- damage (Phys..Almighty HP damage), instakill (Light/Dark), ailment, heal (HP
-- recovery), cure (other recovery), support, utility, passive.
local function skillRole(stock: Stock, skill: number): string
    local elements = stock.SKILL0
    if skill >= ACTIVE_SKILLS or (skill + 1) * SKILL_ELEMENT_STRIDE > buffer.len(elements) then
        return "passive"
    end
    local element = buffer.readi8(elements, skill * SKILL_ELEMENT_STRIDE)
    if element < 0 then
        return "passive"
    end
    local effect = 0
    if (skill + 1) * SKILL_STRIDE <= buffer.len(stock.SKILL1) then
        effect = buffer.readu8(stock.SKILL1, skill * SKILL_STRIDE + 0x11)
    end
    if element <= 5 then
        return effect == 1 and "damage" or "utility"
    elseif element <= 7 then
        return "instakill"
    elseif element <= 15 then
        return "ailment"
    elseif element == 16 then
        return effect == 2 and "heal" or "cure"
    elseif element == 17 then
        return "support"
    end
    return "utility"
end

-------------------------------------------------------------------------------
-- Generation
-------------------------------------------------------------------------------

export type Result = {
    definition: Definition,
    digest: string,
    layer: {[string]: buffer},
    layerHash: string,
    spoiler: string,
    manifest: string,
    counts: {[string]: {changed: number, eligible: number}},
}

type Event = {persona: number, slot: number, level: number, skill: number, stock: number, role: string, band: string}

local MAX_ATTEMPTS = 256

-- R1: shuffle skills among learn events of the same role and ten-level band.
-- Levels, kinds, slot counts and sentinels stay; each event keeps its role, so
-- every persona keeps its damaging, healing and support access at the same
-- levels. A persona never learns one skill twice: duplicates are repaired by a
-- deterministic swap inside the band, and a band that cannot be repaired is
-- reshuffled with the next attempt number (at most 256).
local function generateSkills(def: Definition, stock: Stock, growth: buffer, eligible: {number},
                              log: {string}): (number, number)
    local events: {Event} = {}
    local bands: {[string]: {number}} = {}
    local bandNames: {string} = {}
    -- Known skills per persona, kept current as bands are assigned.
    local known: {[number]: {[number]: number}} = {}
    for _, id in eligible do
        pace()
        local base = buffer.readu8(stock.PERSONA0, id * META_STRIDE + 3)
        local set: {[number]: number} = {}
        known[id] = set
        for slot = 0, LEARN_EVENTS - 1 do
            local at = id * GROWTH_STRIDE + 6 + slot * 4
            local kind = buffer.readi8(stock.PERSONA1, at + 1)
            if kind == 0 then
                break
            end
            if kind == 1 then
                local skill = buffer.readu16(stock.PERSONA1, at + 2)
                local level = math.clamp(base + buffer.readu8(stock.PERSONA1, at), 1, 99)
                local role = skillRole(stock, skill)
                local key = string.format("%s/%02d", role, (level - 1) // 10)
                table.insert(events, {persona = id, slot = slot, level = level, skill = skill, stock = skill,
                                      role = role, band = key})
                set[skill] = (set[skill] or 0) + 1
                local members = bands[key]
                if members == nil then
                    members = {}
                    bands[key] = members
                    table.insert(bandNames, key)
                end
                table.insert(members, #events)
            end
        end
    end
    local function assign(event: Event, skill: number)
        local set = known[event.persona]
        set[event.skill] -= 1
        set[skill] = (set[skill] or 0) + 1
        event.skill = skill
    end
    table.sort(bandNames)
    for _, key in bandNames do
        local members = bands[key]
        local attempt = 0
        while true do
            if attempt >= MAX_ATTEMPTS then
                error("R1: band " .. key .. " has no duplicate-free assignment after 256 attempts")
            end
            local r = stream(def.seed, string.format("persona-skills/band/%s/attempt/%d", key, attempt))
            local skills: {number} = {}
            for i, index in members do
                skills[i] = events[index].stock
            end
            pace()
            shuffle(r, skills)
            for i, index in members do
                pace()
                assign(events[index], skills[i])
            end
            local repaired = true
            for _, index in members do
                pace()
                local event = events[index]
                if known[event.persona][event.skill] > 1 then
                    local found = false
                    local start = below(r, #members)
                    for step = 0, #members - 1 do
                        pace()
                        local other = events[members[(start + step) % #members + 1]]
                        if other.persona ~= event.persona and other.skill ~= event.skill
                            and (known[event.persona][other.skill] or 0) == 0
                            and (known[other.persona][event.skill] or 0) == 0 then
                            local mine, theirs = event.skill, other.skill
                            assign(event, theirs)
                            assign(other, mine)
                            found = true
                            break
                        end
                    end
                    if not found then
                        repaired = false
                        break
                    end
                end
            end
            if repaired then
                break
            end
            -- Put this band back in its stock order before the next attempt.
            for _, index in members do
                pace()
                assign(events[index], events[index].stock)
            end
            attempt += 1
        end
    end
    local changed = 0
    local lastPersona = -1
    for _, event in events do
        pace()
        local at = event.persona * GROWTH_STRIDE + 6 + event.slot * 4 + 2
        buffer.writeu16(growth, at, event.skill)
        if event.stock ~= event.skill then
            changed += 1
        end
        if event.persona ~= lastPersona then
            lastPersona = event.persona
            table.insert(log, string.format("R1 persona %03d %s", event.persona,
                readName(stock.MSG3, event.persona, PERSONA_NAME_STRIDE)))
        end
        table.insert(log, string.format("R1   lv%02d %-9s %03x %-18s -> %03x %s", event.level, event.role, event.stock,
            readName(stock.MSG1, event.stock, SKILL_NAME_STRIDE), event.skill,
            readName(stock.MSG1, event.skill, SKILL_NAME_STRIDE)))
    end
    return changed, #events
end

-- Redistributes five values within their original total, each between 50% and
-- 150% of its original (and 1..99 for stats, never above 255 for weights), by
-- seeded shares; rounding is reconciled in ascending stat order.
local function redistribute(values: {number}, shares: {number}, low: number, high: number): {number}
    local total, floorSum, shareSum = 0, 0, 0
    local lo, hi = {}, {}
    for i = 1, 5 do
        total += values[i]
        lo[i] = math.max(low, math.ceil(values[i] / 2))
        hi[i] = math.min(high, math.floor(values[i] * 3 / 2))
        if values[i] < low then
            lo[i], hi[i] = values[i], values[i]
        end
        floorSum += lo[i]
        shareSum += shares[i]
    end
    local result = table.clone(lo)
    local remaining = total - floorSum
    if remaining > 0 then
        for i = 1, 5 do
            local part = shareSum > 0 and (remaining * shares[i]) // shareSum or remaining // 5
            result[i] = math.min(hi[i], result[i] + part)
        end
        local left = total
        for i = 1, 5 do
            left -= result[i]
        end
        for i = 1, 5 do
            local add = math.min(left, hi[i] - result[i])
            result[i] += add
            left -= add
        end
        assert(left == 0, "redistribution bounds cannot hold the total")
    end
    return result
end

-- R2: base stats (PERSONA0 +4..+8) and level-up growth weights (PERSONA1
-- +0..+4) follow one seeded share vector per persona.
local function generateStats(def: Definition, stock: Stock, meta: buffer, growth: buffer, eligible: {number},
                             log: {string}): (number, number)
    local changed = 0
    for _, id in eligible do
        pace()
        local r = stream(def.seed, string.format("persona-stats/%04x", id))
        local shares: {number} = {}
        for i = 1, 5 do
            shares[i] = below(r, 1001)
        end
        local stats, weights = {}, {}
        for i = 1, 5 do
            stats[i] = buffer.readu8(stock.PERSONA0, id * META_STRIDE + 3 + i)
            weights[i] = buffer.readu8(stock.PERSONA1, id * GROWTH_STRIDE + i - 1)
        end
        local newStats = redistribute(stats, shares, 1, 99)
        local newWeights = redistribute(weights, shares, 0, 255)
        local differs = false
        for i = 1, 5 do
            buffer.writeu8(meta, id * META_STRIDE + 3 + i, newStats[i])
            buffer.writeu8(growth, id * GROWTH_STRIDE + i - 1, newWeights[i])
            differs = differs or newStats[i] ~= stats[i] or newWeights[i] ~= weights[i]
        end
        if differs then
            changed += 1
        end
        table.insert(log, string.format("R2 persona %03d %-16s St/Ma/En/Ag/Lu %2d/%2d/%2d/%2d/%2d -> %2d/%2d/%2d/%2d/%2d"
            .. " growth %d/%d/%d/%d/%d -> %d/%d/%d/%d/%d", id, readName(stock.MSG3, id, PERSONA_NAME_STRIDE),
            stats[1], stats[2], stats[3], stats[4], stats[5], newStats[1], newStats[2], newStats[3], newStats[4],
            newStats[5], weights[1], weights[2], weights[3], weights[4], weights[5], newWeights[1], newWeights[2],
            newWeights[3], newWeights[4], newWeights[5]))
    end
    return changed, #eligible
end

-- R3: whole affinity profiles are permuted among eligible personas of the same
-- ten-level base band, then each profile's Phys/Fire/Ice/Elec/Wind columns and
-- its Light/Dark columns are permuted among themselves. Almighty and ailment
-- columns move only with their profile, so every response count is retained.
local DAMAGE_COLUMNS: {number} = {0, 1, 2, 3, 4}
local KO_COLUMNS: {number} = {6, 7}
local COLUMN_NAMES: {string} = {"Phys", "Fire", "Ice", "Elec", "Wind", "Almi", "Light", "Dark"}

local function generateAffinities(def: Definition, stock: Stock, affinity: buffer, eligible: {number},
                                  log: {string}): (number, number)
    local classes: {[number]: {number}} = {}
    local classKeys: {number} = {}
    for _, id in eligible do
        pace()
        local key = (buffer.readu8(stock.PERSONA0, id * META_STRIDE + 3) - 1) // 10
        local members = classes[key]
        if members == nil then
            members = {}
            classes[key] = members
            table.insert(classKeys, key)
        end
        table.insert(members, id)
    end
    table.sort(classKeys)
    local source: {[number]: number} = {}
    for _, key in classKeys do
        pace()
        local members = classes[key]
        local donors = table.clone(members)
        shuffle(stream(def.seed, string.format("persona-affinity/class/%02d", key)), donors)
        for i, id in members do
            source[id] = donors[i]
        end
    end
    local changed = 0
    for _, id in eligible do
        pace()
        local donor = source[id]
        local words: {number} = {}
        for column = 0, 15 do
            words[column] = buffer.readu16(stock.UNIT2, donor * AFFINITY_STRIDE + column * 2)
        end
        local r = stream(def.seed, string.format("persona-affinity/columns/%04x", id))
        for _, group in {DAMAGE_COLUMNS, KO_COLUMNS} do
            local values: {number} = {}
            for i, column in group do
                values[i] = words[column]
            end
            shuffle(r, values)
            for i, column in group do
                words[column] = values[i]
            end
        end
        local differs = false
        local parts: {string} = {}
        for column = 0, 15 do
            local old = buffer.readu16(stock.UNIT2, id * AFFINITY_STRIDE + column * 2)
            buffer.writeu16(affinity, id * AFFINITY_STRIDE + column * 2, words[column])
            differs = differs or old ~= words[column]
            if column <= 7 then
                table.insert(parts, string.format("%s %04x>%04x", COLUMN_NAMES[column + 1], old, words[column]))
            end
        end
        if differs then
            changed += 1
        end
        table.insert(log, string.format("R3 persona %03d %-16s from %03d: %s", id,
            readName(stock.MSG3, id, PERSONA_NAME_STRIDE), donor, table.concat(parts, " ")))
    end
    return changed, #eligible
end

-------------------------------------------------------------------------------
-- R4: fusion charts and the fusion reachability solver
-------------------------------------------------------------------------------

-- FUSION0..3 hold cells (1,1)..(22,22) of the 24-byte-row arcana charts the
-- game reads as [low * 24 + high]: the pair chart, the alternate pair chart
-- and the two triangle-fusion stage charts. GATES is the 23-entry fusion gate
-- list at 0x006420a0 (s32 story flag, u16 persona, u16 flags; flags bit 1
-- admits the persona only once its flag is set).
local ARCANA_MAX = 22
local CHART_BYTES = 526
local GATE_COUNT = 23
local function cell(a: number, b: number): number
    return a * 24 + b - 25
end

-- The ordinary off-diagonal cells a chart may permute: a result that is not
-- "no fusion" and not one of its own two arcanas (stock self-results stay).
local function chartCells(chart: buffer): {number}
    local cells: {number} = {}
    for a = 1, ARCANA_MAX do
        pace()
        for b = a + 1, ARCANA_MAX do
            local value = buffer.readu8(chart, cell(a, b))
            if value ~= 0 and value ~= a and value ~= b then
                table.insert(cells, a * 32 + b)
            end
        end
    end
    return cells
end

-- One seeded constrained permutation of a chart's cells: returns the source
-- cell for every cell, every result (in each chart sharing the mapping) still
-- differing from both of its cell's arcanas; a violating cell is repaired by a
-- swap inside the chart, and nil means no repair exists for this draw.
local function permuteCells(r: Rng, charts: {buffer}, cells: {number}): {[number]: number}?
    local source = table.clone(cells)
    shuffle(r, source)
    local function fits(target: number, from: number): boolean
        for _, chart in charts do
            local value = buffer.readu8(chart, cell(from // 32, from % 32))
            if value == target // 32 or value == target % 32 then
                return false
            end
        end
        return true
    end
    for i, target in cells do
        pace()
        if not fits(target, source[i]) then
            local found = false
            local start = below(r, #cells)
            for step = 0, #cells - 1 do
                local j = (start + step) % #cells + 1
                if j ~= i and fits(target, source[j]) and fits(cells[j], source[i]) then
                    source[i], source[j] = source[j], source[i]
                    found = true
                    break
                end
            end
            if not found then
                return nil
            end
        end
    end
    local mapping: {[number]: number} = {}
    for i, target in cells do
        mapping[target] = source[i]
    end
    return mapping
end

local function applyCells(stockChart: buffer, out: buffer, mapping: {[number]: number})
    for target, from in mapping do
        pace()
        buffer.writeu8(out, cell(target // 32, target % 32), buffer.readu8(stockChart, cell(from // 32, from % 32)))
    end
end

-- The fusion evaluator of fclFusePair (0x003124a0) without special recipes:
-- the chart's arcana, then fclFuseLowestCandidateAtLeast (0x003130e0) or, for
-- two Personas of the result's own arcana, fclFuseLowestCandidateAtMost
-- (0x003133b0), with the same flag/gate rules, ID-order tie breaking, the two
-- ingredients excluded and the retail fallback to the highest candidate.
export type FusionModel = {
    arcana: {[number]: number},
    level: {[number]: number},
    candidates: {[number]: {number}}, -- per arcana, ascending ID
    shuffle: {number}, -- Shuffle Time pool members (flags & 0xDB == 0), ascending ID
    -- [arcana * 128 + level]: what fclFuseLowestCandidateAtLeast picks with
    -- nothing excluded (the lowest candidate at that level or above, the
    -- highest when none is), 0 for an arcana without candidates.
    atLeast: {[number]: number},
}

local function fusionModel(stock: Stock): FusionModel
    local gated: {[number]: boolean} = {}
    for i = 0, GATE_COUNT - 1 do
        local persona = buffer.readu16(stock.GATES, i * 8 + 4)
        if band(buffer.readu16(stock.GATES, i * 8 + 6), 2) ~= 0 then
            gated[persona] = true -- story-locked: absent from the conservative model
        end
    end
    local model: FusionModel = {arcana = {}, level = {}, candidates = {}, shuffle = {}, atLeast = {}}
    for id = 1, 0xbf do
        pace()
        local row = id * META_STRIDE
        local flags = buffer.readu16(stock.PERSONA0, row)
        local arcana = buffer.readu8(stock.PERSONA0, row + 2)
        local level = buffer.readu8(stock.PERSONA0, row + 3)
        model.arcana[id], model.level[id] = arcana, level
        if arcana >= 1 and arcana <= ARCANA_MAX and band(flags, 3) == 0 and not gated[id] then
            local list = model.candidates[arcana]
            if list == nil then
                list = {}
                model.candidates[arcana] = list
            end
            table.insert(list, id)
        end
        if band(flags, 0xdb) == 0 and arcana >= 1 and level >= 1 and level <= 99 then
            table.insert(model.shuffle, id)
        end
    end
    for arcana, list in model.candidates do
        for level = 0, 127 do
            pace()
            local selected, highest = 0, 0
            for _, id in list do
                local l = model.level[id]
                if highest == 0 or model.level[highest] < l then
                    highest = id
                end
                if l >= level and (selected == 0 or l < model.level[selected]) then
                    selected = id
                end
            end
            model.atLeast[arcana * 128 + level] = if selected ~= 0 then selected else highest
        end
    end
    return model
end

local function fusePair(model: FusionModel, chart: buffer, p: number, q: number): number
    local a, b = model.arcana[p], model.arcana[q]
    if a < 1 or a > ARCANA_MAX or b < 1 or b > ARCANA_MAX then
        return 0
    end
    local result = buffer.readu8(chart, cell(math.min(a, b), math.max(a, b)))
    local list = model.candidates[result]
    if result == 0 or list == nil then
        return 0
    end
    local level = (model.level[p] + model.level[q]) // 2 + 1
    local levels = model.level
    local selected, highest = 0, 0
    local down = result == a and result == b
    for _, id in list do
        local l = levels[id]
        if highest == 0 or levels[highest] < l then
            highest = id
        end
        if id ~= p and id ~= q then
            if down then
                if l <= level and (selected == 0 or levels[selected] < l) then
                    selected = id
                end
            elseif l >= level and (selected == 0 or l < levels[selected]) then
                selected = id
            end
        end
    end
    if selected == 0 and highest ~= 0 and levels[highest] < level then
        selected = highest
    end
    return selected
end

-- The triangle evaluator of fclFuseTriple (0x003127e0) without special recipes:
-- fclFuseTripleArcana orders the three by level, highest first and a tie to the
-- lower arcana (compare-swaps (0,2), (0,1), (1,2), the pairs at 0x007639c0),
-- reads the two lower ones' arcanas in the triangle chart (FUSION2) and that
-- result with the highest one's arcana in the second-stage chart (FUSION3);
-- then fclFuseLowestCandidateAtLeast at the mean level plus five, the three
-- ingredients excluded.
local function tripleArcana(model: FusionModel, first: buffer, second: buffer, p: number, q: number, r: number): number
    local level, arcana = model.level, model.arcana
    -- Each compare-swap moves the left one behind when its level is lower, or
    -- equal with a higher arcana.
    local lx, ly, lz, a, b, c = level[p], level[q], level[r], arcana[p], arcana[q], arcana[r]
    if lx < lz or (lx == lz and a > c) then
        lx, lz, a, c = lz, lx, c, a
    end
    if lx < ly or (lx == ly and a > b) then
        lx, ly, a, b = ly, lx, b, a
    end
    if ly < lz or (ly == lz and b > c) then
        b, c = c, b
    end
    if a < 1 or a > ARCANA_MAX or b < 1 or b > ARCANA_MAX or c < 1 or c > ARCANA_MAX then
        return 0
    end
    local middle = buffer.readu8(first, cell(math.min(b, c), math.max(b, c)))
    if middle == 0 then
        return 0
    end
    return buffer.readu8(second, cell(math.min(a, middle), math.max(a, middle)))
end

local function fuseTriple(model: FusionModel, first: buffer, second: buffer, p: number, q: number, r: number): number
    local result = tripleArcana(model, first, second, p, q, r)
    local list = model.candidates[result]
    if result == 0 or list == nil then
        return 0
    end
    local levels = model.level
    local target = band((levels[p] + levels[q] + levels[r]) // 3 + 5, 0xff)
    local selected, highest = 0, 0
    for _, id in list do
        local l = levels[id]
        if highest == 0 or levels[highest] < l then
            highest = id
        end
        if id ~= p and id ~= q and id ~= r and l >= target and (selected == 0 or l < levels[selected]) then
            selected = id
        end
    end
    if selected == 0 and highest ~= 0 and levels[highest] < target then
        selected = highest
    end
    return selected
end

-- Progression milestones: the level caps of the dungeons' bosses, and the end.
local MILESTONES: {number} = {10, 20, 30, 40, 50, 60, 70, 80, 99}

-- Fusion closure from the protagonist's starter and the Shuffle Time pool: for
-- each Persona the first milestone at which it is obtainable, never fusing
-- above the milestone's level cap. Pair fusions, and with the two triangle
-- charts also triangle fusions; a triangle result reads model.atLeast, so a
-- pick the game would skip because it is an ingredient (already obtained)
-- counts that ingredient instead of the next candidate: a lower bound.
local function fusionReach(model: FusionModel, chart: buffer, first: buffer?, second: buffer?,
                           witness: {[number]: {number}}?): {[number]: number}
    local reached: {[number]: number} = {}
    local order: {number} = {}
    local waiting: {[number]: boolean} = {}
    local levels, arcanas, atLeast = model.level, model.arcana, model.atLeast
    -- The triangle charts as [a * 32 + b] for either order, so the closure's
    -- inner loop (one pass per triple) reads them without calls.
    local stage1: {[number]: number}?, stage2: {[number]: number}? = nil, nil
    if first and second then
        local one: {[number]: number}, two: {[number]: number} = {}, {}
        for a = 1, ARCANA_MAX do
            pace()
            for b = 1, ARCANA_MAX do
                local at = cell(math.min(a, b), math.max(a, b))
                one[a * 32 + b], two[a * 32 + b] = buffer.readu8(first, at), buffer.readu8(second, at)
            end
        end
        stage1, stage2 = one, two
    end
    for milestone, cap in MILESTONES do
        local queue: {number} = {}
        local function offer(id: number, from: {number}?)
            if reached[id] == nil and levels[id] <= cap then
                reached[id] = milestone
                waiting[id] = nil
                table.insert(queue, id)
                if witness and from then
                    witness[id] = from
                end
            elseif reached[id] == nil then
                waiting[id] = true
            end
        end
        offer(1)
        for _, id in model.shuffle do
            pace()
            offer(id)
        end
        local held: {number} = {}
        for id in waiting do
            pace()
            table.insert(held, id)
        end
        table.sort(held)
        for _, id in held do
            pace()
            offer(id)
        end
        local head = 1
        while head <= #queue do
            local p = queue[head]
            head += 1
            local lp, ap = levels[p], arcanas[p]
            for i, q in order do
                pace()
                local result = fusePair(model, chart, p, q)
                if result ~= 0 and reached[result] == nil then
                    offer(result, if witness then {p, q} else nil)
                end
                if stage1 and stage2 then
                    -- tripleArcana inlined: the three compare-swaps on (level, arcana).
                    local lq, aq = levels[q], arcanas[q]
                    for j = i + 1, #order do
                        if j % 64 == 0 then
                            pace()
                        end
                        local r = order[j]
                        local lr = levels[r]
                        local l1, a1, l2, a2, l3, a3 = lp, ap, lq, aq, lr, arcanas[r]
                        if l1 < l3 or (l1 == l3 and a1 > a3) then
                            l1, l3, a1, a3 = l3, l1, a3, a1
                        end
                        if l1 < l2 or (l1 == l2 and a1 > a2) then
                            l1, l2, a1, a2 = l2, l1, a2, a1
                        end
                        if l2 < l3 or (l2 == l3 and a2 > a3) then
                            a2, a3 = a3, a2
                        end
                        local middle = stage1[a2 * 32 + a3]
                        if middle and middle ~= 0 then
                            local arcana = stage2[a1 * 32 + middle]
                            if arcana and arcana ~= 0 then
                                local id = atLeast[arcana * 128 + (lp + lq + lr) // 3 + 5]
                                if id and id ~= 0 and reached[id] == nil then
                                    offer(id, if witness then {p, q, r} else nil)
                                end
                            end
                        end
                    end
                end
            end
            table.insert(order, p)
        end
    end
    return reached
end

-- A generated chart must keep, at every milestone, every arcana the stock
-- charts reach by then, and every Persona the stock charts ever reach: the pair
-- chart by pair fusions alone, then the two triangle charts with it.
local function reachVerdict(model: FusionModel, stockReach: {[number]: number}, newReach: {[number]: number}): string?
    local earliest: {[number]: number} = {}
    for id, at in newReach do
        pace()
        local arcana = model.arcana[id]
        if earliest[arcana] == nil or at < earliest[arcana] then
            earliest[arcana] = at
        end
    end
    for id, milestone in stockReach do
        pace()
        if newReach[id] == nil then
            return string.format("Persona %03d is no longer obtainable", id)
        end
        local arcana = model.arcana[id]
        if earliest[arcana] == nil or earliest[arcana] > milestone then
            return string.format("arcana %d is not obtainable by level %d", arcana, MILESTONES[milestone])
        end
    end
    return nil
end

local function generateFusion(def: Definition, stock: Stock, layer: {[string]: buffer}, log: {string}): (number, number)
    local model = fusionModel(stock)
    local stockReach = fusionReach(model, stock.FUSION0)
    local stockFull = fusionReach(model, stock.FUSION0, stock.FUSION2, stock.FUSION3)
    local newFull = stockFull
    local changed, total = 0, 0
    local plans: {{key: string, label: string, shared: string?}} = {
        {key = "FUSION0", label = "pair", shared = "FUSION1"},
        {key = "FUSION2", label = "triangle"},
        {key = "FUSION3", label = "triangle-stage2"},
    }
    for _, plan in plans do
        local stockChart = stock[plan.key]
        local cells = chartCells(stockChart)
        local attempt = 0
        while true do
            if attempt >= MAX_ATTEMPTS then
                error("R4: no " .. plan.label .. " chart keeps fusion reachability after 256 attempts")
            end
            local sharing = {stockChart}
            if plan.shared then
                table.insert(sharing, stock[plan.shared :: string])
            end
            local mapping = permuteCells(stream(def.seed, string.format("fusion/%s/attempt/%d", plan.label, attempt)),
                sharing, cells)
            local verdict: string? = "the permutation has no valid repair"
            local out = buffer.create(CHART_BYTES)
            buffer.copy(out, 0, stockChart)
            if mapping then
                applyCells(stockChart, out, mapping)
                verdict = nil
                if plan.key == "FUSION0" then
                    verdict = reachVerdict(model, stockReach, fusionReach(model, out))
                elseif plan.key == "FUSION3" then
                    newFull = fusionReach(model, layer.FUSION0, layer.FUSION2, out)
                    verdict = reachVerdict(model, stockFull, newFull)
                end
            end
            if verdict == nil then
                layer[plan.key] = out
                local shared = plan.shared
                if shared then
                    -- The alternate pair chart follows the same cell mapping.
                    local alt = buffer.create(CHART_BYTES)
                    buffer.copy(alt, 0, stock[shared])
                    applyCells(stock[shared], alt, mapping :: {[number]: number})
                    layer[shared] = alt
                end
                break
            end
            table.insert(log, string.format("R4 %s chart attempt %d rejected: %s", plan.label, attempt, verdict :: string))
            attempt += 1
        end
        for _, target in cells do
            pace()
            local at = cell(target // 32, target % 32)
            local old, new = buffer.readu8(stockChart, at), buffer.readu8(layer[plan.key], at)
            total += 1
            if old ~= new then
                changed += 1
                table.insert(log, string.format("R4 %-15s arcana %02d x %02d: %02d -> %02d", plan.label, target // 32,
                    target % 32, old, new))
            end
        end
    end
    for milestone, cap in MILESTONES do
        local stockCount, newCount = 0, 0
        for _, at in stockFull do
            pace()
            if at <= milestone then
                stockCount += 1
            end
        end
        for _, at in newFull do
            pace()
            if at <= milestone then
                newCount += 1
            end
        end
        table.insert(log, string.format("R4 obtainable by level %2d: %3d Personas (stock %3d)", cap, newCount, stockCount))
    end
    return changed, total
end

-------------------------------------------------------------------------------
-- R5: enemy placement (encounter-table formations) and R8: chest contents
-------------------------------------------------------------------------------

-- ENCOUNT0 formations: 24-byte rows, five enemy IDs (u16) from +8. ENCOUNT3
-- weighted encounter tables (Encount_PickEncounter 0x00161630): 0xF8-byte
-- rows, normal/special/rare rates, then two day variants of 30 (formation u16,
-- weight u16) entries: 0..19 normal, 20..24 special, 25..29 rare. ENCOUNT4
-- floor treasure tables (Encount_FindRecord 0x00161c80): 0x15C-byte blocks of
-- 29 twelve-byte records (weight u16, item u16, then chest flags). UNIT0
-- enemy rows: level u8 +3. ITEMS (the 0x007642a0 item table, 68-byte rows):
-- kind u16 +0, price u32 +0x34.
local FORMATION_STRIDE = 24
local ENCOUNTER_TABLE_STRIDE = 0xf8
local ENCOUNTER_SLOTS = 30
local TREASURE_BLOCK = 0x15c
local TREASURE_RECORDS = 29
local TREASURE_STRIDE = 12
local ENEMY_STRIDE = 60
local ENCOUNTER_GROUP_BYTES = 0x12c -- datUnitCreateEncounterGroup's allocation: header and six units
local ENEMY_NAME_STRIDE = 21
local ITEM_STRIDE = 68
local ITEM_ROWS = 1536
local ITEMS_POINTER = 0x007642a0

-- The highest enemy level of a formation, or nil for an empty or unknown one.
-- Facts derived from one set of stock banks, memoized per Stock table: pure
-- functions of the banks, so caching never changes a result.
type Derived = {levels: {[number]: number | false}, enemies: {[number]: {number}}, ordinary: {[number]: boolean}?}
local derivedCache: {[Stock]: Derived} = setmetatable({}, {__mode = "k"}) :: any
local function derived(stock: Stock): Derived
    local known = derivedCache[stock]
    if known == nil then
        local fresh: Derived = {levels = {}, enemies = {}, ordinary = nil}
        derivedCache[stock] = fresh
        return fresh
    end
    return known
end

local function computeFormationLevel(stock: Stock, formation: number): number?
    local formations, enemies = stock.ENCOUNT0, stock.UNIT0
    if (formation + 1) * FORMATION_STRIDE > buffer.len(formations) then
        return nil
    end
    local level: number? = nil
    for slot = 0, 4 do
        local enemy = buffer.readu16(formations, formation * FORMATION_STRIDE + 8 + slot * 2)
        if enemy ~= 0 then
            if (enemy + 1) * ENEMY_STRIDE > buffer.len(enemies) then
                return nil
            end
            local enemyLevel = buffer.readu8(enemies, enemy * ENEMY_STRIDE + 3)
            if enemyLevel == 0 then
                return nil
            end
            level = math.max(level or 0, enemyLevel)
        end
    end
    return level
end

local function formationLevel(stock: Stock, formation: number): number?
    local levels = derived(stock).levels
    local level = levels[formation]
    if level == nil then
        level = computeFormationLevel(stock, formation) or false
        levels[formation] = level
    end
    return if level then level else nil
end

local function formationText(stock: Stock, formation: number): string
    local names: {string} = {}
    for slot = 0, 4 do
        local enemy = buffer.readu16(stock.ENCOUNT0, formation * FORMATION_STRIDE + 8 + slot * 2)
        if enemy ~= 0 then
            table.insert(names, readName(stock.MSG2, enemy, ENEMY_NAME_STRIDE))
        end
    end
    return table.concat(names, ", ")
end

-- Entries whose formation may move: a non-empty, known formation; grouped by
-- slot category and ten-level band so formations stay normal/special/rare
-- (placementFits then keeps each move within three levels of the slot).
local function placementGroups(stock: Stock): ({[string]: {number}}, {string})
    local groups: {[string]: {number}} = {}
    local keys: {string} = {}
    local tables = buffer.len(stock.ENCOUNT3) // ENCOUNTER_TABLE_STRIDE
    for t = 0, tables - 1 do
        for variant = 0, 1 do
            pace()
            for slot = 0, ENCOUNTER_SLOTS - 1 do
                pace()
                local at = t * ENCOUNTER_TABLE_STRIDE + 8 + variant * ENCOUNTER_SLOTS * 4 + slot * 4
                local formation = buffer.readu16(stock.ENCOUNT3, at)
                local level = if formation ~= 0 then formationLevel(stock, formation) else nil
                if level then
                    local category = if slot < 20 then "normal" elseif slot < 25 then "special" else "rare"
                    local key = string.format("%s/%02d", category, (level - 1) // 10)
                    local members = groups[key]
                    if members == nil then
                        members = {}
                        groups[key] = members
                        table.insert(keys, key)
                    end
                    table.insert(members, at)
                end
            end
        end
    end
    table.sort(keys)
    return groups, keys
end

-- Treasure items that may move: an ordinary item kind (0..3: weapons, armor,
-- accessories, consumables) with a shop price; grouped by kind and price tier
-- so a chest keeps its class, flags, weight and approximate value.
local PRICE_TIERS: {number} = {100, 500, 2000, 8000, 32000}
local function itemTier(stock: Stock, item: number): string?
    if item <= 0 or item >= ITEM_ROWS or (item + 1) * ITEM_STRIDE > buffer.len(stock.ITEMS) then
        return nil
    end
    local kind = buffer.readu16(stock.ITEMS, item * ITEM_STRIDE)
    local price = buffer.readu32(stock.ITEMS, item * ITEM_STRIDE + 0x34)
    if kind > 3 or price == 0 then
        return nil
    end
    local tier = 0
    for i, limit in PRICE_TIERS do
        if price >= limit then
            tier = i
        end
    end
    return string.format("kind%d/tier%d", kind, tier)
end

local function generateTreasure(def: Definition, stock: Stock, layer: {[string]: buffer}, log: {string}): (number, number)
    local source = stock.ENCOUNT4
    local out = buffer.create(buffer.len(source))
    buffer.copy(out, 0, source)
    local groups: {[string]: {number}} = {}
    local keys: {string} = {}
    for block = 0, buffer.len(source) // TREASURE_BLOCK - 1 do
        pace()
        for record = 0, TREASURE_RECORDS - 1 do
            local at = block * TREASURE_BLOCK + record * TREASURE_STRIDE
            local tier = itemTier(stock, buffer.readu16(source, at + 2))
            if tier then
                local members = groups[tier]
                if members == nil then
                    members = {}
                    groups[tier] = members
                    table.insert(keys, tier)
                end
                table.insert(members, at)
            end
        end
    end
    table.sort(keys)
    local changed, total = 0, 0
    for _, key in keys do
        local members = groups[key]
        local items: {number} = {}
        for i, at in members do
            items[i] = buffer.readu16(source, at + 2)
        end
        shuffle(stream(def.seed, "treasure/" .. key), items)
        for i, at in members do
            pace()
            local old, new = buffer.readu16(source, at + 2), items[i]
            buffer.writeu16(out, at + 2, new)
            total += 1
            if old ~= new then
                changed += 1
                table.insert(log, string.format("R8 floor table %02d record %02d %-12s item %03x -> %03x", at // TREASURE_BLOCK,
                    at % TREASURE_BLOCK // TREASURE_STRIDE, key, old, new))
            end
        end
    end
    layer.ENCOUNT4 = out
    return changed, total
end

-- R6: enemy stats scaled to the destination floor. Each ordinary enemy met in
-- a floor's encounter table (after R5, when on) gets one profile for that
-- table: its HP, SP, attack and five stats scaled from its own level to the
-- level of the enemy the slot held in the original game (the ratio of the
-- ordinary-enemy medians within three levels of each), varied by 90..110%,
-- then held within 75..125% (HP, SP, attack) and 80..120% (stats) of that
-- original enemy; EXP and yen become the original enemy's. With R5 off the
-- original enemy is itself, so only the variation applies. Bosses, story and
-- tutorial fights (formations outside the floor tables) keep stock stats.
-- Profiles are 24-byte records sorted by (table, enemy): table u16, enemy u16,
-- original enemy u16, pad, HP, SP, attack, EXP, yen (u16), five stats (u8), pad.
local PROFILE_KEY = "R6"
local PROFILE_STRIDE = 24
local PROFILE_FIELDS: {{name: string, at: number, size: number, low: number, high: number, out: number}} = {
    {name = "HP", at = 4, size = 2, low = 75, high = 125, out = 8},
    {name = "SP", at = 6, size = 2, low = 75, high = 125, out = 10},
    {name = "ATK", at = 0x3a, size = 2, low = 75, high = 125, out = 12},
    {name = "St", at = 8, size = 1, low = 80, high = 120, out = 18},
    {name = "Ma", at = 9, size = 1, low = 80, high = 120, out = 19},
    {name = "En", at = 10, size = 1, low = 80, high = 120, out = 20},
    {name = "Ag", at = 11, size = 1, low = 80, high = 120, out = 21},
    {name = "Lu", at = 12, size = 1, low = 80, high = 120, out = 22},
}

-- Ordinary enemies: no unit flags, met in a stock weighted encounter table.
local function ordinaryEnemies(stock: Stock): {[number]: boolean}
    local facts = derived(stock)
    if facts.ordinary then
        return facts.ordinary
    end
    local ordinary: {[number]: boolean} = {}
    local tables = buffer.len(stock.ENCOUNT3) // ENCOUNTER_TABLE_STRIDE
    for t = 0, tables - 1 do
        pace()
        for slot = 0, ENCOUNTER_SLOTS * 2 - 1 do
            pace()
            local formation = buffer.readu16(stock.ENCOUNT3, t * ENCOUNTER_TABLE_STRIDE + 8 + slot * 4)
            if formation ~= 0 and formationLevel(stock, formation) then
                for member = 0, 4 do
                    local enemy = buffer.readu16(stock.ENCOUNT0, formation * FORMATION_STRIDE + 8 + member * 2)
                    if enemy ~= 0 and buffer.readu16(stock.UNIT0, enemy * ENEMY_STRIDE) == 0 then
                        ordinary[enemy] = true
                    end
                end
            end
        end
    end
    facts.ordinary = ordinary
    return ordinary
end

local function enemyValue(stock: Stock, enemy: number, field: {at: number, size: number}): number
    local at = enemy * ENEMY_STRIDE + field.at
    return if field.size == 2 then buffer.readu16(stock.UNIT0, at) else buffer.readu8(stock.UNIT0, at)
end

-- A formation's enemies in order, without empty slots.
local function formationEnemies(stock: Stock, formation: number): {number}
    local cache = derived(stock).enemies
    local known = cache[formation]
    if known then
        return known
    end
    local enemies: {number} = {}
    for member = 0, 4 do
        local enemy = buffer.readu16(stock.ENCOUNT0, formation * FORMATION_STRIDE + 8 + member * 2)
        if enemy ~= 0 then
            table.insert(enemies, enemy)
        end
    end
    cache[formation] = enemies
    return enemies
end

-- (table, enemy) -> the enemy the slot originally held, by first appearance:
-- day variant, then slot, then the enemy's position in its formation.
local function profileTargets(stock: Stock, placement: buffer, ordinary: {[number]: boolean}): {{number}}
    local targets: {{number}} = {}
    local tables = buffer.len(stock.ENCOUNT3) // ENCOUNTER_TABLE_STRIDE
    for t = 0, tables - 1 do
        local seen: {[number]: boolean} = {}
        local taken: {[number]: boolean} = {} -- (formation, original) pairs already handled
        local found: {{number}} = {}
        for slot = 0, ENCOUNTER_SLOTS * 2 - 1 do
            pace()
            local at = t * ENCOUNTER_TABLE_STRIDE + 8 + slot * 4
            local formation, original = buffer.readu16(placement, at), buffer.readu16(stock.ENCOUNT3, at)
            local pair = formation * 65536 + original
            if not taken[pair] and formation ~= 0 and original ~= 0 and formationLevel(stock, formation)
                and formationLevel(stock, original) then
                taken[pair] = true
                local now, before = formationEnemies(stock, formation), formationEnemies(stock, original)
                for position, enemy in now do
                    local target = before[math.min(position, #before)]
                    if not seen[enemy] and ordinary[enemy] and ordinary[target] then
                        seen[enemy] = true
                        table.insert(found, {t, enemy, target})
                    end
                end
            end
        end
        table.sort(found, function(a: {number}, b: {number}): boolean
            return a[2] < b[2]
        end)
        pace()
        for _, entry in found do
            table.insert(targets, entry)
        end
    end
    return targets
end

-- Holds a value within low..high percent of the original enemy's (at least 1,
-- at most the field's limit). An original value of 0 has no band: the enemy
-- keeps its own value.
local function clampPercent(value: number, source: number, base: number, low: number, high: number,
                            limit: number): number
    if base == 0 then
        return source
    end
    local least = math.max(1, (base * low + 99) // 100)
    local most = math.min(limit, (base * high) // 100)
    return math.clamp(value, math.min(least, most), most)
end

-- R5 moves a formation into a slot only when it fits there: as many enemies as
-- the slot's original formation, its level (its highest enemy's) within
-- PLACEMENT_LEVELS of that formation's, and each enemy R6 will not scale already
-- inside the R6 budget of the original enemy at its position (75..125%
-- HP/SP/attack, 80..120% stats).
local PLACEMENT_LEVELS = 3
local function placementFits(stock: Stock, original: number, formation: number, scaled: boolean): boolean
    if original == formation then
        return true
    end
    local a, b = formationLevel(stock, original), formationLevel(stock, formation)
    if a == nil or b == nil or math.abs(a - b) > PLACEMENT_LEVELS then
        return false
    end
    local ordinary = ordinaryEnemies(stock)
    local now, before = formationEnemies(stock, formation), formationEnemies(stock, original)
    if #now ~= #before then
        return false
    end
    for position, enemy in now do
        local target = before[position]
        if not (scaled and ordinary[enemy] and ordinary[target]) then
            for _, field in PROFILE_FIELDS do
                local base, value = enemyValue(stock, target, field), enemyValue(stock, enemy, field)
                if base ~= 0 and (value * 100 < base * field.low or value * 100 > base * field.high) then
                    return false
                end
            end
        end
    end
    return true
end

-- Each group's formations start where they are and trade places by random
-- swaps that keep both formations fitting their new slots, so every
-- intermediate placement (and the result) is one the validator accepts.
local PLACEMENT_SWAPS = 8
local function generatePlacement(def: Definition, stock: Stock, layer: {[string]: buffer}, log: {string}): (number, number)
    local out = buffer.create(buffer.len(stock.ENCOUNT3))
    buffer.copy(out, 0, stock.ENCOUNT3)
    local scaled = band(def.options, lsh(1, 6 - 1)) ~= 0
    local fits: {[number]: boolean} = {}
    local function fit(original: number, formation: number): boolean
        local key = original * 65536 + formation
        local known = fits[key]
        if known == nil then
            known = placementFits(stock, original, formation, scaled)
            fits[key] = known
        end
        return known
    end
    local groups, keys = placementGroups(stock)
    local changed, total = 0, 0
    for _, key in keys do
        local members = groups[key]
        local originals: {number} = {}
        local formations: {number} = {}
        for i, at in members do
            originals[i] = buffer.readu16(stock.ENCOUNT3, at)
            formations[i] = originals[i]
        end
        local r = stream(def.seed, "enemy-placement/" .. key)
        local n = #members
        if n > 1 then
            for _ = 1, n * PLACEMENT_SWAPS do
                pace()
                local i, j = below(r, n) + 1, below(r, n) + 1
                local fi, fj = formations[i], formations[j]
                if fi ~= fj and fit(originals[i], fj) and fit(originals[j], fi) then
                    formations[i], formations[j] = fj, fi
                end
            end
        end
        for i, at in members do
            pace()
            local old, new = originals[i], formations[i]
            buffer.writeu16(out, at, new)
            total += 1
            if old ~= new then
                changed += 1
                local t = at // ENCOUNTER_TABLE_STRIDE
                local within = at % ENCOUNTER_TABLE_STRIDE - 8
                table.insert(log, string.format("R5 table %02d day %d slot %02d %-7s formation %03d (%s) -> %03d (%s)", t,
                    within // (ENCOUNTER_SLOTS * 4), within % (ENCOUNTER_SLOTS * 4) // 4, key, old,
                    formationText(stock, old), new, formationText(stock, new)))
            end
        end
    end
    layer.ENCOUNT3 = out
    return changed, total
end

local function generateProfiles(def: Definition, stock: Stock, layer: {[string]: buffer}, log: {string}): (number, number)
    local ordinary = ordinaryEnemies(stock)
    local targets = profileTargets(stock, layer.ENCOUNT3 or stock.ENCOUNT3, ordinary)
    -- Lower medians of each field over the ordinary enemies within three levels.
    local byLevel: {[number]: {number}} = {}
    for enemy in ordinary do
        pace()
        local level = buffer.readu8(stock.UNIT0, enemy * ENEMY_STRIDE + 3)
        byLevel[level] = byLevel[level] or {}
        table.insert(byLevel[level], enemy)
    end
    local medians: {[number]: number} = {}
    local function median(index: number, field: {at: number, size: number}, level: number): number
        local key = index * 256 + level
        local known = medians[key]
        if known then
            return known
        end
        local values: {number} = {}
        for l = level - 3, level + 3 do
            pace()
            for _, enemy in byLevel[l] or {} do
                table.insert(values, enemyValue(stock, enemy, field))
            end
        end
        table.sort(values)
        local value = values[(#values + 1) // 2] or 0
        medians[key] = value
        return value
    end
    local out = buffer.create(#targets * PROFILE_STRIDE)
    local streams: {[number]: Rng} = {}
    local changed = 0
    for index, target in targets do
        pace()
        local t, enemy, original = target[1], target[2], target[3]
        local r = streams[t]
        if r == nil then
            r = stream(def.seed, string.format("enemy-stats/%02d", t))
            streams[t] = r
        end
        local at = (index - 1) * PROFILE_STRIDE
        buffer.writeu16(out, at, t)
        buffer.writeu16(out, at + 2, enemy)
        buffer.writeu16(out, at + 4, original)
        local from = buffer.readu8(stock.UNIT0, enemy * ENEMY_STRIDE + 3)
        local to = buffer.readu8(stock.UNIT0, original * ENEMY_STRIDE + 3)
        local values: {number} = {}
        local differs = false
        -- Eight variations of 90..110% as base-21 digits of two draws below 21^4.
        local draws = {below(r, 194481), below(r, 194481)}
        for index, field in PROFILE_FIELDS do
            local source, base = enemyValue(stock, enemy, field), enemyValue(stock, original, field)
            local numerator, denominator = median(index, field, to), median(index, field, from)
            local scaled = if denominator > 0 then (source * numerator + denominator // 2) // denominator else source
            local draw = (index - 1) // 4 + 1
            local digit = draws[draw] % 21
            draws[draw] //= 21
            local varied = (scaled * (90 + digit) + 50) // 100
            local value = clampPercent(varied, source, base, field.low, field.high, if field.size == 2 then 0xffff else 99)
            if field.size == 2 then
                buffer.writeu16(out, at + field.out, value)
            else
                buffer.writeu8(out, at + field.out, value)
            end
            differs = differs or value ~= source
            table.insert(values, source)
            table.insert(values, value)
        end
        local exp, money = buffer.readu16(stock.UNIT0, original * ENEMY_STRIDE + 0x1e),
            buffer.readu16(stock.UNIT0, original * ENEMY_STRIDE + 0x20)
        buffer.writeu16(out, at + 14, exp)
        buffer.writeu16(out, at + 16, money)
        if differs then
            changed += 1
        end
        table.insert(log, string.format("R6 table %02d enemy %03d %-20s Lv%d as %03d Lv%d: HP %d>%d SP %d>%d ATK %d>%d "
            .. "St %d>%d Ma %d>%d En %d>%d Ag %d>%d Lu %d>%d EXP %d>%d Yen %d>%d", t, enemy,
            readName(stock.MSG2, enemy, ENEMY_NAME_STRIDE), from, original, to, values[1], values[2], values[3],
            values[4], values[5], values[6], values[7], values[8], values[9], values[10], values[11], values[12],
            values[13], values[14], values[15], values[16], buffer.readu16(stock.UNIT0, enemy * ENEMY_STRIDE + 0x1e),
            exp, buffer.readu16(stock.UNIT0, enemy * ENEMY_STRIDE + 0x20), money))
    end
    layer[PROFILE_KEY] = out
    return changed, #targets
end

-- Validation: records sorted by (table, enemy) and unique, both enemies
-- ordinary, every value inside its clamp around the original enemy, and EXP
-- and yen the original enemy's.
local function validateProfiles(stock: Stock, layer: {[string]: buffer})
    local profiles = layer[PROFILE_KEY]
    local ordinary = ordinaryEnemies(stock)
    local tables = buffer.len(stock.ENCOUNT3) // ENCOUNTER_TABLE_STRIDE
    if buffer.len(profiles) % PROFILE_STRIDE ~= 0 then
        error("validation: R6 records are malformed")
    end
    local previous = -1
    for at = 0, buffer.len(profiles) - PROFILE_STRIDE, PROFILE_STRIDE do
        pace()
        local target = {buffer.readu16(profiles, at), buffer.readu16(profiles, at + 2), buffer.readu16(profiles, at + 4)}
        local key = target[1] * 65536 + target[2]
        if key <= previous or target[1] >= tables or not ordinary[target[2]] or not ordinary[target[3]] then
            error(string.format("validation: R6 record for table %d enemy %d is out of order or not ordinary",
                target[1], target[2]))
        end
        previous = key
        local original = target[3]
        for _, field in PROFILE_FIELDS do
            local source, base = enemyValue(stock, target[2], field), enemyValue(stock, original, field)
            local value = if field.size == 2 then buffer.readu16(profiles, at + field.out)
                else buffer.readu8(profiles, at + field.out)
            if value ~= clampPercent(value, source, base, field.low, field.high, if field.size == 2 then 0xffff else 99) then
                error(string.format("validation: R6 table %d enemy %d %s %d outside its band", target[1], target[2],
                    field.name, value))
            end
        end
        if buffer.readu16(profiles, at + 14) ~= buffer.readu16(stock.UNIT0, original * ENEMY_STRIDE + 0x1e)
            or buffer.readu16(profiles, at + 16) ~= buffer.readu16(stock.UNIT0, original * ENEMY_STRIDE + 0x20) then
            error(string.format("validation: R6 table %d enemy %d rewards are not the slot's", target[1], target[2]))
        end
    end
end

-- R7: ordinary enemy drops. UNIT0 rows (DatEnemyUnitInit) hold four weighted
-- drops (item u16 +0x22 + 4i, weight u8 +0x24 + 4i) that datCalcPickWeightedSkill
-- (0x00243a30) rolls, and one flag-gated special drop (+0x32..+0x36). Only the
-- four ordinary drops of ordinary enemies (no unit flags, met in the weighted
-- encounter tables) move: consumables and materials, within their item kind
-- and the enemy's ten-level band, so every drop stays in the same part of the
-- game; weights, special (quest) drops and flag-gated items 0x400..0x4FF stay.
local DROP_SLOTS = 4
local function dropGroups(stock: Stock): ({[string]: {number}}, {string})
    local ordinary = ordinaryEnemies(stock)
    local groups: {[string]: {number}} = {}
    local keys: {string} = {}
    for enemy = 1, buffer.len(stock.UNIT0) // ENEMY_STRIDE - 1 do
        pace()
        if ordinary[enemy] then
            local level = buffer.readu8(stock.UNIT0, enemy * ENEMY_STRIDE + 3)
            for slot = 0, DROP_SLOTS - 1 do
                local at = enemy * ENEMY_STRIDE + 0x22 + slot * 4
                local item = buffer.readu16(stock.UNIT0, at)
                local kind = if item > 0 and item < ITEM_ROWS then buffer.readu16(stock.ITEMS, item * ITEM_STRIDE) else -1
                if (kind == 3 or kind == 5) and (item < 0x400 or item >= 0x500) then
                    local key = string.format("kind%d/%02d", kind, (level - 1) // 10)
                    if groups[key] == nil then
                        groups[key] = {}
                        table.insert(keys, key)
                    end
                    table.insert(groups[key], at)
                end
            end
        end
    end
    table.sort(keys)
    return groups, keys
end

local function generateDrops(def: Definition, stock: Stock, layer: {[string]: buffer}, log: {string}): (number, number)
    local out = buffer.create(buffer.len(stock.UNIT0))
    buffer.copy(out, 0, stock.UNIT0)
    local groups, keys = dropGroups(stock)
    local changed, total = 0, 0
    for _, key in keys do
        local members = groups[key]
        local items: {number} = {}
        for i, at in members do
            items[i] = buffer.readu16(stock.UNIT0, at)
        end
        shuffle(stream(def.seed, "drops/" .. key), items)
        for i, at in members do
            pace()
            local old, new = buffer.readu16(stock.UNIT0, at), items[i]
            buffer.writeu16(out, at, new)
            total += 1
            if old ~= new then
                changed += 1
                local enemy = at // ENEMY_STRIDE
                table.insert(log, string.format("R7 enemy %03d %-20s drop %d %-8s item %03x -> %03x", enemy,
                    readName(stock.MSG2, enemy, ENEMY_NAME_STRIDE), (at % ENEMY_STRIDE - 0x22) // 4, key, old, new))
            end
        end
    end
    layer.UNIT0 = out
    return changed, total
end

-- R12: ordinary battle music. btlSoundPlayRetailSelectTrack (0x001f73d0) plays
-- the cue the battle formation's selector (ENCOUNT0 u16 +0x16) names: 2 -> 2,
-- 3 -> 3, 6 -> 4, 4 -> 5, 5 -> 6, 7 -> 0x29, 8 -> 0x30, 9 -> 0x2f, 10 -> 0x2d and
-- every other value -> 1. The qualified ordinary pool is the distinct cues of
-- the formations the weighted floor tables hold; bosses, story and special
-- fights outside those tables keep theirs. One stable permutation of the pool
-- per run is written back as those formations' selectors. A pool of one track
-- maps to itself, and the spoiler says so rather than claiming a change.
local SELECTOR_AT = 0x16
local CUE_OF_SELECTOR: {[number]: number} = {[2] = 2, [3] = 3, [6] = 4, [4] = 5, [5] = 6, [7] = 0x29, [8] = 0x30,
                                             [9] = 0x2f, [10] = 0x2d}
local function cueOf(selector: number): number
    return CUE_OF_SELECTOR[selector] or 1
end

-- The formations of the stock floor tables (R5 only moves them between tables).
local function tableFormations(stock: Stock): {number}
    local seen: {[number]: boolean} = {}
    local list: {number} = {}
    for at = 8, buffer.len(stock.ENCOUNT3) - 4, 4 do
        pace()
        if at % ENCOUNTER_TABLE_STRIDE >= 8 then
            local formation = buffer.readu16(stock.ENCOUNT3, at)
            if formation ~= 0 and not seen[formation] and formationLevel(stock, formation) then
                seen[formation] = true
                table.insert(list, formation)
            end
        end
    end
    table.sort(list)
    pace()
    return list
end

local function generateMusic(def: Definition, stock: Stock, layer: {[string]: buffer}, log: {string}): (number, number)
    local formations = tableFormations(stock)
    local selectorOf: {[number]: number} = {} -- cue -> the smallest selector naming it
    local cues: {number} = {}
    for _, formation in formations do
        pace()
        local selector = buffer.readu16(stock.ENCOUNT0, formation * FORMATION_STRIDE + SELECTOR_AT)
        local cue = cueOf(selector)
        if selectorOf[cue] == nil then
            table.insert(cues, cue)
            selectorOf[cue] = selector
        else
            selectorOf[cue] = math.min(selectorOf[cue], selector)
        end
    end
    table.sort(cues)
    if #cues < 2 then
        table.insert(log, string.format("R12 battle music: %d qualified ordinary battle track%s (cue %s) across %d "
            .. "floor-table formations; the pool maps to itself and no battle's music changed", #cues,
            if #cues == 1 then "" else "s", table.concat(cues, ","), #formations))
        return 0, #cues
    end
    local shuffled = table.clone(cues)
    shuffle(stream(def.seed, "battle-music"), shuffled)
    local mapped: {[number]: number} = {}
    for index, cue in cues do
        mapped[cue] = shuffled[index]
        table.insert(log, string.format("R12 battle music cue %02x -> %02x", cue, shuffled[index]))
    end
    local out = buffer.create(buffer.len(stock.ENCOUNT0))
    buffer.copy(out, 0, stock.ENCOUNT0)
    local changed = 0
    for _, formation in formations do
        pace()
        local at = formation * FORMATION_STRIDE + SELECTOR_AT
        local selector = buffer.readu16(stock.ENCOUNT0, at)
        local next = selectorOf[mapped[cueOf(selector)]]
        if next ~= selector then
            buffer.writeu16(out, at, next)
            changed += 1
        end
    end
    layer.ENCOUNT0 = out
    return changed, #formations
end

-- R11: the protagonist's randomized starter. One ordinary Persona of base
-- level 1..3 (an eligible row whose Shuffle Time flags allow it, not Izanagi)
-- is chosen from the run's stream. The story's scripts need Izanagi through the
-- 04/15 awakening and Shadow Yosuke, so Izanagi stays; the starter joins the
-- protagonist's roster at the first field from 04/16 on, through the game's own
-- grant (datPersonaTryAddRosterMember, which builds the record from the run's
-- tables). The grant happens once per run and is recorded in the save block;
-- a load never replays it. The choice is a 2-byte record (persona u16).
local STARTER_KEY = "R11"
local function starterCandidates(stock: Stock, eligible: {number}): {number}
    local list: {number} = {}
    for _, id in eligible do
        local row = id * META_STRIDE
        local level = buffer.readu8(stock.PERSONA0, row + 3)
        if id >= 2 and band(buffer.readu16(stock.PERSONA0, row), 0xdb) == 0 and level >= 1 and level <= 3 then
            table.insert(list, id)
        end
    end
    table.sort(list)
    return list
end

local function generateStarter(def: Definition, stock: Stock, layer: {[string]: buffer}, eligible: {number},
                               log: {string}): (number, number)
    local candidates = starterCandidates(stock, eligible)
    if #candidates == 0 then
        error("R11: this disc has no ordinary level 1..3 Persona to start with")
    end
    local id = candidates[below(stream(def.seed, "starter"), #candidates) + 1]
    local out = buffer.create(2)
    buffer.writeu16(out, 0, id)
    layer[STARTER_KEY] = out
    local names: {string} = {}
    for _, candidate in candidates do
        table.insert(names, string.format("%03d %s", candidate, readName(stock.MSG3, candidate, PERSONA_NAME_STRIDE)))
    end
    table.insert(log, string.format("R11 starter: persona %03d %s (base level %d), granted to the protagonist's roster "
        .. "at the first field from 04/16 on; Izanagi stays for the story. Pool: %s", id,
        readName(stock.MSG3, id, PERSONA_NAME_STRIDE), buffer.readu8(stock.PERSONA0, id * META_STRIDE + 3),
        table.concat(names, ", ")))
    return 1, #candidates
end

-- R9: shop offers. The shops list every item of their type whose unlock holds
-- (yListRebuild 0x002e2a10): equipment's unlock is its gate words (ITEMS +0x3c
-- and +0x40, material id << 8 | count: Daidara unlocks an item once that many of
-- the material were sold; material 0x500 is the opening stock). Within each
-- group of equipment of one item kind, wielder/type flags (+4), users mask
-- (+0x30) and price tier, the material-gated items permute their gates: every
-- material still unlocks exactly one same-tier item for the same wielder, but
-- which one changes. Prices, effects, names, the opening stock (material 0x500),
-- consumables, materials, key items and unsold items (users bit 0x2000) stay.
-- The layer is the ITEMTBL session bank (tables.overlay@2), restricted to those words.
local ITEM_KIND_AT, ITEM_FLAGS_AT, ITEM_USERS_AT, ITEM_PRICE_AT, ITEM_GATES_AT = 0, 4, 0x30, 0x34, 0x3c
local ITEM_GATES_BYTES = 8
local OPENING_STOCK = 0x500
local function shopGroups(stock: Stock): ({[string]: {number}}, {string})
    local items = stock.ITEMS
    local groups: {[string]: {number}} = {}
    local keys: {string} = {}
    for item = 1, ITEM_ROWS - 1 do
        pace()
        local row = item * ITEM_STRIDE
        local kind = buffer.readu32(items, row + ITEM_KIND_AT)
        local price = buffer.readu32(items, row + ITEM_PRICE_AT)
        local users = buffer.readu32(items, row + ITEM_USERS_AT)
        local material = band(bit32.rshift(buffer.readu32(items, row + ITEM_GATES_AT), 8), 0xfff)
        if kind <= 2 and price > 0 and band(users, 0x2000) == 0 and material ~= OPENING_STOCK then
            local tier = 0
            for i, limit in PRICE_TIERS do
                if price >= limit then
                    tier = i
                end
            end
            local key = string.format("kind%d/%08x/%08x/tier%d", kind, buffer.readu32(items, row + ITEM_FLAGS_AT), users,
                tier)
            if groups[key] == nil then
                groups[key] = {}
                table.insert(keys, key)
            end
            table.insert(groups[key], item)
        end
    end
    table.sort(keys)
    return groups, keys
end

local function gateText(items: buffer, item: number): string
    local first = buffer.readu32(items, item * ITEM_STRIDE + ITEM_GATES_AT)
    local second = buffer.readu32(items, item * ITEM_STRIDE + ITEM_GATES_AT + 4)
    local text = string.format("%03x x%d", band(bit32.rshift(first, 8), 0xfff), band(first, 0xff))
    if band(second, 0xff) ~= 0 then
        text ..= string.format(" + %03x x%d", band(bit32.rshift(second, 8), 0xfff), band(second, 0xff))
    end
    return text
end

local function generateShops(def: Definition, stock: Stock, layer: {[string]: buffer}, log: {string}): (number, number)
    local out = buffer.create(buffer.len(stock.ITEMS))
    buffer.copy(out, 0, stock.ITEMS)
    local groups, keys = shopGroups(stock)
    local changed, total = 0, 0
    for _, key in keys do
        local members = groups[key]
        if #members > 1 then
            local donors = table.clone(members)
            shuffle(stream(def.seed, "shop-gates/" .. key), donors)
            for i, item in members do
                pace()
                local donor = donors[i]
                buffer.copy(out, item * ITEM_STRIDE + ITEM_GATES_AT, stock.ITEMS, donor * ITEM_STRIDE + ITEM_GATES_AT,
                    ITEM_GATES_BYTES)
                total += 1
                if buffer.readstring(stock.ITEMS, donor * ITEM_STRIDE + ITEM_GATES_AT, ITEM_GATES_BYTES)
                    ~= buffer.readstring(stock.ITEMS, item * ITEM_STRIDE + ITEM_GATES_AT, ITEM_GATES_BYTES) then
                    changed += 1
                    table.insert(log, string.format("R9 item %03x %-36s unlock %s -> %s (was item %03x's)", item, key,
                        gateText(stock.ITEMS, item), gateText(stock.ITEMS, donor), donor))
                end
            end
        end
    end
    layer.ITEMTBL = out
    return changed, total
end

-- Validation: only grouped items' gate words move, as a permutation per group.
local function validateShops(stock: Stock, layer: {[string]: buffer})
    local moved = layer.ITEMTBL
    local masked, original = buffer.create(buffer.len(moved)), buffer.create(buffer.len(moved))
    buffer.copy(masked, 0, moved)
    buffer.copy(original, 0, stock.ITEMS)
    local groups, keys = shopGroups(stock)
    for _, key in keys do
        local counts: {[string]: number} = {}
        for _, item in groups[key] do
            pace()
            local at = item * ITEM_STRIDE + ITEM_GATES_AT
            local old, new = buffer.readstring(stock.ITEMS, at, ITEM_GATES_BYTES), buffer.readstring(moved, at, ITEM_GATES_BYTES)
            counts[old] = (counts[old] or 0) + 1
            counts[new] = (counts[new] or 0) - 1
            buffer.fill(masked, at, 0, ITEM_GATES_BYTES)
            buffer.fill(original, at, 0, ITEM_GATES_BYTES)
        end
        for _, count in counts do
            if count ~= 0 then
                error("validation: R9 group " .. key .. " does not permute its unlock gates")
            end
        end
    end
    if buffer.tostring(masked) ~= buffer.tostring(original) then
        error("validation: ITEMTBL changed outside the grouped items' unlock gates")
    end
end

-------------------------------------------------------------------------------
-- Validation: the generated layer against the stock banks and the rules above
-------------------------------------------------------------------------------

local function sameBytes(a: buffer, b: buffer, offset: number, size: number): boolean
    return buffer.readstring(a, offset, size) == buffer.readstring(b, offset, size)
end

local function validate(def: Definition, stock: Stock, layer: {[string]: buffer}, eligible: {number})
    local isEligible: {[number]: boolean} = {}
    for _, id in eligible do
        isEligible[id] = true
    end
    local meta, growth, affinity = layer.PERSONA0, layer.PERSONA1, layer.UNIT2
    for id = 0, PERSONA_ROWS - 1 do
        pace()
        if not isEligible[id] then
            -- Protected and ineligible rows keep every stock byte.
            if meta and not sameBytes(meta, stock.PERSONA0, id * META_STRIDE, META_STRIDE) then
                error(string.format("validation: protected persona %03d metadata changed", id))
            end
            if growth and not sameBytes(growth, stock.PERSONA1, id * GROWTH_STRIDE, GROWTH_STRIDE) then
                error(string.format("validation: protected persona %03d growth changed", id))
            end
            if affinity and not sameBytes(affinity, stock.UNIT2, id * AFFINITY_STRIDE, AFFINITY_STRIDE) then
                error(string.format("validation: protected persona %03d affinity changed", id))
            end
            continue
        end
        if meta then
            -- Flags, arcana, level and the bytes after the stats are unchanged.
            if not sameBytes(meta, stock.PERSONA0, id * META_STRIDE, 4)
                or not sameBytes(meta, stock.PERSONA0, id * META_STRIDE + 9, META_STRIDE - 9) then
                error(string.format("validation: persona %03d identity bytes changed", id))
            end
            local total, newTotal = 0, 0
            for i = 4, 8 do
                local old = buffer.readu8(stock.PERSONA0, id * META_STRIDE + i)
                local new = buffer.readu8(meta, id * META_STRIDE + i)
                total += old
                newTotal += new
                if new < 1 or new > 99 or (old >= 1 and (new * 2 < old or new * 2 > old * 3)) then
                    error(string.format("validation: persona %03d stat %d out of bounds", id, i - 3))
                end
            end
            if total ~= newTotal then
                error(string.format("validation: persona %03d stat total changed", id))
            end
        end
        if growth then
            local weights, newWeights = 0, 0
            for i = 0, 4 do
                weights += buffer.readu8(stock.PERSONA1, id * GROWTH_STRIDE + i)
                newWeights += buffer.readu8(growth, id * GROWTH_STRIDE + i)
            end
            if weights ~= newWeights or buffer.readu8(growth, id * GROWTH_STRIDE + 5)
                ~= buffer.readu8(stock.PERSONA1, id * GROWTH_STRIDE + 5) then
                error(string.format("validation: persona %03d growth budget changed", id))
            end
            -- No duplicate learned skill the stock learnset does not already have.
            local stockCount: {[number]: number} = {}
            local newCount: {[number]: number} = {}
            for slot = 0, LEARN_EVENTS - 1 do
                local at = id * GROWTH_STRIDE + 6 + slot * 4
                -- Levels, kinds and sentinels are stock; each skill keeps its role.
                if not sameBytes(growth, stock.PERSONA1, at, 2) then
                    error(string.format("validation: persona %03d event %d level/kind changed", id, slot))
                end
                local kind = buffer.readi8(growth, at + 1)
                local old, new = buffer.readu16(stock.PERSONA1, at + 2), buffer.readu16(growth, at + 2)
                if kind ~= 1 then
                    if old ~= new then
                        error(string.format("validation: persona %03d non-learn event %d changed", id, slot))
                    end
                else
                    if skillRole(stock, old) ~= skillRole(stock, new) then
                        error(string.format("validation: persona %03d event %d changed role", id, slot))
                    end
                    stockCount[old] = (stockCount[old] or 0) + 1
                    newCount[new] = (newCount[new] or 0) + 1
                end
            end
            for skill, count in newCount do
                if count > 1 and count > (stockCount[skill] or 0) then
                    error(string.format("validation: persona %03d learns skill %03x twice", id, skill))
                end
            end
        end
        if affinity then
            for _, group in {DAMAGE_COLUMNS, KO_COLUMNS} do
                for _, column in group do
                    local new = buffer.readu16(affinity, id * AFFINITY_STRIDE + column * 2)
                    if group == KO_COLUMNS and new == 0x14 or group == DAMAGE_COLUMNS and new == 0xa then
                        error(string.format("validation: persona %03d affinity channel semantics changed", id))
                    end
                end
            end
        end
    end
    -- Fusion charts: only ordinary cells change, each to a result that is not
    -- one of its own arcanas, and every chart keeps its multiset of results.
    for _, key in {"FUSION0", "FUSION1", "FUSION2", "FUSION3"} do
        local chart = layer[key]
        if chart then
            local stockChart = stock[key]
            local movable: {[number]: boolean} = {}
            local counts: {[number]: number} = {}
            for _, target in chartCells(stockChart) do
                pace()
                local at = cell(target // 32, target % 32)
                movable[at] = true
                local old, new = buffer.readu8(stockChart, at), buffer.readu8(chart, at)
                if new == target // 32 or new == target % 32 or new < 1 or new > ARCANA_MAX then
                    error(string.format("validation: %s cell %d x %d holds an invalid result", key, target // 32, target % 32))
                end
                counts[old] = (counts[old] or 0) + 1
                counts[new] = (counts[new] or 0) - 1
            end
            for at = 0, CHART_BYTES - 1 do
                pace()
                if not movable[at] and buffer.readu8(chart, at) ~= buffer.readu8(stockChart, at) then
                    error(string.format("validation: %s byte %d outside the ordinary cells changed", key, at))
                end
            end
            for value, count in counts do
                if count ~= 0 then
                    error(string.format("validation: %s changed how often arcana %d results", key, value))
                end
            end
        end
    end
    -- Encounter tables: only grouped formation IDs move, each group keeps its
    -- multiset; treasure tables likewise for grouped item IDs.
    local function checkMoves(key: string, groups: {[string]: {number}}, keys: {string})
        local moved = layer[key]
        if moved == nil then
            return
        end
        -- Masked copies: with every movable field zeroed both must be identical.
        local maskedMoved, maskedStock = buffer.create(buffer.len(moved)), buffer.create(buffer.len(moved))
        buffer.copy(maskedMoved, 0, moved)
        buffer.copy(maskedStock, 0, stock[key])
        for _, group in keys do
            local counts: {[number]: number} = {}
            for _, at in groups[group] do
                pace()
                local old, new = buffer.readu16(stock[key], at), buffer.readu16(moved, at)
                counts[old] = (counts[old] or 0) + 1
                counts[new] = (counts[new] or 0) - 1
                buffer.writeu16(maskedMoved, at, 0)
                buffer.writeu16(maskedStock, at, 0)
            end
            for value, count in counts do
                if count ~= 0 then
                    error(string.format("validation: %s group %s changed how often %d appears", key, group, value))
                end
            end
        end
        if buffer.tostring(maskedMoved) ~= buffer.tostring(maskedStock) then
            error("validation: " .. key .. " changed outside the moved fields")
        end
    end
    if layer.ENCOUNT3 then
        local groups, keys = placementGroups(stock)
        checkMoves("ENCOUNT3", groups, keys)
        local scaled = band(def.options, lsh(1, 6 - 1)) ~= 0
        for _, key in keys do
            for _, at in groups[key] do
                pace()
                local original, formation = buffer.readu16(stock.ENCOUNT3, at), buffer.readu16(layer.ENCOUNT3, at)
                if not placementFits(stock, original, formation, scaled) then
                    error(string.format("validation: R5 put formation %d where formation %d was, outside its enemy "
                        .. "count, level window or budget", formation, original))
                end
            end
        end
    end
    if layer.UNIT0 then
        local groups, keys = dropGroups(stock)
        checkMoves("UNIT0", groups, keys)
    end
    if layer[PROFILE_KEY] then
        validateProfiles(stock, layer)
    end
    if layer.ITEMTBL then
        validateShops(stock, layer)
    end
    -- R11: one 2-byte record naming an ordinary level 1..3 starter.
    if layer[STARTER_KEY] then
        local starter = layer[STARTER_KEY]
        if buffer.len(starter) ~= 2 or not table.find(starterCandidates(stock, eligible), buffer.readu16(starter, 0)) then
            error("validation: R11 starter is not an ordinary level 1..3 Persona")
        end
    end
    -- R12: only floor-table formations' selectors change, as a bijection of cues.
    if layer.ENCOUNT0 then
        local masked, original = buffer.create(buffer.len(layer.ENCOUNT0)), buffer.create(buffer.len(layer.ENCOUNT0))
        buffer.copy(masked, 0, layer.ENCOUNT0)
        buffer.copy(original, 0, stock.ENCOUNT0)
        local image: {[number]: number} = {}
        local preimage: {[number]: number} = {}
        for _, formation in tableFormations(stock) do
            pace()
            local at = formation * FORMATION_STRIDE + SELECTOR_AT
            local from, to = cueOf(buffer.readu16(stock.ENCOUNT0, at)), cueOf(buffer.readu16(layer.ENCOUNT0, at))
            if (image[from] or to) ~= to or (preimage[to] or from) ~= from then
                error("validation: R12 battle music is not a permutation of the ordinary pool")
            end
            image[from], preimage[to] = to, from
            buffer.writeu16(masked, at, 0)
            buffer.writeu16(original, at, 0)
        end
        if buffer.tostring(masked) ~= buffer.tostring(original) then
            error("validation: ENCOUNT0 changed outside the floor formations' music selectors")
        end
    end
    if layer.ENCOUNT4 then
        local groups: {[string]: {number}} = {}
        local keys: {string} = {}
        for block = 0, buffer.len(stock.ENCOUNT4) // TREASURE_BLOCK - 1 do
            pace()
            for record = 0, TREASURE_RECORDS - 1 do
                local at = block * TREASURE_BLOCK + record * TREASURE_STRIDE + 2
                local tier = itemTier(stock, buffer.readu16(stock.ENCOUNT4, at))
                if tier then
                    if groups[tier] == nil then
                        groups[tier] = {}
                        table.insert(keys, tier)
                    end
                    table.insert(groups[tier], at)
                end
            end
        end
        checkMoves("ENCOUNT4", groups, keys)
    end
    -- Every band's multiset of skills is exactly the stock multiset.
    if growth then
        local counts: {[string]: number} = {}
        for _, id in eligible do
            pace()
            for slot = 0, LEARN_EVENTS - 1 do
                local at = id * GROWTH_STRIDE + 6 + slot * 4
                if buffer.readi8(growth, at + 1) == 0 then
                    break
                end
                local old, new = buffer.readu16(stock.PERSONA1, at + 2), buffer.readu16(growth, at + 2)
                counts[skillRole(stock, old) .. ":" .. tostring(old)] = (counts[skillRole(stock, old) .. ":" .. tostring(old)] or 0) + 1
                counts[skillRole(stock, new) .. ":" .. tostring(new)] = (counts[skillRole(stock, new) .. ":" .. tostring(new)] or 0) - 1
            end
        end
        for key, count in counts do
            pace()
            if count ~= 0 then
                error("validation: the learnable skill multiset changed at " .. key)
            end
        end
    end
end

-------------------------------------------------------------------------------
-- Spoiler log and manifest
-------------------------------------------------------------------------------

-- SHA-256 of LE32(#key) key LE32(#bytes) bytes for each named bank in order,
-- assembled in one buffer (no per-bank string copies for the collector).
local function banksDigest(keys: {string}, banks: {[string]: buffer}): string
    local size = 0
    for _, key in keys do
        local bank = banks[key]
        if bank == nil then
            error("missing bank " .. key)
        end
        size += 8 + #key + buffer.len(bank)
    end
    local message = buffer.create(size)
    local at = 0
    for _, key in keys do
        local bank = banks[key]
        buffer.writeu32(message, at, #key)
        buffer.writestring(message, at + 4, key)
        buffer.writeu32(message, at + 4 + #key, buffer.len(bank))
        buffer.copy(message, at + 8 + #key, bank)
        at += 8 + #key + buffer.len(bank)
    end
    return p4.sha256(message)
end

local function bankHash(layer: {[string]: buffer}): string
    local keys: {string} = {}
    for key in layer do
        table.insert(keys, key)
    end
    table.sort(keys)
    return banksDigest(keys, layer)
end

local function jsonString(text: string): string
    return '"' .. string.gsub(text, '[%c"\\]', function(c: string): string
        return string.format("\\u%04x", string.byte(c))
    end) .. '"'
end

local function seedWords(seed: string): string
    local h = hex(seed)
    return string.format("%s %s %s %s", string.sub(h, 1, 8), string.sub(h, 9, 16), string.sub(h, 17, 24),
        string.sub(h, 25, 32))
end

local function buildSpoiler(result: Result, log: {string}): (string, string)
    local def = result.definition
    local lines: {string} = {
        "P4PC seeded randomizer spoiler log",
        "run code:     " .. runCode(def),
        "run digest:   " .. hex(result.digest),
        "seed:         " .. seedWords(def.seed),
        string.format("algorithm:    %d  catalog: %d  save schema: %d", def.algorithm, def.catalog, SAVE_SCHEMA),
        "features:     " .. optionsText(def.options),
        "input hash:   " .. hex(def.fingerprint) .. "  (" .. table.concat(inputsFor(def.options), " ") .. ")",
        "output hash:  " .. hex(result.layerHash),
        "protected:    persona 001 (protagonist starter and tutorial); party, evolution, enemy-side, placeholder and special rows",
    }
    local manifestCounts: {string} = {}
    for _, feature in FEATURES do
        local count = result.counts[feature.code]
        if count then
            local percent = count.eligible > 0 and count.changed * 100 // count.eligible or 0
            table.insert(lines, string.format("%s %-20s changed %d of %d (%d%%)", feature.code, feature.name,
                count.changed, count.eligible, percent))
            table.insert(manifestCounts, string.format('"%s":{"changed":%d,"eligible":%d}', feature.code,
                count.changed, count.eligible))
        end
    end
    table.insert(lines, "")
    for _, line in log do
        pace()
        table.insert(lines, line)
    end
    table.insert(lines, "")
    local banks: {string} = {}
    for key in result.layer do
        table.insert(banks, jsonString(key))
    end
    table.sort(banks)
    local manifest = string.format(
        '{"mod":%s,"algorithm":%d,"catalog":%d,"save_schema":%d,"run_code":%s,"run_digest":%s,'
            .. '"seed":%s,"options":%d,"features":%s,"input_banks":[%s],"input_hash":%s,"output_banks":[%s],'
            .. '"output_hash":%s,"counts":{%s}}\n',
        jsonString(p4.id), def.algorithm, def.catalog, SAVE_SCHEMA, jsonString(runCode(def)),
        jsonString(hex(result.digest)), jsonString(hex(def.seed)), def.options, jsonString(optionsText(def.options)),
        table.concat((function(): {string}
            local quoted = {}
            for _, key in inputsFor(def.options) do
                table.insert(quoted, jsonString(key))
            end
            return quoted
        end)(), ","), jsonString(hex(def.fingerprint)), table.concat(banks, ","), jsonString(hex(result.layerHash)),
        table.concat(manifestCounts, ","))
    return table.concat(lines, "\n"), manifest
end

local function fingerprint(stock: Stock, options: number): string
    return banksDigest(inputsFor(options), stock)
end

-- The whole run, from its definition and the stock banks. Pure: no game state,
-- clock or platform input; may yield between steps inside a coroutine.
local function generate(def: Definition, stock: Stock): Result
    if def.algorithm ~= ALGORITHM or def.catalog ~= CATALOG then
        error(string.format("this randomizer generates algorithm %d catalog %d, not %d/%d", ALGORITHM, CATALOG,
            def.algorithm, def.catalog))
    end
    for _, key in {"PERSONA0", "PERSONA1", "UNIT2"} do
        local bank = stock[key]
        local stride = key == "PERSONA0" and META_STRIDE or key == "PERSONA1" and GROWTH_STRIDE or AFFINITY_STRIDE
        if bank == nil or buffer.len(bank) < PERSONA_ROWS * stride then
            error("bank " .. key .. " is missing or smaller than 256 persona rows")
        end
    end
    local eligible: {number} = {}
    for id = 1, PERSONA_ROWS - 1 do
        pace()
        if eligiblePersona(stock, id) then
            table.insert(eligible, id)
        end
    end
    local layer: {[string]: buffer} = {}
    local log: {string} = {}
    local counts: {[string]: {changed: number, eligible: number}} = {}
    local function enabled(id: number): boolean
        return band(def.options, lsh(1, id - 1)) ~= 0
    end
    local meta: buffer? = nil
    local growth: buffer? = nil
    if enabled(1) or enabled(2) then
        growth = buffer.create(buffer.len(stock.PERSONA1))
        buffer.copy(growth :: buffer, 0, stock.PERSONA1)
    end
    if enabled(2) then
        meta = buffer.create(buffer.len(stock.PERSONA0))
        buffer.copy(meta :: buffer, 0, stock.PERSONA0)
    end
    if enabled(1) then
        local changed, total = generateSkills(def, stock, growth :: buffer, eligible, log)
        counts.R1 = {changed = changed, eligible = total}
    end
    if enabled(2) then
        local changed, total = generateStats(def, stock, meta :: buffer, growth :: buffer, eligible, log)
        counts.R2 = {changed = changed, eligible = total}
    end
    if enabled(3) then
        local affinity = buffer.create(buffer.len(stock.UNIT2))
        buffer.copy(affinity, 0, stock.UNIT2)
        local changed, total = generateAffinities(def, stock, affinity, eligible, log)
        counts.R3 = {changed = changed, eligible = total}
        layer.UNIT2 = affinity
    end
    if enabled(4) then
        local changed, total = generateFusion(def, stock, layer, log)
        counts.R4 = {changed = changed, eligible = total}
    end
    if enabled(5) then
        local changed, total = generatePlacement(def, stock, layer, log)
        counts.R5 = {changed = changed, eligible = total}
    end
    if enabled(6) then
        local changed, total = generateProfiles(def, stock, layer, log)
        counts.R6 = {changed = changed, eligible = total}
    end
    if enabled(7) then
        local changed, total = generateDrops(def, stock, layer, log)
        counts.R7 = {changed = changed, eligible = total}
    end
    if enabled(8) then
        local changed, total = generateTreasure(def, stock, layer, log)
        counts.R8 = {changed = changed, eligible = total}
    end
    if enabled(9) then
        local changed, total = generateShops(def, stock, layer, log)
        counts.R9 = {changed = changed, eligible = total}
    end
    if enabled(10) then
        table.insert(log, "R10 Shuffle Time cards: each Persona card is drawn live, per offered Shuffle Time, with this "
            .. "run's stream \"shuffle-time/<award ordinal>\" from the same eligible list (owned, above or within the "
            .. "player's level window) the game's card came from")
    end
    if enabled(11) then
        local changed, total = generateStarter(def, stock, layer, eligible, log)
        counts.R11 = {changed = changed, eligible = total}
    end
    if enabled(12) then
        local changed, total = generateMusic(def, stock, layer, log)
        counts.R12 = {changed = changed, eligible = total}
    end
    if meta then
        layer.PERSONA0 = meta
    end
    if growth then
        layer.PERSONA1 = growth
    end
    validate(def, stock, layer, eligible)
    local result: Result = {
        definition = def,
        digest = runDigest(def),
        layer = layer,
        layerHash = bankHash(layer),
        spoiler = "",
        manifest = "",
        counts = counts,
    }
    result.spoiler, result.manifest = buildSpoiler(result, log)
    return result
end

-- The pure engine, also returned by this chunk for headless qualification
-- (tests/mod_randomizer_engine_test.py); the game ignores a chunk's results.
local Engine = {
    algorithm = ALGORITHM,
    catalog = CATALOG,
    features = FEATURES,
    inputBanks = INPUT_BANKS,
    inputsFor = inputsFor,
    sha256 = sha256,
    hex = hex,
    unhex = unhex,
    stream = stream,
    next = rngNext,
    below = below,
    shuffle = shuffle,
    counterAdd = counterAdd,
    counterHex = counterHex,
    canonical = canonical,
    runDigest = runDigest,
    runCode = runCode,
    fingerprint = fingerprint,
    bankHash = bankHash,
    generate = generate,
    eligiblePersona = eligiblePersona,
    skillRole = skillRole,
    fusionModel = fusionModel,
    fusePair = fusePair,
    fuseTriple = fuseTriple,
    fusionReach = fusionReach,
}

-------------------------------------------------------------------------------
-- The mod in the game: settings, generation task, run lifecycle and save block
-------------------------------------------------------------------------------

local function log(message: string)
    p4.log(message)
end

local ZERO_SEED = string.rep("\0", 16)
local function readSeed(): string
    local stored = p4.settings.get("seed")
    if type(stored) == "string" then
        local bytes = unhex(stored)
        if bytes and #bytes == 16 then
            return bytes
        end
        log("RANDO_SEED_INVALID the stored seed is not 32 hexadecimal digits; using the all-zero seed")
    end
    return ZERO_SEED
end

-- The next New Game's options, from the settings; 0 when it is not randomized.
local function settingsOptions(): number
    if p4.settings.get("randomize") ~= true then
        return 0
    end
    local options = 0
    for _, feature in FEATURES do
        if p4.settings.get(feature.key) == true then
            options = bit32.bor(options, lsh(1, feature.id - 1))
        end
    end
    return options
end

-- shuffle: the R10 award ordinal a loaded run continues from.
-- starterGranted: the R11 starter already joined the roster in this save.
type Run = {result: Result, adopted: string, shuffle: {hi: number, lo: number}?, starterGranted: boolean?}
type Task = {definition: Definition, thread: thread, result: Result?, failure: string?}
type Check = {run: Run, thread: thread, export: boolean}
type NewGame = {seed: string, options: number}

local seed = readSeed()
local run: Run? = nil -- the active run, published as this mod's overlay
local task: Task? = nil -- background generation
local check: Check? = nil -- background verification of a loaded run
local newGame: NewGame? = nil -- the armed New Game waiting for its generation
local pendingNewGame = false -- a reset happened and no load followed yet
local pendingSeed: string? = nil -- edited on the page, persisted at the next frame
local exportRequested = false
local statusText: string? = nil
local stock: Stock? = nil
local stockPrints: {[number]: string} = {} -- input fingerprints by options
local inputHashed = false -- the settings' input fingerprint was hashed and logged
local fingerprintWanted = false
local setPending: (pending: boolean) -> ()

local function setStatus(text: string?)
    if statusText ~= text then
        statusText = text
        p4.ui.status(text, "camp")
    end
end

-- This disc's input fingerprint for a feature set, once the stock banks are read.
local function stockPrint(options: number): string?
    local banks = stock
    if banks == nil then
        return nil
    end
    local print = stockPrints[options]
    if print == nil then
        print = fingerprint(banks, options)
        stockPrints[options] = print
    end
    return print
end

local function definitionOf(choice: NewGame): Definition?
    local print = stockPrint(choice.options)
    if print == nil then
        return nil
    end
    return {algorithm = ALGORITHM, catalog = CATALOG, seed = choice.seed, options = choice.options,
            fingerprint = print}
end

-- frame_start runs only while there is background work.
local frameSubscription: P4Subscription
local function wantFrames()
    local working = (task ~= nil and task.result == nil and task.failure == nil) or check ~= nil
        or newGame ~= nil -- an armed New Game publishes in a later frame
        or (fingerprintWanted and (stock == nil or not inputHashed))
    if working or pendingSeed ~= nil or exportRequested then
        frameSubscription:enable()
    else
        frameSubscription:disable()
    end
end

local function startTask(def: Definition)
    local current = task
    if current and canonical(current.definition) == canonical(def) then
        return
    end
    local input = stock :: Stock
    task = {definition = def, thread = coroutine.create(function(): Result
        return generate(def, input)
    end)}
    log("RANDO_GENERATE start code=" .. runCode(def))
    wantFrames()
end

-- Writes the run's spoiler and manifest, then drops their text: the mod's
-- memory limit is shared with everything else it keeps, and an export of a run
-- whose text was dropped regenerates it in the background first.
local function writeArtifacts(result: Result): boolean
    local folder = "runs/" .. string.sub(hex(result.digest), 1, 16)
    local ok, why = p4.artifacts.write(folder .. "/spoiler.txt", result.spoiler)
    if ok then
        ok, why = p4.artifacts.write(folder .. "/manifest.json", result.manifest)
    end
    result.spoiler, result.manifest = "", ""
    if not ok then
        log("RANDO_SPOILER_FAILED " .. tostring(why))
        return false
    end
    log("RANDO_SPOILER " .. tostring(p4.artifacts.folder()) .. "/" .. folder .. "/spoiler.txt")
    return true
end

-- R6 live: the floor's encounter table comes from the encounter picker
-- (Encount_PickEncounter's entry index -> the ENCOUNT1 entry's table); a battle
-- whose formation that table holds gives each of its ordinary enemies the
-- run's profile for (table, enemy) as datUnitInit creates it. The game's stat,
-- attack and reward readers then return the profile's values, scaled by the
-- profile/stock ratio so other mods' adjustments of the same readers compose.
-- Hooks are enabled only while a run with R6 is active; they never call the
-- game's RNG or change which encounter, formation or enemy the game picks.
local FLOOR_ENTRIES_SLOT = 0x0076450c -- ENCOUNT1: 10-byte floor entries, table id u16 +0
local ENCOUNTER_TABLES_SLOT = 0x00764508 -- ENCOUNT3
local ENEMY_ROWS_SLOT = 0x007644b4 -- UNIT0
local function seam(address: number, signature: string): number
    local info = p4.describe(address)
    assert(info and info.signature == signature, string.format("randomizer needs 0x%08x %s", address, signature))
    return address
end
type LiveProfile = {hp: {number}, sp: {number}, attack: {number}, exp: {number}, money: {number}, stats: {{number}}}
local profiles: buffer? = nil
local floorTable: number? = nil
local battleTable: number? = nil
local liveUnits: {[number]: LiveProfile} = {}
local liveCount = 0
local profiledReads = 0 -- reader results a profile changed in the current battle
local readerHooks: {P4Subscription} = {} -- the stat/reward readers, created below
local readersOn = true -- new hooks start enabled
local function setReaders(on: boolean)
    if on ~= readersOn then
        readersOn = on
        for _, hook in readerHooks do
            if on then
                hook:enable()
            else
                hook:disable()
            end
        end
    end
end
local function setLive(unit: number, live: LiveProfile?)
    local before = liveUnits[unit]
    if before == nil and live ~= nil then
        liveCount += 1
    elseif before ~= nil and live == nil then
        liveCount -= 1
    end
    liveUnits[unit] = live
    setReaders(liveCount > 0)
end

-- The record for (table, enemy), by binary search over the sorted records.
local function findProfile(records: buffer, t: number, enemy: number): number?
    local low, high = 0, buffer.len(records) // PROFILE_STRIDE - 1
    local key = t * 65536 + enemy
    while low <= high do
        local middle = (low + high) // 2
        local at = middle * PROFILE_STRIDE
        local here = buffer.readu16(records, at) * 65536 + buffer.readu16(records, at + 2)
        if here == key then
            return at
        elseif here < key then
            low = middle + 1
        else
            high = middle - 1
        end
    end
    return nil
end

local function ratio(value: number, pair: {number}, limit: number): number
    local stock, profile = pair[1], pair[2]
    if stock == 0 or value == 0 then
        return value
    end
    return math.clamp((value * profile + stock // 2) // stock, 1, limit)
end

local profileHooks: {P4Subscription} = {
    p4.hook(seam(0x00161630, "iiiii"), "iiiii", "after", function(context)
        if (context.result :: number) ~= -1 then
            local entries = p4.mem.read("u32", FLOOR_ENTRIES_SLOT)
            floorTable = p4.mem.read("u16", entries + band(context.args[3] :: number, 0xffff) * 10)
        end
    end),
    -- Floors create their Shadows' groups when they spawn, several at a time: each
    -- unit keeps the profile its group was created with.
    p4.hook(seam(0x00231630, "pH"), "pH", "before", function(context)
        battleTable = nil
        local t = floorTable
        if t == nil then
            return
        end
        local formation = context.args[1] :: number
        local row = p4.mem.read_bytes(p4.mem.read("u32", ENCOUNTER_TABLES_SLOT) + t * ENCOUNTER_TABLE_STRIDE + 8,
            ENCOUNTER_SLOTS * 8)
        for slot = 0, ENCOUNTER_SLOTS * 2 - 1 do
            if buffer.readu16(row, slot * 4) == formation then
                battleTable = t
                return
            end
        end
    end),
    p4.hook(seam(0x00231af0, "IpBH"), "IpBH", "after", function(context)
        local unit = context.args[1] :: number
        local enemy = context.args[3] :: number
        local found, t = profiles, battleTable
        local at = if found and t and (context.args[2] :: number) == 1 then findProfile(found, t, enemy) else nil
        if at == nil or found == nil or t == nil then
            setLive(unit, nil) -- a reused unit address never keeps an earlier profile
            return
        end
        local records = found :: buffer
        local row = p4.mem.read_bytes(p4.mem.read("u32", ENEMY_ROWS_SLOT) + enemy * ENEMY_STRIDE, ENEMY_STRIDE)
        local stats: {{number}} = {}
        for i = 0, 4 do
            stats[i + 1] = {buffer.readu8(row, 8 + i), buffer.readu8(records, at + 18 + i)}
        end
        local live: LiveProfile = {
            hp = {buffer.readu16(row, 4), buffer.readu16(records, at + 8)},
            sp = {buffer.readu16(row, 6), buffer.readu16(records, at + 10)},
            attack = {buffer.readu16(row, 0x3a), buffer.readu16(records, at + 12)},
            exp = {buffer.readu16(row, 0x1e), buffer.readu16(records, at + 14)},
            money = {buffer.readu16(row, 0x20), buffer.readu16(records, at + 16)},
            stats = stats,
        }
        setLive(unit, live)
        local hp, sp = p4.mem.read("u16", unit + 8), p4.mem.read("u16", unit + 0xa)
        p4.mem.write("u16", unit + 8, ratio(hp, live.hp, 0xffff))
        p4.mem.write("u16", unit + 0xa, ratio(sp, live.sp, 0xffff))
        log(string.format("RANDO_R6 table=%d enemy=%d hp=%d>%d sp=%d>%d", t, enemy, hp, p4.mem.read("u16", unit + 8),
            sp, p4.mem.read("u16", unit + 0xa)))
    end),
    -- A group's profiles end when the game frees it (datUnitFree: the field unit's
    -- teardown, or the battle's when it ends the group). A floor's Shadows keep
    -- the groups their spawns created through the battle that tears the field
    -- down, so leaving the field must not end them.
    p4.hook(seam(0x002319c0, "vi"), "vi", "before", function(context)
        if liveCount > 0 then
            local from = context.args[1] :: number
            for unit in liveUnits do
                if unit >= from and unit < from + ENCOUNTER_GROUP_BYTES then
                    setLive(unit, nil)
                end
            end
        end
    end),
}

-- The readers run for every unit in every battle; they are enabled only while a
-- live enemy unit carries a profile, so fights without one allocate no hook contexts.
readerHooks = {
    p4.hook(seam(0x00231f80, "Hp"), "Hp", "after", function(context)
        local live = liveUnits[context.args[1] :: number]
        if live then
            profiledReads += 1
            context.result = ratio(context.result :: number, live.hp, 0xffff)
        end
    end),
    p4.hook(seam(0x00232290, "Hp"), "Hp", "after", function(context)
        local live = liveUnits[context.args[1] :: number]
        if live then
            profiledReads += 1
            context.result = ratio(context.result :: number, live.sp, 0xffff)
        end
    end),
    p4.hook(seam(0x00232b40, "Bpi"), "Bpi", "after", function(context)
        local live = liveUnits[context.args[1] :: number]
        local pair = live and live.stats[band(context.args[2] :: number, 0xffff) + 1]
        if pair then
            profiledReads += 1
            context.result = ratio(context.result :: number, pair, 99)
        end
    end),
    p4.hook(seam(0x0023e3e0, "Hpi"), "Hpi", "after", function(context)
        local live = liveUnits[context.args[1] :: number]
        if live and band(context.args[2] :: number, 0xff) == 0 then
            profiledReads += 1
            context.result = ratio(context.result :: number, live.attack, 0xffff)
        end
    end),
    p4.hook(seam(0x00243950, "Hp"), "Hp", "after", function(context)
        local live = liveUnits[context.args[1] :: number]
        if live then
            profiledReads += 1
            context.result = ratio(context.result :: number, live.exp, 0xffff)
        end
    end),
    p4.hook(seam(0x002439c0, "Hp"), "Hp", "after", function(context)
        local live = liveUnits[context.args[1] :: number]
        if live then
            profiledReads += 1
            context.result = ratio(context.result :: number, live.money, 0xffff)
        end
    end),
}

-- One line per battle whose readers applied a profile (the in-battle witness).
table.insert(profileHooks, p4.events.on("battle_end", function()
    if profiledReads > 0 then
        log(string.format("RANDO_R6_BATTLE reads=%d", profiledReads))
        profiledReads = 0
    end
end))

local function setProfiles(records: buffer?)
    profiles = records
    floorTable, battleTable, liveUnits, liveCount, profiledReads = nil, nil, {}, 0, 0
    setReaders(false)
    for _, hook in profileHooks do
        if records then
            hook:enable()
        else
            hook:disable()
        end
    end
end
setProfiles(nil)

-- R10 live: Shuffle Time's Persona cards. btlShuffleSelectMember (0x0036ee60)
-- builds three lists from the Persona metadata (flags & 0xDB clear, level within
-- the player's window): A already in the roster, B above the player's level, C
-- the rest; it shuffles them with the game's RNG and fills up to four cards.
-- When it offers cards (result 1), each card is replaced by one drawn with the
-- run's own stream for this award from the same list the game's card came
-- from, without repeats: eligibility, the A/B/C mix, the card count and every
-- other card and roll stay the game's. The award ordinal counts offered
-- Shuffle Times since the run began and travels in the save block; the game's
-- RNG is never read or advanced by the mod.
local SHUFFLE_SELECT = 0x0036ee60
local LEVEL_OF = 0x00104c70 -- datCalcGetLevelById
local EARLY_WINDOW = 0x0064e76f -- the highest level offered while the player is below 10
local PERSONA_ROWS_SLOT = 0x007644c4 -- PERSONA0
local SHUFFLE_ROSTER, SHUFFLE_ROSTER_RECORDS, SHUFFLE_RECORD = 0x00797f8c, 12, 48
seam(LEVEL_OF, "ii")
local shuffleSeed: string? = nil
local shuffleOrdinal = {hi = 0, lo = 0}
-- Shuffle Time candidates (flags & 0xDB clear) as {id, level}, by level then id.
local shuffleCandidates: {{number}}? = nil

local function candidates(): {{number}}
    local known = shuffleCandidates
    if known then
        return known
    end
    local rows = p4.mem.read_bytes(p4.mem.read("u32", PERSONA_ROWS_SLOT), PERSONA_ROWS * META_STRIDE)
    local list: {{number}} = {}
    for id = 0, PERSONA_ROWS - 1 do
        if band(buffer.readu16(rows, id * META_STRIDE), 0xdb) == 0 then
            table.insert(list, {id, buffer.readu8(rows, id * META_STRIDE + 3)})
        end
    end
    table.sort(list, function(a: {number}, b: {number}): boolean
        return a[2] < b[2] or (a[2] == b[2] and a[1] < b[1])
    end)
    shuffleCandidates = list
    return list
end

local function shuffleLists(encounterLevel: number): {[number]: string}
    local player = band(p4.call(LEVEL_OF, "ii", 1) :: number, 0xff)
    local most, least
    if player > 0 and player < 10 then
        most, least = p4.mem.read("u8", EARLY_WINDOW + player), 1
    elseif player >= encounterLevel then
        most, least = encounterLevel - 5, encounterLevel - 10
    else
        most, least = player - 5, player - 10
    end
    local cap = math.min(player + 3, 99)
    least, most = math.max(least, 0), math.max(most, 0)
    local roster = p4.mem.read_bytes(SHUFFLE_ROSTER, SHUFFLE_ROSTER_RECORDS * SHUFFLE_RECORD)
    local owned: {[number]: boolean} = {}
    for slot = 0, SHUFFLE_ROSTER_RECORDS - 1 do
        if band(buffer.readu16(roster, slot * SHUFFLE_RECORD), 1) ~= 0 then
            owned[buffer.readu16(roster, slot * SHUFFLE_RECORD + 2)] = true
        end
    end
    local listOf: {[number]: string} = {}
    for _, candidate in candidates() do
        local id, level = candidate[1], candidate[2]
        if level > cap then
            break
        end
        if level > player or (level <= most and level >= least) then
            listOf[id] = if owned[id] then "A" elseif level > player then "B" else "C"
        end
    end
    return listOf
end

local shuffleHook = p4.hook(seam(SHUFFLE_SELECT, "iphi"), "iphi", "after", function(context)
    local seed = shuffleSeed
    if (context.result :: number) ~= 1 or seed == nil then
        return
    end
    local selection = context.args[1] :: number
    local count = math.clamp(p4.mem.read("i32", selection + 8), 0, 4)
    local listOf = shuffleLists(context.args[2] :: number)
    local label = "shuffle-time/" .. counterHex(shuffleOrdinal.hi, shuffleOrdinal.lo)
    local r = stream(seed, label)
    shuffleOrdinal.hi, shuffleOrdinal.lo = counterAdd(shuffleOrdinal.hi, shuffleOrdinal.lo, 1)
    local used: {[number]: boolean} = {}
    local cards: {number} = {}
    for slot = 0, count - 1 do
        local card = p4.mem.read("i16", selection + slot * 2)
        cards[slot + 1] = card
        used[card] = true
    end
    local before, after = {}, {}
    for slot = 0, count - 1 do
        local card = cards[slot + 1]
        local from = listOf[card]
        local pick = card
        if from then
            local choices: {number} = {}
            for _, candidate in candidates() do
                local id = candidate[1]
                if listOf[id] == from and (id == card or not used[id]) then
                    table.insert(choices, id)
                end
            end
            pick = choices[below(r, #choices) + 1]
            used[card], used[pick] = nil, true
        end
        p4.mem.write("i16", selection + slot * 2, pick)
        table.insert(before, string.format("%03d", card))
        table.insert(after, string.format("%03d%s", pick, from or "?"))
    end
    log(string.format("RANDO_R10 %s cards=%s>%s", label, table.concat(before, ","), table.concat(after, ",")))
end)

local function setShuffle(current: Run?, ordinal: {hi: number, lo: number}?)
    local active = current ~= nil and bit32.btest(current.result.definition.options, bit32.lshift(1, 10 - 1))
    shuffleSeed = if active and current then current.result.definition.seed else nil
    shuffleOrdinal = ordinal or {hi = 0, lo = 0}
    if active then
        shuffleHook:enable()
    else
        shuffleHook:disable()
    end
end
setShuffle(nil, nil)

-- R11 live: the starter joins the roster at the first field from 04/16 on
-- (date index 15; 04/01 is 0), after the story's Izanagi fights, once per run.
local GRANT = 0x0010b060 -- datPersonaTryAddRosterMember: the record from the run's tables
local ROSTER_SLOT_OF = 0x0010aa80 -- datPersonaFindRosterSlotById
local GRANT_FROM_DATE = 15
seam(GRANT, "pH")
seam(ROSTER_SLOT_OF, "ii")
local starterId: number? = nil
local starterGranted = false
local starterSubscription: P4Subscription
starterSubscription = p4.events.on("field_enter", function(event: P4FieldEnterEvent)
    local id = starterId
    if id == nil or starterGranted or type(event.date) ~= "number" or event.date < GRANT_FROM_DATE then
        return
    end
    starterGranted = true
    starterSubscription:disable()
    if (p4.call(ROSTER_SLOT_OF, "ii", id) :: number) ~= -1 then
        log(string.format("RANDO_R11 persona=%d already in the roster", id))
        return
    end
    local record = p4.call(GRANT, "pH", id) :: number
    log(string.format("RANDO_R11 %s persona=%d date=%d", if record ~= 0 then "granted" else "roster_full", id,
        event.date))
end)

local function setStarter(current: Run?, granted: boolean)
    local record = if current then current.result.layer[STARTER_KEY] else nil
    starterId = if record then buffer.readu16(record, 0) else nil
    starterGranted = granted
    if starterId ~= nil and not granted then
        starterSubscription:enable()
    else
        starterSubscription:disable()
    end
end
setStarter(nil, false)

-- Run records that are not banks (R6 profiles, the R11 starter): saved and
-- hashed with the run, never overlaid. Their sizes: a whole number of records.
local RECORD_SIZES: {[string]: number} = {[PROFILE_KEY] = PROFILE_STRIDE, [STARTER_KEY] = 2}

-- The layer the table service overlays: every bank, without the run records.
local function overlayLayer(layer: {[string]: buffer}): {[string]: buffer}
    local banks = table.clone(layer)
    for key in RECORD_SIZES do
        banks[key] = nil
    end
    return banks
end

local function bankCount(layer: {[string]: buffer}): number
    local n = 0
    for key in layer do
        if RECORD_SIZES[key] == nil then
            n += 1
        end
    end
    return n
end

-- Publishes a run's banks as this mod's overlay (and its R6 records), or clears it.
local function publish(next: Run?, why: string): boolean
    local ok, reason = p4.tables.overlay(if next then overlayLayer(next.result.layer) else {})
    if not ok then
        log("RANDO_OVERLAY_REFUSED " .. why .. " " .. tostring(reason))
        setStatus("RANDOMIZER NOT APPLIED")
        return false
    end
    setProfiles(if next then next.result.layer[PROFILE_KEY] else nil)
    setShuffle(next, if next then next.shuffle else nil)
    setStarter(next, if next then next.starterGranted == true else false)
    run = next
    if next then
        local result = next.result
        log(string.format("RANDO_RUN %s code=%s digest=%s features=%s output=%s banks=%d", why,
            runCode(result.definition), hex(result.digest), optionsText(result.definition.options),
            hex(result.layerHash), bankCount(result.layer)))
    else
        log("RANDO_RUN " .. why .. " off")
    end
    setStatus(nil)
    return true
end

-- New Game+ carries the compendium over; a randomized New Game+ is refused
-- visibly rather than mixing carried records with a fresh seed. A run that is
-- generated only after a Persona was granted is refused the same way.
local COMPENDIUM = 0x007981cc
local ROSTER = 0x00797f8c
local RECORD = 48
-- The persona record cache (the compendium) holds an active record for every
-- Persona the game has seen; a fresh game can hold the protagonist's starter
-- and the party's Personas, never an ordinary one fused or drawn earlier.
local function carriedOver(): number?
    local bytes = p4.mem.read_bytes(COMPENDIUM, 256 * RECORD)
    for id = 2, 0xbf do
        if band(buffer.readu16(bytes, id * RECORD), 1) ~= 0 then
            return id
        end
    end
    return nil
end
-- A fresh game already stocks the protagonist's starter (protected, so its
-- rows never change); any other ordinary Persona means the run came too late.
local function personaGranted(): number?
    for i = 0, 11 do
        local id = p4.mem.read("u16", ROSTER + i * RECORD + 2)
        if id >= 2 and id < 0xc0 then
            return id
        end
    end
    return nil
end

-- Drives the armed New Game: generation (if the speculative one does not
-- match the armed choice), then the overlay once its banks are validated.
local function advanceNewGame()
    local choice = newGame
    if choice == nil then
        return
    end
    local def = definitionOf(choice)
    if def == nil then
        return -- the input fingerprint is still being hashed
    end
    startTask(def)
    local current = task :: Task
    if current.failure then
        newGame = nil
        log("RANDO_NEW_GAME_FAILED " .. tostring(current.failure))
        setStatus("RANDOMIZER FAILED")
        return
    end
    local result = current.result
    if result == nil then
        return
    end
    newGame = nil
    local granted = personaGranted()
    if granted then
        log(string.format("RANDO_NEW_GAME_LATE Persona %03d was granted before the run was ready; "
            .. "this game stays unrandomized", granted))
        setStatus("RANDOMIZER NOT APPLIED")
        return
    end
    task = nil -- the run keeps the result; the finished generation goes
    if publish({result = result, adopted = "new_game"}, "new_game") then
        writeArtifacts(result)
    end
end

-- The first field or battle after a reset with no load in between is a New
-- Game: the next-run settings are snapshotted here and the run locks.
local function armNewGame()
    if not pendingNewGame then
        return
    end
    setPending(false)
    local options = settingsOptions()
    if options == 0 then
        log("RANDO_NEW_GAME vanilla")
        return
    end
    local carried = carriedOver()
    if carried then
        log(string.format("RANDO_NEW_GAME_PLUS_REFUSED a New Game+ carries the compendium over (Persona %03d); "
            .. "this game stays unrandomized", carried))
        setStatus("RANDOMIZER: NO NG+")
        return
    end
    newGame = {seed = seed, options = options}
    fingerprintWanted = true
    log(string.format("RANDO_NEW_GAME armed seed=%s features=%s", hex(seed), optionsText(options)))
    advanceNewGame()
    wantFrames()
end

local function stepTask()
    local current = task
    if current == nil or current.result ~= nil or current.failure ~= nil then
        return
    end
    local ok, value = coroutine.resume(current.thread)
    if not ok then
        current.failure = tostring(value)
        log("RANDO_GENERATE_FAILED " .. tostring(value))
    elseif coroutine.status(current.thread) == "dead" then
        local result = value :: Result
        current.result = result
        log(string.format("RANDO_GENERATE done code=%s output=%s", runCode(current.definition),
            hex(result.layerHash)))
    end
end

-- A loaded run is re-verified in the background: its digest and output hash,
-- this disc's data against the run's input fingerprint, and a fresh
-- generation from its definition against the banks the save carried.
local function startCheck(current: Run, export: boolean)
    local def = current.result.definition
    check = {run = current, export = export, thread = coroutine.create(function(): string
        if runDigest(def) ~= current.result.digest then
            return "the saved run digest does not match its definition"
        end
        if bankHash(current.result.layer) ~= current.result.layerHash then
            return "the saved banks do not match their output hash"
        end
        if stockPrint(def.options) ~= def.fingerprint then
            return "this game's battle tables differ from the ones the run was generated from"
        end
        local fresh = generate(def, stock :: Stock)
        if fresh.layerHash ~= current.result.layerHash then
            return "regenerating the run from its seed produced different banks"
        end
        current.result.spoiler, current.result.manifest = fresh.spoiler, fresh.manifest
        current.result.counts = fresh.counts
        return "ok"
    end)}
    fingerprintWanted = true
    wantFrames()
end

local function stepCheck()
    local current = check :: Check
    local ok, value = coroutine.resume(current.thread)
    if ok and coroutine.status(current.thread) ~= "dead" then
        return
    end
    check = nil
    local verdict = if ok then tostring(value) else "error: " .. tostring(value)
    if verdict == "ok" then
        log("RANDO_VERIFY ok code=" .. runCode(current.run.result.definition))
        if current.export then
            writeArtifacts(current.run.result)
        else
            current.run.result.spoiler, current.run.result.manifest = "", ""
        end
    else
        log("RANDO_VERIFY_FAILED " .. verdict)
        if run == current.run then
            setStatus("RANDOMIZER RUN MISMATCH")
        end
    end
end

-- This disc's stock input banks, read once in the background; the input
-- fingerprint for the current settings is hashed in the following frame.
local function stepFingerprint(): boolean
    if stock ~= nil then
        if inputHashed then
            return false
        end
        inputHashed = true
        local options = settingsOptions()
        log("RANDO_INPUT fingerprint=" .. hex(stockPrint(options) :: string))
        -- Speculative generation while the title screen is up.
        local def = definitionOf({seed = seed, options = options})
        if pendingNewGame and options ~= 0 and def then
            startTask(def)
        end
        return true
    end
    local banks: Stock = {}
    for _, key in INPUT_BANKS do
        if key == "GATES" then
            banks[key] = p4.mem.read_bytes(GATES_ADDRESS, GATES_BYTES)
        elseif key == "ITEMS" then
            -- The retail item table's stock through the table service (never an
            -- ITEMTBL overlay's bytes); the live table when no retail bank exists.
            local ok, items = pcall(p4.tables.stock, "ITEMTBL")
            banks[key] = if ok then items else p4.mem.read_bytes(p4.mem.read("u32", ITEMS_POINTER), ITEM_ROWS * ITEM_STRIDE)
        else
            banks[key] = p4.tables.stock(key)
        end
    end
    stock = banks
    return true
end

frameSubscription = p4.events.on("frame_start", function()
    if pendingSeed then
        local edited = pendingSeed :: string
        pendingSeed = nil
        local ok, why = p4.settings.set("seed", hex(edited))
        if ok then
            seed = edited
            log("RANDO_SEED " .. hex(edited))
        else
            log("RANDO_SEED_FAILED " .. tostring(why))
        end
    end
    if exportRequested then
        exportRequested = false
        local current = run
        local verifying = check
        if current == nil then
            log("RANDO_SPOILER_UNAVAILABLE no run is active")
        elseif current.result.spoiler ~= "" then
            writeArtifacts(current.result)
        elseif verifying and verifying.run == current then
            verifying.export = true -- written once the running check regenerated it
        else
            log("RANDO_SPOILER_REGENERATING code=" .. runCode(current.result.definition))
            startCheck(current, true)
        end
    end
    -- Work resumes only with a full pacing margin left: a frame whose budget
    -- other callbacks already spent (frames without a flip share one) waits.
    if p4.budget() < PACE_MARGIN then
        wantFrames()
        return
    end
    if fingerprintWanted and stepFingerprint() then
        -- the stock banks are read in a frame of their own
    elseif check then
        stepCheck()
    elseif task and task.result == nil and task.failure == nil then
        stepTask()
    end
    -- Publishing (overlay, hooks, spoiler files) waits for a frame with margin.
    if p4.budget() >= PACE_MARGIN then
        advanceNewGame()
    end
    wantFrames()
end)

local arming: {P4Subscription} = {
    p4.events.on("field_enter", function()
        armNewGame()
    end),
    p4.events.on("battle_start", function()
        armNewGame()
    end),
}
setPending = function(pending: boolean)
    pendingNewGame = pending
    for _, subscription in arming do
        if pending then
            subscription:enable()
        else
            subscription:disable()
        end
    end
end

-- Save block: the run definition and its generated banks.
local function encodeRun(current: Run): P4SaveValue
    local result = current.result
    local def = result.definition
    local banks: {[string]: string} = {}
    for key, bank in result.layer do
        banks[key] = buffer.tostring(bank)
    end
    return {
        algorithm = def.algorithm,
        catalog = def.catalog,
        seed = hex(def.seed),
        options = def.options,
        fingerprint = hex(def.fingerprint),
        digest = hex(result.digest),
        output = hex(result.layerHash),
        adopted = current.adopted,
        banks = banks,
        -- The R10 award ordinal, while this run's Shuffle Time cards are live.
        shuffle = if current == run and shuffleSeed then counterHex(shuffleOrdinal.hi, shuffleOrdinal.lo) else nil,
        -- Whether the R11 starter already joined the roster (never granted twice).
        starter = if current == run and starterId ~= nil then (if starterGranted then "granted" else "pending") else nil,
    }
end

-- Decodes a saved run within the snapshot budget (no hashing): versions,
-- shapes and that every bank still has its loaded size.
local function decodeRun(value: P4SaveValue): (Run?, string?)
    if type(value) ~= "table" then
        return nil, "the randomizer block is not a table"
    end
    local t = value :: {[any]: any}
    if t.algorithm ~= ALGORITHM then
        return nil, string.format("this save was randomized by algorithm %s; this mod generates %d",
            tostring(t.algorithm), ALGORITHM)
    end
    if t.catalog ~= CATALOG then
        return nil, string.format("this save uses catalog %s; this mod has %d", tostring(t.catalog), CATALOG)
    end
    local function bytesField(name: string, size: number): string?
        local text = t[name]
        if type(text) ~= "string" then
            return nil
        end
        local bytes = unhex(text)
        if bytes == nil or #bytes ~= size then
            return nil
        end
        return bytes
    end
    local seedBytes = bytesField("seed", 16)
    local print = bytesField("fingerprint", 32)
    local digest = bytesField("digest", 32)
    local output = bytesField("output", 32)
    if seedBytes == nil or print == nil or digest == nil or output == nil or type(t.options) ~= "number"
        or type(t.banks) ~= "table" then
        return nil, "the randomizer block is malformed"
    end
    local layer: {[string]: buffer} = {}
    for key, bytes in t.banks do
        if type(key) ~= "string" or type(bytes) ~= "string" then
            return nil, "the randomizer block has a malformed bank"
        end
        local size = RECORD_SIZES[key]
        if size then
            if #bytes == 0 or #bytes % size ~= 0 or (key == STARTER_KEY and #bytes ~= size) then
                return nil, "the randomizer block's " .. key .. " records are malformed"
            end
        else
            local info = p4.tables.bank(key)
            if not info.overlayable or info.bytes ~= #bytes then
                return nil, "this save's " .. key .. " no longer matches the game's battle tables"
            end
        end
        layer[key] = buffer.fromstring(bytes)
    end
    local def: Definition = {algorithm = ALGORITHM, catalog = CATALOG, seed = seedBytes, options = t.options,
                             fingerprint = print}
    local result: Result = {definition = def, digest = digest, layer = layer, layerHash = output, spoiler = "",
                            manifest = "", counts = {}}
    local shuffle: {hi: number, lo: number}? = nil
    if t.shuffle ~= nil then
        local words = if type(t.shuffle) == "string" and #t.shuffle == 16 then unhex(t.shuffle) else nil
        if words == nil then
            return nil, "the randomizer block's Shuffle Time ordinal is malformed"
        end
        local bytes = words :: string
        shuffle = {hi = string.unpack(">I4", bytes, 1) :: number, lo = string.unpack(">I4", bytes, 5) :: number}
    end
    if t.starter ~= nil and t.starter ~= "granted" and t.starter ~= "pending" then
        return nil, "the randomizer block's starter state is malformed"
    end
    return {result = result, adopted = if type(t.adopted) == "string" then t.adopted else "load", shuffle = shuffle,
            starterGranted = t.starter == "granted"}, nil
end

p4.save.register({
    schema = SAVE_SCHEMA,
    max_bytes = 512 * 1024,
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
            return false, "Randomizer: " .. tostring(why)
        end
        return true
    end,
    published = function(value: P4SaveValue)
        local current = run
        if current then
            log(string.format("RANDO_SAVED code=%s output=%s", runCode(current.result.definition),
                hex(current.result.layerHash)))
        end
    end,
    load = function(value: P4SaveValue, origin: "load" | "reset")
        -- The native commit already restored every bank: the previous run ended.
        run = nil
        setProfiles(nil)
        setShuffle(nil, nil)
        setStarter(nil, false)
        newGame = nil
        check = nil
        setStatus(nil)
        if origin == "reset" then
            setPending(settingsOptions() ~= 0)
            log("RANDO_RESET pending=" .. tostring(pendingNewGame))
            if pendingNewGame then
                -- Speculative generation while the title screen is up.
                fingerprintWanted = true
                local def = definitionOf({seed = seed, options = settingsOptions()})
                if def then
                    startTask(def)
                end
            end
            wantFrames()
            return
        end
        setPending(false)
        if value == nil then
            log("RANDO_RUN load vanilla")
            wantFrames()
            return
        end
        local decoded, why = decodeRun(value)
        if decoded == nil then
            error("randomizer save block rejected after authorization: " .. tostring(why))
        end
        if publish(decoded, "load") then
            startCheck(decoded, false)
        end
    end,
})

-------------------------------------------------------------------------------
-- MODS hub page: the locked run, the next run's seed and the spoiler export
-------------------------------------------------------------------------------

local digitCursor = 0 -- 0..31: the seed digit the WORD rows edit
local page: P4UIPage? = nil

local function editedSeed(): string
    return pendingSeed or seed
end

local function setDigit(index: number, delta: number)
    local digits = hex(editedSeed())
    local value = ((tonumber(string.sub(digits, index + 1, index + 1), 16) :: number) + delta) % 16
    pendingSeed = unhex(string.sub(digits, 1, index) .. string.format("%x", value) .. string.sub(digits, index + 2))
end

local ROWS: {string} = {"run", "code", "next", "word1", "word2", "word3", "word4", "cursor", "zero", "export",
                        "folder"}

-- R5 without R6 moves only groups that already fit a slot's budget, so few move:
-- a note row follows NEXT NEW GAME while this run or the next one is set that way.
local function limitedVariety(options: number): boolean
    return bit32.btest(options, lsh(1, 5 - 1)) and not bit32.btest(options, lsh(1, 6 - 1))
end

local function pageRows(): {string}
    local current = run
    if not (limitedVariety(settingsOptions()) or (current ~= nil and limitedVariety(current.result.definition.options))) then
        return ROWS
    end
    local rows = table.clone(ROWS)
    table.insert(rows, (table.find(rows, "next") :: number) + 1, "variety")
    return rows
end

local function pageRow(index: number): P4UIRow
    local key = pageRows()[index]
    local current = run
    if key == "run" then
        return {key = key, label = "THIS RUN (LOCKED)", kind = "info",
                value = if current then string.upper(optionsText(current.result.definition.options)) else "NOT RANDOMIZED",
                help = "The loaded game's randomizer run. It never changes inside a game; the settings apply from the next New Game."}
    elseif key == "code" then
        local code = if current then runCode(current.result.definition) else "-"
        return {key = key, label = "RUN CODE", kind = "info", value = string.upper(code),
                help = "Version, seed, features and input fingerprint of this run. The same code generates the same game on every platform."}
    elseif key == "next" then
        local options = settingsOptions()
        return {key = key, label = "NEXT NEW GAME", kind = "info",
                value = if options ~= 0 then string.upper(optionsText(options)) else "NOT RANDOMIZED",
                help = "Turn on RANDOMIZE NEW GAMES and at least one feature in this mod's settings to randomize the next New Game."}
    elseif key == "variety" then
        return {key = key, label = "NOTE", kind = "info", value = "ENEMY VARIETY IS LIMITED WITHOUT R6",
                help = "R5 without R6 moves a Shadow group only where it already fits the original group's HP, SP, attack and stats, so few groups move. Turn on R6 ENEMY STATS for the full variety."}
    elseif string.sub(key, 1, 4) == "word" then
        local word = tonumber(string.sub(key, 5)) :: number
        local text = string.upper(string.sub(hex(editedSeed()), (word - 1) * 8 + 1, word * 8))
        if digitCursor // 8 + 1 == word then
            local at = digitCursor % 8
            text = string.sub(text, 1, at) .. "[" .. string.sub(text, at + 1, at + 1) .. "]" .. string.sub(text, at + 2)
        end
        return {key = key, label = "SEED WORD " .. tostring(word), kind = "value", value = text,
                help = "Left/Right changes the marked digit and confirm moves the mark. The seed is 32 hexadecimal digits; all zero is a valid seed. It applies to the next New Game."}
    elseif key == "cursor" then
        return {key = key, label = "MARKED DIGIT", kind = "value", value = tostring(digitCursor + 1) .. " OF 32",
                help = "Left/Right moves the marked seed digit."}
    elseif key == "zero" then
        return {key = key, label = "RESET SEED TO ZERO", kind = "action", help = "Sets the next run's seed to all zeros."}
    elseif key == "export" then
        return {key = key, label = "EXPORT SPOILER LOG", kind = "action",
                help = "Writes this run's spoiler.txt and manifest.json to the folder below. Nothing opens automatically."}
    end
    local folder = p4.artifacts.folder()
    return {key = key, label = "SPOILER FOLDER", kind = "info",
            value = if folder then string.upper(string.sub(folder, -60)) else "UNAVAILABLE",
            help = "Spoiler logs are written to mod-artifacts/p4pc.randomizer/runs/<run digest>/ beside the settings file."}
end

page = p4.ui.page({
    key = "run",
    title = "SEEDED RANDOMIZER",
    count = function(): number
        return #pageRows()
    end,
    row = pageRow,
    find = function(key: string): number?
        return table.find(pageRows(), key)
    end,
    adjust = function(key: string, direction: number)
        -- Synchronous UI callback: no disk writes here; the next frame persists.
        if string.sub(key, 1, 4) == "word" then
            local word = tonumber(string.sub(key, 5)) :: number
            if digitCursor // 8 + 1 ~= word then
                digitCursor = (word - 1) * 8
            end
            if direction == 0 then
                digitCursor = (digitCursor + 1) % 32
            else
                setDigit(digitCursor, direction)
            end
        elseif key == "cursor" then
            digitCursor = (digitCursor + (if direction == 0 then 1 else direction)) % 32
        elseif key == "zero" and direction == 0 then
            pendingSeed = ZERO_SEED
        elseif key == "export" and direction == 0 then
            exportRequested = true
        end
        wantFrames()
        local current = page
        if current then
            current:refresh()
        end
    end,
})

setPending(false)
wantFrames()
log(string.format("RANDO_READY algorithm=%d catalog=%d", ALGORITHM, CATALOG))

return Engine
