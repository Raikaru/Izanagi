-- Synthetic p4 provider for the headless hard-mode checks
-- (tests/mod_hardmode_lifecycle_test.py, tests/mod_hardmode_weakness_test.py,
-- tests/mod_hardmode_retry_test.py). It is the first chunk run by
-- luau_script_runner: it defines the global p4 that mods/p4pc.hardmode/main.luau
-- runs against, plus FIXTURE for the test chunk. No game and no retail bytes:
-- memory reads come from FIXTURE.memMap (zero elsewhere), native calls answer
-- from FIXTURE.calls, p4.data reads and session edits use the enemy rows a test
-- puts in FIXTURE.rows, events run through FIXTURE.fire, and the tuning file is
-- the shipped one. The mod's own save, reset and load callbacks, hooks and
-- defeat handler are the ones it registers; this file only stands in for the
-- game around them.
local args = {...}
local MAIN, TUNING = args[1], args[2]
assert(MAIN and TUNING, "usage: mod_hardmode_fixture.luau -- <main.luau> <TUNING.TXT>")

local F = {
    settings = {mode = "Hard", enemy_rules = true, boss_play = true, party_kit = true,
                adopt_on_load = true} :: {[string]: any},
    logs = {} :: {string},
    hooks = 0,
    events = {} :: {[string]: number},
    -- Event handlers by name, with their subscriptions: fire() runs the enabled ones.
    handlers = {} :: {[string]: {any}},
    -- The loaded enemy rows p4.data reads and p4.data.session.edit writes:
    -- enemy ID -> {affinities = {element -> kind}}. A test sets them.
    rows = {} :: {[number]: any},
    sessionEdits = 0,
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
    -- The step-down/retry checks (mod_hardmode_retry_test.luau) drive the
    -- hooks, memory and input the lifecycle check never fires.
    hookFns = {} :: {[number]: any},
    memMap = {} :: {[number]: number},
    callLog = {} :: {any},
    battleUnits = nil :: any,
    -- p4.battle.defeat (battle.defeat@1): the handler and its subscription.
    defeat = nil :: any,
    defeatSub = nil :: any,
    -- p4.input.action: the actions pressed in this poll.
    input = {} :: {[string]: boolean},
    -- The registered screen overlay's specification, nil when none is.
    screen = nil :: any,
    -- The hook catalog the mod checks its seams against, as main.luau declares them.
    seams = {} :: {[number]: string},
}
FIXTURE = F

-- Runs the enabled handlers of one event, in registration order, as the game does.
function F.fire(name: string, event: any)
    for _, entry in F.handlers[name] or {} do
        if entry.subscription.enabled then
            entry.fn(event)
        end
    end
end

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
        F.hookFns[address] = fn
        return subscription()
    end,
    call = function(address: number, signature: string, ...: any): number
        local args = table.pack(...)
        table.insert(F.callLog, {address = address, signature = signature, args = args})
        return F.calls[address] or 0
    end,
    mem = {
        read = function(kind: string, address: number): number return F.memMap[address] or 0 end,
        write = function(kind: string, address: number, value: number)
            F.memMap[address] = value
        end,
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
            local s = subscription()
            F.handlers[name] = F.handlers[name] or {}
            table.insert(F.handlers[name], {fn = fn, subscription = s})
            return s
        end,
    },
    data = {
        enemy = function(query: any): any
            local row = F.rows[query]
            assert(row ~= nil, "fixture has no enemy row " .. tostring(query))
            local affinities = {}
            for element, kind in row.affinities do
                affinities[element] = kind
            end
            return {id = query, affinities = affinities}
        end,
        session = {
            edit = function(fn: any, options: any?)
                F.sessionEdits += 1
                fn({enemy = function(query: any): any
                    local row = F.rows[query]
                    assert(row ~= nil, "fixture has no enemy row " .. tostring(query))
                    return row
                end})
            end,
        },
    },
    damage = {on = function(spec: any): any return subscription() end},
    chance = {on = function(spec: any): any return subscription() end},
    clock = {
        retail = function(): number return 1 end,
        updates = function(): number return 0 end,
    },
    battle = {
        menu = function(): any return nil end,
        units = function(): any return F.battleUnits end,
        substitute = function(action: number, skill: number): (boolean, string?) return false, "no battle" end,
        defeat = function(fn: any): any
            assert(F.defeat == nil, "one defeat handler at a time")
            F.defeat = fn
            F.defeatSub = subscription()
            return F.defeatSub
        end,
    },
    input = {
        action = function(name: string): (boolean, boolean, boolean)
            local down = F.input[name] == true
            return down, down, false
        end,
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
            local slot = if spec.context == "screen" then "screen" else "camp"
            if slot == "screen" then
                assert(F.screen == nil, "one screen overlay at a time")
                assert(spec.fade == nil and #spec.items >= 1 and #spec.items <= 4, "a screen overlay never fades")
            else
                assert(F.camp == nil, "one Camp overlay at a time")
                assert(spec.context == "camp" and spec.items[1].key == "badge" and spec.items[1].sprite ~= nil,
                    "the Camp badge overlay shows the badge sprite")
            end
            for _, entry in spec.items do
                assert(entry.text == nil or #entry.text <= 47, "overlay text over 47 glyphs")
            end
            F[slot] = spec
            local overlay = {}
            function overlay.set(self: any, item: string, text: string): boolean
                assert(F[slot] == spec, "overlay was already removed")
                for _, entry in spec.items do
                    if entry.key == item then entry.text = text end
                end
                return true
            end
            function overlay.show(self: any, item: string, visible: boolean)
                assert(F[slot] == spec, "overlay was already removed")
                for _, entry in spec.items do
                    if entry.key == item then entry.visible = visible end
                end
            end
            function overlay.remove(self: any)
                assert(F[slot] == spec, "overlay was already removed")
                F[slot] = nil
            end
            return overlay, nil
        end,
    },
}
