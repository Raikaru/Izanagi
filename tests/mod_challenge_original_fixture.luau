-- Synthetic host for the real Challenge script; no game data or guest access.
local F = {settings = {boss_cap = true}, events = {}, hooks = {}, authorizers = {}, level = 15, reads = 0, save = nil, status = nil}
FIXTURE = F
local function subscription()
    return {disable = function() end, enable = function() end, remove = function() end}
end
p4 = {
    log = function() end,
    settings = {get = function(key) return F.settings[key] end,
        set = function(key, value) F.settings[key] = value return true end},
    data = {original_enemy_level = function(id)
        F.reads += 1
        return {id = id, level = F.level, source = "BTL/BATTLE/UNIT.TBL", sha256 = string.rep("a", 64)}
    end, enemy = function() error("Challenge must not read the mutable catalog") end},
    ui = {detail = function() end, status = function(lines) F.status = lines return true end},
    save = {register = function(spec) F.save = spec end},
    hook = function(address, signature, phase, fn) F.hooks[address] = fn return subscription() end,
    events = {on = function(name, fn) F.events[name] = fn return subscription() end},
    authorize = {on = function(name, fn) F.authorizers[name] = fn return subscription() end},
    battle = {formation = function() return subscription() end},
    damage = {on = function() return subscription() end},
    mem = {alloc = function() return {address = 100} end, read = function() return 0 end},
    lifecycle = {session = function() return 1 end},
    call = function(address)
        if address == 0x00105510 then return 101 end
        if address == 0x00104c70 then return 5 end
        error("unexpected native call")
    end,
}
