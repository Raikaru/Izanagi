-- Synthetic provider for the actual Golden script; no game or retail data.
FIXTURE = {
    scene = {ready = true, context = "field", field = 8, room = 2},
    x = -594, z = -813, day = 145, slot = 3, pressed = false, passes = 0,
    status = nil, pages = {}, events = {}, opened = 0, active = nil, save = nil,
    request = "idle", detail = "", teaches = 0, slotSkill = 0, replaceResult = 1,
    addResult = 1, persona = 1, level = 20, bankLoaded = true, bankShort = false, passAllowed = true, hold = nil,
}
local f = FIXTURE
local source = buffer.create(4 + 36 + 16 + 7 * 20)
buffer.writeu32(source, 0, 1)
buffer.writestring(source, 4, "cmpPsSkillAddTable.ctd")
buffer.writeu32(source, 36, 16 + 7 * 20)
buffer.writeu32(source, 40, 7 * 20)
buffer.writeu32(source, 44, 7)
buffer.writeu16(source, 56, 1)
local party = buffer.create(0x26E)
buffer.writeu8(party, 4, 5)
buffer.writei8(party, 5, 1)
buffer.writeu16(party, 6, 9)
local shortParty = buffer.create(8)
p4 = {
    settings = {get = function(key) return key == "bike_rides" end},
    describe = function(_) return {signature = "i"} end,
    log = function(_) end,
    contract = {publish = function(...) end},
    call = function(address, ...)
        if address == 0x0010CC20 then f.teaches += 1; return f.addResult end
        if address == 0x0010CD70 then return f.replaceResult end
        return 1
    end,
    u64 = function(...) return 1 end,
    calendar = {
        pass = function()
            f.passes += 1
            if f.hold then return false, f.hold end
            if not f.passAllowed then return false, "busy" end
            f.request = "requested"
            return true
        end,
        hold = function() return f.hold end,
    },
    tables = {
        source = function() return source end,
        bank = function() return {bytes = if f.bankLoaded then 0x26E else nil} end,
        stock = function() return if f.bankShort then shortParty else party end,
    },
    save = {register = function(spec) f.save = spec end},
    events = {on = function(_, callback)
        local sub = {callback = callback, enabled = true}
        function sub:disable() self.enabled = false end
        function sub:enable() self.enabled = true end
        table.insert(f.events, sub)
        return sub
    end},
    ui = {
        page = function(spec)
            function spec:refresh() end
            function spec:open() f.opened += 1; f.active = self.key end
            f.pages[spec.key] = spec
            return spec
        end,
        active = function() return f.active end,
        status = function(rows, _) f.status = if rows then rows[1] else nil end,
        close = function() f.active = nil end,
        back = function() end,
    },
    scene = {
        state = function()
            local state = table.clone(f.scene)
            if f.active then state.ready = false end
            return state
        end,
        player_position = function()
            if not f.scene.ready or f.active then return nil, nil end
            return f.x, f.z
        end,
        status = function() return f.request, f.detail end,
    },
    input = {action = function(_) local p = f.pressed; f.pressed = false; return false, p end},
    mem = {read = function(_, address)
        if address == 0x00797B7A then return f.day end
        if address == 0x00797B7C then return f.slot end
        if address == 0x00764538 then return 1 end
        if address == 0x00796D94 + 2 * 0x88 + 2 then return f.persona end
        if address == 0x00796D94 + 2 * 0x88 + 4 then return f.level end
        if address == 0x00796D94 + 2 * 0x88 + 0x0C then return f.slotSkill end
        return 0
    end, read_string = function() return "SKILL" end},
    clock = {wall = function() return 0 end},
}
function FIXTURE.tick()
    for _, sub in f.events do if sub.enabled then sub.callback({}) end end
    f.pressed = false
end
