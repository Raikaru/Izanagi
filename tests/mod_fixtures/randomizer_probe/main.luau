--!strict
-- Witness-only probe for tests/mod_randomizer_game_test.py, loaded after
-- p4pc.randomizer in the separately admitted MOD_WITNESS lane. It reads what
-- the game's own table consumers produce: datPersonaInitRecord builds a Persona
-- record from PERSONA0/PERSONA1 (level, stats, starting skills) and
-- datPersonaRunSkillSlot decodes a record's affinity from UNIT2, plus the raw
-- live rows behind each bank's slot, and digests of the live encounter, treasure
-- and enemy banks the field and battle code read (Encount_PickEncounter,
-- Encount_FindRecord, datCalcPickWeightedSkill). It changes no game state:
-- records are built in its own scratch buffer, and a base-level record never
-- levels up.
local INIT_RECORD = 0x0010cad0
local AFFINITY = 0x001097c0
local FUSE_PAIR = 0x003124a0
local FUSE_TRIPLE = 0x003127e0
-- Ordinary pairs of different arcana for the game's own pair evaluator.
local FUSIONS: {{number}} = {{46, 26}, {14, 7}, {172, 61}, {33, 44}, {6, 54}, {1, 14}, {67, 146}, {110, 152}}
-- Ordinary triples of three arcanas for its triangle evaluator, equal-level ties
-- included (fclFuseTripleArcana orders the three records by level).
local TRIPLES: {{number}} = {{27, 59, 128}, {27, 102, 156}, {29, 44, 114}, {29, 54, 167}, {29, 67, 172}, {29, 74, 148}, {29, 110, 152}, {29, 114, 128}}
local ROSTER = 0x00797f8c
local LIVE_BANKS: {{key: string, slot: number}} = {
    {key = "ENCOUNT3", slot = 0x00764508}, {key = "ENCOUNT4", slot = 0x00764514}, {key = "UNIT0", slot = 0x007644b4},
}
local PROBES: {number} = {1, 7, 14, 26, 46, 172}
type Bank = {key: string, slot: number, stride: number}
local BANKS: {Bank} = {
    {key = "PERSONA0", slot = 0x007644c4, stride = 14},
    {key = "PERSONA1", slot = 0x007644cc, stride = 70},
    {key = "UNIT2", slot = 0x007644c8, stride = 32},
}

for _, seam in {{INIT_RECORD, "vpH"}, {AFFINITY, "ipi"}, {FUSE_PAIR, "ipHH"}, {FUSE_TRIPLE, "ipppp"}} do
    local info = p4.describe(seam[1] :: number)
    assert(info and info.signature == seam[2], string.format("probe needs 0x%08x %s", seam[1] :: number, seam[2] :: string))
end

local scratch = p4.mem.alloc(64)
-- Three ingredient records (0x30 bytes each, base level) for fclFuseTriple.
local ingredients = p4.mem.alloc(3 * 0x30)

local function hexOf(bytes: buffer): string
    local parts = table.create(buffer.len(bytes))
    for i = 0, buffer.len(bytes) - 1 do
        parts[i + 1] = string.format("%02x", buffer.readu8(bytes, i))
    end
    return table.concat(parts)
end

local function probeOne(tag: string, id: number)
    p4.call(INIT_RECORD, "vpH", scratch.address, id)
    local skills, stats, affinity = {}, {}, {}
    for i = 0, 7 do
        skills[i + 1] = string.format("%03x", p4.mem.read("u16", scratch.address + 0x0c + i * 2))
        affinity[i + 1] = string.format("%x", p4.call(AFFINITY, "ipi", scratch.address, i) :: number)
    end
    for i = 0, 4 do
        stats[i + 1] = tostring(p4.mem.read("u8", scratch.address + 0x1c + i))
    end
    p4.log(string.format("PROBE %s persona=%d id=%d level=%d skills=%s stats=%s affinity=%s", tag, id,
        p4.mem.read("u16", scratch.address + 2), p4.mem.read("u8", scratch.address + 4),
        table.concat(skills, ","), table.concat(stats, ","), table.concat(affinity, ",")))
    for _, bank in BANKS do
        local base = p4.mem.read("u32", bank.slot)
        p4.log(string.format("PROBE_ROW %s %s %d %s", tag, bank.key, id,
            hexOf(p4.mem.read_bytes(base + id * bank.stride, bank.stride))))
    end
end

-- Probe once per session state, one Persona per frame: "stock" from the first
-- field without an overlay, "applied" once the randomizer's overlay is live.
type Pending = {tag: string, next: number}
local pending: Pending? = nil
local probed: {[string]: boolean} = {}
local function owner(): string?
    return p4.tables.bank("PERSONA1").owner
end
local function fuseAll(tag: string)
    for _, pair in FUSIONS do
        local status = p4.call(FUSE_PAIR, "ipHH", scratch.address, pair[1], pair[2]) :: number
        p4.log(string.format("PROBE_FUSE %s %d %d ret=%d result=%d", tag, pair[1], pair[2], status,
            p4.mem.read("u16", scratch.address + 2)))
    end
    for _, triple in TRIPLES do
        for i = 1, 3 do
            p4.call(INIT_RECORD, "vpH", ingredients.address + (i - 1) * 0x30, triple[i])
        end
        local status = p4.call(FUSE_TRIPLE, "ipppp", scratch.address, ingredients.address,
            ingredients.address + 0x30, ingredients.address + 0x60) :: number
        p4.log(string.format("PROBE_FUSE3 %s %d %d %d ret=%d result=%d", tag, triple[1], triple[2], triple[3], status,
            p4.mem.read("u16", scratch.address + 2)))
    end
end
local function liveBanks(tag: string)
    for _, live in LIVE_BANKS do
        local size = buffer.len(p4.tables.stock(live.key))
        local bytes = p4.mem.read_bytes(p4.mem.read("u32", live.slot), size)
        p4.log(string.format("PROBE_BANK %s %s %s", tag, live.key, hexOf(buffer.fromstring(p4.sha256(bytes)))))
    end
end
p4.events.on("frame_start", function()
    local current = pending
    if current then
        if current.next == #PROBES + 1 then
            fuseAll(current.tag)
            current.next += 1
            return
        elseif current.next > #PROBES then
            liveBanks(current.tag)
            p4.log("PROBE_DONE " .. current.tag)
            pending = nil
            return
        end
        probeOne(current.tag, PROBES[current.next])
        current.next += 1
    elseif owner() == "p4pc.randomizer" and not probed.applied then
        probed.applied = true
        pending = {tag = "applied", next = 1}
    end
end)
p4.events.on("field_enter", function()
    if owner() == nil and not probed.stock and pending == nil then
        probed.stock = true
        pending = {tag = "stock", next = 1}
    end
end)
-- The protagonist's own Persona record (roster slot 0) at each battle start and
-- each field entry: the story's tutorial battle may end without battle_end.
local function roster(where: string)
    local skills = {}
    for i = 0, 7 do
        skills[i + 1] = string.format("%03x", p4.mem.read("u16", ROSTER + 0x0c + i * 2))
    end
    -- Every active roster record's Persona (flags bit 0), in slot order.
    local members = {}
    for slot = 0, 11 do
        if bit32.band(p4.mem.read("u16", ROSTER + slot * 48), 1) ~= 0 then
            table.insert(members, tostring(p4.mem.read("u16", ROSTER + slot * 48 + 2)))
        end
    end
    -- The materials carried (datItemCount), for the Daidara sale.
    local held = {}
    if where == "field" then
        for material = 0x501, 0x5ff do
            local count = p4.call(0x00106600, "ih", material) :: number
            if count ~= 0 then
                table.insert(held, string.format("%x:%d", material, count))
            end
        end
    end
    p4.log(string.format("PROBE_ROSTER %s id=%d level=%d skills=%s members=%s held=%s", where,
        p4.mem.read("u16", ROSTER + 2), p4.mem.read("u8", ROSTER + 4), table.concat(skills, ","),
        table.concat(members, ","), table.concat(held, ",")))
end
p4.events.on("battle_start", function()
    roster("battle")
end)
-- Which formation each floor encounter picked from which table (the ENCOUNT1
-- entry's table), and which formation each battle created its enemies from.
local PICK, GROUP = 0x00161630, 0x00231630
for _, seam in {{PICK, "iiiii"}, {GROUP, "pH"}} do
    local info = p4.describe(seam[1] :: number)
    assert(info and info.signature == seam[2], string.format("probe needs 0x%08x %s", seam[1] :: number, seam[2] :: string))
end
p4.hook(PICK, "iiiii", "after", function(context)
    local result = context.result :: number
    if result ~= -1 then
        local entries = p4.mem.read("u32", 0x0076450c)
        p4.log(string.format("PROBE_PICK table=%d formation=%d kind=%d", p4.mem.read("u16",
            entries + bit32.band(context.args[3] :: number, 0xffff) * 10), bit32.band(result, 0xffff),
            bit32.rshift(result, 16)))
    end
end)
p4.hook(GROUP, "pH", "before", function(context)
    p4.log(string.format("PROBE_GROUP formation=%d", context.args[1] :: number))
end)
-- Every shop buy list the game builds (yListRebuild kind 2): the items it offers.
local REBUILD, LIST_COUNT, LIST_VALUE, SOLD = 0x002e2a10, 0x002e2670, 0x002e2740, 0x00110830
for _, seam in {{REBUILD, "viibb"}, {LIST_COUNT, "h"}, {LIST_VALUE, "ii"}, {SOLD, "Bi"}, {0x00106600, "ih"}} do
    local info = p4.describe(seam[1] :: number)
    assert(info and info.signature == seam[2], string.format("probe needs 0x%08x %s", seam[1] :: number, seam[2] :: string))
end
p4.hook(REBUILD, "viibb", "after", function(context)
    if (context.args[3] :: number) ~= 2 then
        return
    end
    local items = {}
    for index = 0, (p4.call(LIST_COUNT, "h") :: number) - 1 do
        table.insert(items, string.format("%x", p4.call(LIST_VALUE, "ii", index) :: number))
    end
    -- The materials sold so far (the item state counts Daidara's unlock gates compare).
    local sold = {}
    for material = 0x500, 0x5ff do
        local count = p4.call(SOLD, "Bi", material) :: number
        if count ~= 0 then
            table.insert(sold, string.format("%x:%d", material, count))
        end
    end
    p4.log(string.format("PROBE_SHOP filters=%08x/%08x sold=%s items=%s", context.args[1] :: number,
        context.args[2] :: number, table.concat(sold, ","), table.concat(items, ",")))
end)
p4.events.on("field_enter", function()
    roster("field")
end)
-- Every defeated enemy's drop roll (btlTargetRecordDefeatReward's datCalcPickWeightedSkill):
-- the enemy's native ID and the item its drop table gave (0: no drop this time).
local PICK_DROP = 0x00243a30
assert((p4.describe(PICK_DROP) :: any).signature == "Hpp", "probe needs 0x00243a30 Hpp")
p4.hook(PICK_DROP, "Hpp", "after", function(context)
    p4.log(string.format("PROBE_DROP enemy=%d item=%x", p4.mem.read("u16", (context.args[1] :: number) + 2),
        context.result :: number))
end)
-- Witness-only reconstruction for the Daidara leg of tests/mod_randomizer_dungeon_test.py:
-- its "grant" setting ("511:1,..."; absent everywhere else) adds those materials once,
-- through the game's own inventory setter (datItemSetCount), when the shopping
-- district loads; the sale, the unlock check and the buy list stay the game's.
local ITEM_COUNT, ITEM_SET_COUNT = 0x00106600, 0x00106620
for _, seam in {{ITEM_COUNT, "ih"}, {ITEM_SET_COUNT, "vhi"}} do
    local info = p4.describe(seam[1] :: number)
    assert(info and info.signature == seam[2], string.format("probe needs 0x%08x %s", seam[1] :: number, seam[2] :: string))
end
local granted = false
p4.events.on("field_enter", function(event: P4FieldEnterEvent)
    local grant = p4.settings.get("grant")
    if granted or type(grant) ~= "string" or event.field ~= 8 then
        return
    end
    granted = true
    for part in string.gmatch(grant, "[^,]+") do
        local id, count = string.match(part, "^(%x+):(%d+)$")
        if id and count then
            local item = tonumber(id, 16) :: number
            local held = p4.call(ITEM_COUNT, "ih", item) :: number
            p4.call(ITEM_SET_COUNT, "vhi", item, held + (tonumber(count) :: number))
            p4.log(string.format("PROBE_GRANT %x:%s witness-only, now %d", item, count,
                p4.call(ITEM_COUNT, "ih", item) :: number))
        end
    end
end)
p4.save.register({
    schema = 1,
    max_bytes = 64,
    snapshot = function(): P4SaveValue
        return nil
    end,
    load = function(value: P4SaveValue, origin: "load" | "reset")
        probed = {}
        pending = nil
    end,
})
p4.log("PROBE_READY")
