-- Exercises the real script's arming, level hook, save and load callbacks.
local F = FIXTURE
F.save.load(nil, "reset")
F.events.field_enter()
assert(F.reads == 8 and F.status[2] == "CAP 15")
local saved = F.save.snapshot()
assert(saved.bossLevels.levels[1] == 15 and saved.bossLevels.source == "BTL/BATTLE/UNIT.TBL")
assert(saved.bossLevels.sha256 == string.rep("a", 64))
F.level = 45 -- the later provider value cannot change an already armed run
local context = {args = {101, 40}, next = function() end}
F.hooks[0x00231ef0](context)
assert(context.args[2] == 15 and F.reads == 8)
F.save.load(nil, "reset")
assert(F.save.authorize_load(saved))
F.save.load(saved, "load")
context = {args = {101, 40}, next = function() end}
F.hooks[0x00231ef0](context)
assert(context.args[2] == 15 and F.reads == 8)
local legacy = table.clone(saved)
legacy.bossLevels = nil
local allowed = F.save.authorize_load(legacy)
assert(not allowed)
local bad = table.clone(saved)
bad.bossLevels = {levels = {15}, source = "x", sha256 = string.rep("a", 64)}
assert(not F.save.authorize_load(bad))

-- The protection-item rule arms independently and is fixed by the saved run.
local protection = assert(F.authorizers.battle_protection_item)
local protectionContext = {character = 2, item = 0x340, element = 6}
local fusion = assert(F.authorizers.fusion)
local personaFusion = {kind = 0, materials = 2}
F.settings.no_battle_items = true
F.save.load(nil, "reset")
assert(protection(protectionContext)) -- no active run
F.events.field_enter()
local ok, why = protection(protectionContext)
assert(not ok and why == "NO ITEMS IN BATTLE challenge rule")
local noItems = F.save.snapshot()
F.settings.no_battle_items = false
assert(not protection(protectionContext))
F.save.load(nil, "reset")
assert(protection(protectionContext))
assert(F.save.authorize_load(noItems))
F.save.load(noItems, "load")
assert(not protection(protectionContext))
assert(fusion(personaFusion))

-- Exercise the actual Challenge authorizers and run lifecycle, not a duplicate
-- of their rule logic. Stage 0 is the offer; stage 1 is the forge safe point.
local weapon = assert(F.authorizers.weapon_fusion)
local offer = {stage = 0, character = 0, vessel = 0, persona = 0, signature = 0}
local commit = {stage = 1, character = 1, vessel = 2, persona = 7, signature = 0}
local function arm(noWeapon, noPersona)
    F.settings.boss_cap = false
    F.settings.no_weapon_fusion = noWeapon
    F.settings.no_fusion = noPersona
    F.save.load(nil, "reset")
    F.events.field_enter()
end
local function weaponDenied(context)
    local allowed, reason = weapon(context)
    assert(not allowed and reason == "NO WEAPON FUSION challenge rule")
end

arm(true, false)
weaponDenied(offer)
weaponDenied(commit)
assert(fusion(personaFusion))
local restricted = F.save.snapshot()
assert(restricted.rules.no_weapon_fusion and not restricted.rules.no_fusion)
F.settings.no_weapon_fusion = false
weaponDenied(commit) -- settings cannot change an armed run
F.save.load(nil, "reset")
assert(weapon(commit)) -- no armed run while loading
assert(F.save.authorize_load(restricted))
F.save.load(restricted, "load")
weaponDenied(offer)
weaponDenied(commit)
assert(fusion(personaFusion))

arm(false, true)
assert(weapon(offer) and weapon(commit))
assert(not fusion(personaFusion))
local old = F.save.snapshot()
old.rules.no_weapon_fusion = nil -- save made before this rule existed
F.settings.no_weapon_fusion = true
assert(F.save.authorize_load(old))
F.save.load(old, "load")
assert(weapon(offer) and weapon(commit) and not fusion(personaFusion))

arm(true, true)
weaponDenied(offer)
weaponDenied(commit)
assert(not fusion(personaFusion))
arm(false, false)
assert(weapon(offer) and weapon(commit) and fusion(personaFusion))
