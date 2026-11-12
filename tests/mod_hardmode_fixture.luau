-- Synthetic p4 provider for the headless hard-mode lifecycle check
-- (tests/mod_hardmode_lifecycle_test.py). It is the first chunk run by
-- luau_script_runner: it defines the global p4 that mods/p4pc.hardmode/main.luau
-- runs against, plus FIXTURE for the test chunk. No game and no retail bytes:
-- memory reads are zero, native calls answer from FIXTURE.calls, and the
-- tuning file is the shipped one. The mod's own save, reset and load callbacks
-- are the ones it registers; this file only stands in for the game around them.
local args = {...}
local MAIN, TUNING = args[1], args[2]
assert(MAIN and TUNING, "usage: mod_hardmode_fixture.luau -- <main.luau> <TUNING.TXT>")

local F = {
    settings = {mode = "Hard", enemy_stats = true, smart_ai = true, rewards = true, technical = true,
                windows = true, rebalance = true, party = true, gear = true, boss_rules = true,
                adopt_on_load = true} :: {[string]: any},
    logs = {} :: {string},
    hooks = 0,
    events = {} :: {[string]: number},
    save = nil :: any,
    -- The registered Camp overlay's specification, nil when none is.
    camp = nil :: any,
    -- ui_overlay_native.c: P4_UI_SPRITES (32) synthetic sprite records shared by
    -- every mod, released only when the mod's VM closes (p4_ui_overlay_release_owner);
    -- removing an overlay frees its items, not the sprites they show.
    spritePool = 32,
    sprites = 0,
    -- p4.call results by address; datCalcGetLevelById (protagonist level).
    calls = {[0x00104c70] = 12} :: {[number]: number},
    -- The hook catalog the mod checks its seams against, as main.luau declares them.
    seams = {} :: {[number]: string},
}
FIXTURE = F

do
    local text = buffer.tostring(assert(read_file(MAIN), "cannot read " .. MAIN))
    for address, signature in string.gmatch(text, 'seam%("[%w_]+", (0x%x+), "(%w+)"%)') do
        F.seams[tonumber(address) :: number] = signature
    end
end

local function subscription(): any
    local s = {enabled = true}
    function s.enable(self: any) self.enabled = true end
    function s.disable(self: any) self.enabled = false end
    function s.remove(self: any) self.enabled = false end
    return s
end

p4 = {
    log = function(text: string)
        table.insert(F.logs, text)
    end,
    describe = function(address: number): any
        local signature = F.seams[address]
        return if signature then {address = address, signature = signature} else nil
    end,
    hook = function(address: number, signature: string, phase: string, fn: any): any
        assert(F.seams[address] == signature, string.format("hook 0x%08x %s outside the catalog", address, signature))
        F.hooks += 1
        return subscription()
    end,
    call = function(address: number, signature: string, ...: any): number
        return F.calls[address] or 0
    end,
    mem = {
        read = function(kind: string, address: number): number return 0 end,
        write = function(kind: string, address: number, value: number) end,
        alloc = function(size: number): any return {address = 0x01f00000, size = size} end,
        write_string = function(pin: any, capacity: number, text: string) end,
    },
    files = {
        read = function(path: string, limit: number): buffer
            assert(path == "DATA/HARDMODE/TUNING.TXT", "fixture has no file " .. path)
            return assert(read_file(TUNING), "cannot read " .. TUNING)
        end,
    },
    events = {
        on = function(name: string, fn: any): any
            F.events[name] = (F.events[name] or 0) + 1
            return subscription()
        end,
    },
    damage = {on = function(spec: any): any return subscription() end},
    chance = {on = function(spec: any): any return subscription() end},
    clock = {
        retail = function(): number return 1 end,
        updates = function(): number return 0 end,
    },
    battle = {
        menu = function(): any return nil end,
        units = function(): any return nil end,
        substitute = function(action: number, skill: number): (boolean, string?) return false, "no battle" end,
    },
    contract = {
        read = function(owner: string, name: string): any return nil end,
    },
    mods = {
        required = function(id: string): any
            assert(id == "p4pc.physsplit", "fixture requires no " .. id)
            return {id = id, version = "1.0.0"}
        end,
    },
    settings = {
        get = function(key: string): any return F.settings[key] end,
        set = function(key: string, value: any): (boolean, string?)
            F.settings[key] = value
            return true, nil
        end,
    },
    save = {
        register = function(spec: any)
            assert(F.save == nil, "one save block per mod")
            F.save = spec
        end,
    },
    ui = {
        sprite = function(image: string, rect: any): (any, string?)
            assert(image == "badge" and rect.x == 0 and rect.y == 0 and rect.w == 58 and rect.h == 21,
                "fixture has no sprite " .. image)
            if F.sprites >= F.spritePool then
                return nil, "no free overlay capacity"
            end
            F.sprites += 1
            return {width = rect.w, height = rect.h}, nil
        end,
        overlay = function(spec: any): (any, string?)
            assert(F.camp == nil, "one Camp overlay at a time")
            assert(spec.context == "camp" and spec.items[1].key == "badge" and spec.items[1].sprite ~= nil,
                "the Camp badge overlay shows the badge sprite")
            F.camp = spec
            local overlay = {}
            function overlay.set(self: any, item: string, text: string): boolean
                assert(F.camp == spec, "overlay was already removed")
                return true
            end
            function overlay.show(self: any, item: string, visible: boolean)
                assert(F.camp == spec, "overlay was already removed")
            end
            function overlay.remove(self: any)
                assert(F.camp == spec, "overlay was already removed")
                F.camp = nil
            end
            return overlay, nil
        end,
    },
}
