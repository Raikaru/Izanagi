-- Synthetic p4 provider for the headless randomizer qualification
-- (tests/mod_randomizer_engine_test.py). It is the first chunk run by
-- luau_script_runner: it defines the global p4 that mods/p4pc.randomizer/
-- main.luau runs against, plus FIXTURE for the test chunk. No game, no retail
-- bytes: banks are either generated here or read from files the driver
-- extracted from the user's own disc.
local args = {...}

local F = {
    args = args,
    settings = {} :: {[string]: any},
    logs = {} :: {string},
    events = {} :: {[string]: {any}},
    save = nil :: any,
    page = nil :: any,
    status = nil :: string?,
    artifacts = {} :: {[string]: string},
    overlay = {} :: {[string]: string},
    overlays = 0,
    banks = {} :: {[string]: buffer},
    memory = {} :: {[number]: number},
    -- Guest byte regions (base address -> bytes) behind the scalar memory map.
    regions = {} :: {[number]: buffer},
    -- The typed seams p4.describe knows (address -> signature).
    seams = {[0x00161630] = "iiiii", [0x00231630] = "pH", [0x00231af0] = "IpBH", [0x00231f80] = "Hp",
             [0x00232290] = "Hp", [0x00232b40] = "Bpi", [0x0023e3e0] = "Hpi", [0x00243950] = "Hp",
             [0x002439c0] = "Hp", [0x0036ee60] = "iphi", [0x00104c70] = "ii", [0x0010aa80] = "ii",
             [0x0010b060] = "pH", [0x002319c0] = "vi"} :: {[number]: string},
    -- p4.call results by address (a function of the arguments).
    calls = {} :: {[number]: (...number) -> number},
    budgetMode = "large",
    paceCalls = 0,
    yieldEvery = 0,
    resumeAt = 0,
    maxUsed = 0,
    maxGap = 0,
    lastPace = 0,
    maxCallback = 0,
    -- The units one callback starts with in "game" mode (the game gives 4096).
    callbackFuel = 4096,
    maxHeap = 0,
    maxCallbackAt = "",
    verbose = false,
}
FIXTURE = F
-- The seed main.luau reads at start-up (32 hexadecimal digits).
F.settings.seed = "00112233445566778899aabbccddeeff"

-- MINSTD keeps every product below 2^53.
local function lcg(state: number): number
    return (state * 48271) % 2147483647
end

-- Coherent synthetic battle banks with the retail strides and row counts.
function F.syntheticBanks(): {[string]: buffer}
    local x = 20261103
    local function rand(n: number): number
        x = lcg(x)
        return x % n
    end
    local skill0 = buffer.create(576 * 2)
    local skill1 = buffer.create(440 * 40)
    local msg1 = buffer.create(576 * 19)
    local roleOf = {}
    for skill = 0, 575 do
        local element, effect = -1, 0
        if skill == 0 then
            element = 0
        elseif skill < 80 then
            element, effect = skill % 6, 1
        elseif skill < 100 then
            element = 6 + skill % 2
        elseif skill < 140 then
            element = 8 + skill % 8
        elseif skill < 170 then
            element, effect = 16, if skill % 2 == 0 then 2 else 0
        elseif skill < 220 then
            element = 17
        elseif skill < 240 then
            element = 18
        elseif skill < 440 then
            element, effect = skill % 6, 1
        end
        buffer.writei8(skill0, skill * 2, element)
        if skill < 440 then
            buffer.writeu8(skill1, skill * 40 + 0x11, effect)
        end
        buffer.writestring(msg1, skill * 19, string.format("SKILL %03X", skill))
        roleOf[skill] = element
    end
    local persona0 = buffer.create(256 * 14)
    local persona1 = buffer.create(256 * 70)
    local unit2 = buffer.create(256 * 32)
    local msg3 = buffer.create(256 * 17)
    local damage = {0x14, 0x14, 0x14, 0x800, 0x100, 0x1000, 0x400, 0x200}
    local ko = {0xa, 0xa, 0x100, 0x800, 0x200}
    for id = 0, 255 do
        buffer.writestring(msg3, id * 17, string.format("PERSONA %03d", id))
        local flags = if id >= 0xc0 then 0x30 elseif id % 37 == 0 then 0x22 elseif id % 11 == 0 then 0x80 else 0
        local level = 1 + (id * 7) % 90
        buffer.writeu16(persona0, id * 14, flags)
        buffer.writeu8(persona0, id * 14 + 2, if id == 0 then 0 else 1 + id % 21)
        buffer.writeu8(persona0, id * 14 + 3, level)
        for i = 0, 4 do
            buffer.writeu8(persona0, id * 14 + 4 + i, math.min(99, 2 + rand(level + 3)))
            buffer.writeu8(persona1, id * 70 + i, 5 * (1 + rand(5)))
        end
        buffer.writeu8(persona0, id * 14 + 9, rand(8))
        local events = 3 + rand(8)
        local used = {}
        local offset = 0
        for slot = 0, events - 1 do
            local skill
            repeat
                local band = rand(10)
                skill = if band < 4 then 1 + rand(79) elseif band < 5 then 80 + rand(20)
                    elseif band < 6 then 141 + rand(29) elseif band < 7 then 171 + rand(49)
                    elseif band < 8 then 100 + rand(40) else 440 + rand(136)
            until not used[skill]
            used[skill] = true
            buffer.writeu8(persona1, id * 70 + 6 + slot * 4, offset)
            buffer.writei8(persona1, id * 70 + 6 + slot * 4 + 1, 1)
            buffer.writeu16(persona1, id * 70 + 6 + slot * 4 + 2, skill)
            if slot >= 1 then
                offset += 1 + rand(4)
            end
        end
        for column = 0, 15 do
            local word = if column <= 4 then damage[1 + rand(#damage)] elseif column == 6 or column == 7
                then ko[1 + rand(#ko)] else 0x14
            buffer.writeu16(unit2, id * 32 + column * 2, word)
        end
    end
    -- Arcana charts: the diagonal is its own arcana, other cells a different
    -- arcana or (rarely) no fusion; the alternate pair chart differs in two cells.
    local charts = {}
    for k = 0, 3 do
        local chart = buffer.create(526)
        for a = 1, 22 do
            for b = a, 22 do
                local value = a
                if b ~= a then
                    repeat
                        value = 1 + rand(22)
                    until value ~= a and value ~= b
                    if rand(12) == 0 then
                        value = 0
                    end
                end
                buffer.writeu8(chart, a * 24 + b - 25, value)
            end
        end
        charts[k] = chart
    end
    buffer.copy(charts[1], 0, charts[0])
    buffer.writeu8(charts[1], 1 * 24 + 5 - 25, 7)
    buffer.writeu8(charts[1], 16 * 24 + 20 - 25, 18)
    -- Fusion gates: s32 story flag, u16 persona, u16 flags (2: story-locked).
    local gates = buffer.create(23 * 8)
    for i = 0, 22 do
        buffer.writeu32(gates, i * 8, 672 + i)
        buffer.writeu16(gates, i * 8 + 4, 4 + i * 7)
        buffer.writeu16(gates, i * 8 + 6, if i % 3 == 0 then 0 else 2)
    end
    -- Enemies (level +3), names, formations of one to five enemies, two-variant
    -- weighted encounter tables, treasure tables and items (kind +0, price +0x34).
    local unit0 = buffer.create(336 * 60)
    local msg2 = buffer.create(336 * 21)
    for enemy = 1, 335 do
        local level = 1 + (enemy * 3) % 90
        local row = enemy * 60
        buffer.writeu16(unit0, row, if enemy % 37 == 0 then 1 else 0) -- an occasional boss flag
        buffer.writeu8(unit0, row + 3, level)
        buffer.writeu16(unit0, row + 4, 20 + level * 12 + rand(40))
        buffer.writeu16(unit0, row + 6, level * 3 + rand(20))
        for stat = 0, 4 do
            buffer.writeu8(unit0, row + 8 + stat, 2 + level // 3 + rand(5))
        end
        buffer.writeu16(unit0, row + 0x1e, level * 4 + rand(10))
        buffer.writeu16(unit0, row + 0x20, level * 6 + rand(10))
        for drop = 0, 1 do
            buffer.writeu16(unit0, row + 0x22 + drop * 4, 0x500 + rand(200))
            buffer.writeu8(unit0, row + 0x24 + drop * 4, 30 + rand(50))
        end
        buffer.writeu16(unit0, row + 0x3a, 30 + level * 2)
        buffer.writestring(msg2, enemy * 21, string.format("ENEMY %03d", enemy))
    end
    local encount0 = buffer.create(912 * 24)
    for formation = 40, 911 do
        local count = 1 + rand(5)
        local base = 1 + rand(320)
        for slot = 0, count - 1 do
            buffer.writeu16(encount0, formation * 24 + 8 + slot * 2, base + rand(12))
        end
    end
    local encount3 = buffer.create(56 * 0xf8)
    for t = 1, 55 do
        for variant = 0, 1 do
            for slot = 0, 29 do
                if rand(4) ~= 0 then
                    local at = t * 0xf8 + 8 + variant * 120 + slot * 4
                    buffer.writeu16(encount3, at, 40 + rand(872))
                    buffer.writeu16(encount3, at + 2, 1 + rand(30))
                end
            end
        end
    end
    local items = buffer.create(1536 * 68)
    for item = 1, 1535 do
        buffer.writeu16(items, item * 68, item // 256)
        buffer.writeu32(items, item * 68 + 0x34, if item % 9 == 0 then 0 else 10 * (1 + rand(5000)))
        -- Unlock gates: the opening stock (material 0x500) or a material and count.
        local material = if item % 5 == 0 then 0x500 else 0x501 + item % 60
        buffer.writeu32(items, item * 68 + 0x3c, material * 256 + item % 11)
    end
    local encount4 = buffer.create(75 * 0x15c)
    for block = 1, 74 do
        for record = 0, 28 do
            if rand(3) ~= 0 then
                local at = block * 0x15c + record * 12
                buffer.writeu16(encount4, at, 10 + rand(200))
                buffer.writeu16(encount4, at + 2, 1 + rand(1535))
                buffer.writeu8(encount4, at + 4, rand(2))
                buffer.writeu8(encount4, at + 6, 1)
            end
        end
    end
    return {SKILL0 = skill0, SKILL1 = skill1, PERSONA0 = persona0, PERSONA1 = persona1, UNIT2 = unit2,
            MSG1 = msg1, MSG3 = msg3, FUSION0 = charts[0], FUSION1 = charts[1], FUSION2 = charts[2],
            FUSION3 = charts[3], GATES = gates, UNIT0 = unit0, MSG2 = msg2, ENCOUNT0 = encount0,
            ENCOUNT3 = encount3, ENCOUNT4 = encount4, ITEMS = items, ITEMTBL = items}
end

-- Banks the driver extracted from the user's disc: <dir>/<KEY>.bin.
function F.diskBanks(directory: string): {[string]: buffer}?
    local banks = {}
    for _, key in {"SKILL0", "SKILL1", "PERSONA0", "PERSONA1", "UNIT2", "MSG1", "MSG3", "FUSION0", "FUSION1",
                   "FUSION2", "FUSION3", "GATES", "UNIT0", "MSG2", "ENCOUNT0", "ENCOUNT3", "ENCOUNT4", "ITEMS"} do
        local bytes = read_file(directory .. "/" .. key .. ".bin")
        if bytes == nil then
            return nil
        end
        banks[key] = bytes
    end
    banks.ITEMTBL = banks.ITEMS -- the table service's retail item table bank
    return banks
end

local OVERLAYABLE = {PERSONA0 = true, PERSONA1 = true, UNIT0 = true, UNIT1 = true, UNIT2 = true, ENCOUNT0 = true,
                     ENCOUNT1 = true, ENCOUNT2 = true, ENCOUNT3 = true, ENCOUNT4 = true, FUSION0 = true,
                     FUSION1 = true, FUSION2 = true, FUSION3 = true, ITEMTBL = true}

-- The region holding [address, address + size), if any.
local function region(address: number, size: number): (buffer?, number)
    for base, bytes in F.regions do
        if address >= base and address + size <= base + buffer.len(bytes) then
            return bytes, address - base
        end
    end
    return nil, 0
end

local function subscription(name: string, callback: any): any
    local entry = {name = name, callback = callback, enabled = true}
    function entry:enable() self.enabled = true end
    function entry:disable() self.enabled = false end
    function entry:remove() self.enabled = false; self.removed = true end
    F.events[name] = F.events[name] or {}
    table.insert(F.events[name], entry)
    return entry
end

-- Fires an event; returns how many enabled callbacks ran. Each callback is one
-- budget invocation: F.maxCallback keeps the most units one of them used.
function F.fire(name: string, payload: any?): number
    local ran = 0
    for _, entry in F.events[name] or {} do
        if entry.enabled then
            ran += 1
            local outer = F.resumeAt
            F.resumeAt = safepoints()
            entry.callback(payload or {name = name})
            local used = safepoints() - F.resumeAt
            if used > F.maxCallback then
                F.maxCallback, F.maxCallbackAt = used, name .. "#" .. #F.logs .. ":" .. tostring(F.logs[#F.logs])
            end
            F.resumeAt = outer
        end
    end
    return ran
end

-- Runs frames until no frame_start callback is enabled (or the limit).
function F.frames(limit: number): number
    for frame = 1, limit do
        if F.fire("frame_start", {name = "frame_start", frame = frame}) == 0 then
            return frame - 1
        end
    end
    return limit
end

local function copy(bytes: buffer): string
    return buffer.tostring(bytes)
end

p4 = {
    describe = function(address: number): any
        local signature = F.seams[address]
        return if signature then {address = address, signature = signature, hooked = false} else nil
    end,
    call = function(address: number, signature: string, ...: number): number
        local fn = F.calls[address]
        assert(fn ~= nil and F.seams[address] == signature, "fixture cannot call " .. tostring(address))
        return fn(...)
    end,
    -- Hook callbacks are fixture events named "hook:<address>:<phase>".
    hook = function(address: number, signature: string, phase: string, callback: any): any
        assert(F.seams[address] == signature, "fixture has no seam " .. tostring(address))
        return subscription(string.format("hook:%08x:%s", address, phase), callback)
    end,
    api_version = "1.6",
    language = "Luau",
    language_version = "0.744",
    id = "p4pc.randomizer",
    sha256 = sha256,
    log = function(message: string)
        table.insert(F.logs, message)
        if F.verbose then
            emit("LOG " .. message)
        end
    end,
    -- Pacing fixture: "large" never yields; "every" yields on every Nth check;
    -- "starved" has no budget left;
    -- "game" emulates one 4096-unit callback per resume from the VM's real
    -- interrupt count (F.resumeAt is set by the caller before each resume).
    budget = function(): number
        F.paceCalls += 1
        F.maxHeap = math.max(F.maxHeap, heap())
        if F.budgetMode == "every" and F.paceCalls % F.yieldEvery == 0 then
            return 0
        elseif F.budgetMode == "starved" then
            return 0 -- a presentation frame other callbacks already spent
        elseif F.budgetMode == "game" then
            local now = safepoints()
            local used = now - F.resumeAt
            F.maxUsed = math.max(F.maxUsed, used)
            -- The longest stretch between two pacing checks inside one resume.
            F.maxGap = math.max(F.maxGap, now - math.max(F.lastPace, F.resumeAt))
            F.lastPace = now
            return math.max(0, F.callbackFuel - used)
        end
        return 4096
    end,
    settings = {
        get = function(key: string): any
            return F.settings[key]
        end,
        set = function(key: string, value: any): (boolean, string?)
            F.settings[key] = value
            return true
        end,
    },
    events = {
        on = function(name: string, callback: any): any
            return subscription(name, callback)
        end,
    },
    save = {
        register = function(spec: any)
            assert(F.save == nil, "one save provider")
            F.save = spec
        end,
    },
    ui = {
        page = function(spec: any): any
            F.page = spec
            return {key = "lua/p4pc.randomizer/" .. spec.key, open = function() return true end,
                    refresh = function() return true end}
        end,
        status = function(text: string?, placement: string?): boolean
            F.status = text
            return true
        end,
    },
    artifacts = {
        write = function(path: string, data: any): (boolean, string?)
            F.artifacts[path] = if type(data) == "string" then data else buffer.tostring(data)
            return true
        end,
        folder = function(): string?
            return "fixture/mod-artifacts/p4pc.randomizer"
        end,
    },
    tables = {
        stock = function(key: string): buffer
            local bank = F.banks[key]
            assert(bank ~= nil, "fixture has no bank " .. key)
            local out = buffer.create(buffer.len(bank))
            buffer.copy(out, 0, bank)
            return out
        end,
        bank = function(key: string): any
            local bank = F.banks[key]
            return {key = key, overlayable = OVERLAYABLE[key] == true, bytes = if bank then buffer.len(bank) else nil}
        end,
        overlay = function(layer: {[string]: buffer}): (boolean, string?)
            local copied = {}
            for key, bytes in layer do
                local bank = F.banks[key]
                if not OVERLAYABLE[key] or bank == nil or buffer.len(bank) ~= buffer.len(bytes) then
                    return false, "fixture refuses " .. key
                end
                copied[key] = copy(bytes)
            end
            F.overlay = copied
            F.overlays += 1
            return true
        end,
    },
    mem = {
        read = function(kind: string, address: number): number
            if address == 0x007642a0 then
                return 0x01000000 -- the item table's (fixture) base
            end
            local bytes, at = region(address, 4)
            if bytes then
                return if kind == "u16" then buffer.readu16(bytes, at) elseif kind == "u8" then buffer.readu8(bytes, at)
                    elseif kind == "i16" then buffer.readi16(bytes, at) elseif kind == "i32" then buffer.readi32(bytes, at)
                    else buffer.readu32(bytes, at)
            end
            return F.memory[address] or 0
        end,
        write = function(kind: string, address: number, value: number)
            local bytes, at = region(address, 2)
            if bytes and (kind == "i16" or kind == "u16") then
                buffer.writeu16(bytes, at, value % 65536)
                return
            end
            F.memory[address] = value
        end,
        read_bytes = function(address: number, size: number): buffer
            if address == 0x01000000 then
                local items = F.banks.ITEMS
                assert(items ~= nil and size == buffer.len(items), "fixture has no item table")
                local out = buffer.create(size)
                buffer.copy(out, 0, items)
                return out
            end
            if address == 0x006420a0 then
                local gates = F.banks.GATES
                assert(gates ~= nil and size == buffer.len(gates), "fixture has no fusion gates")
                local out = buffer.create(size)
                buffer.copy(out, 0, gates)
                return out
            end
            local bytes, offset = region(address, size)
            if bytes then
                local out = buffer.create(size)
                buffer.copy(out, 0, bytes, offset, size)
                return out
            end
            local out = buffer.create(size)
            for at, value in F.memory do
                if at >= address and at < address + size then
                    buffer.writeu8(out, at - address, value)
                end
            end
            return out
        end,
    },
}
