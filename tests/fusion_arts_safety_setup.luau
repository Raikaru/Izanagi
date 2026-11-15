-- Test-only providers: the shipped script runs unmodified in the real Luau VM.
-- p4.equipment here stands in for the native provider's contract (validated in
-- tests/mod_equipment_test.c): a forged item fits only the fighter it was forged
-- for, check/equip refuse with a reason, and equip applies at a later safe point.
test = {events = {}, page = nil, mode = "success", result = nil, unnamed = 2, refused = 4,
    owner = {}, worn = {[1] = 1, [2] = 3}, joined = {[1] = true, [2] = true},
    full = false, queued = nil, applied = nil, equips = 0}
p4 = {}
p4.settings = {get = function() return true end}
p4.story = {dungeon_cleared = function() return true end}
p4.ui = {page = function(spec)
    test.page = spec
    spec.refresh = function() end
    return spec
end}
p4.velvet = {command = function()
    return {enable = function() end, disable = function() end}
end}
p4.events = {on = function(name, callback) test.events[name] = callback end}
p4.persona = {stock = function()
    return {
        {slot = 1, persona = 2, arcana = 2, level = 10, lead = true, registered = false},
        {slot = 2, persona = 1, arcana = 2, level = 10, lead = false, registered = false},
    }
end}
p4.social = {rank = function() return 0 end}
-- Only the semantic reads the page makes. There is no p4.call or p4.mem: any raw
-- read or write is an error, and items and money change only through providers.
-- Item test.unnamed stands for a weapon another mod renamed to a placeholder: the
-- catalog has no name for it and p4.inventory.count refuses it, as the real providers
-- do, yet it is still a weapon row the forge accepts as a vessel. Item test.refused
-- fails the forge's vessel check (not a weapon this fighter may equip).
p4.inventory = {
    count = function(id)
        if id == test.unnamed then error("p4.inventory.count: no such item") end
        return 1
    end,
    money = function() return 10000 end,
}
p4.data = {
    item = function(id)
        return {id = id, name = if id == test.unnamed then nil else "TEST", kind = "weapon", price = 100}
    end,
    persona = function(id) return {id = id, name = "TEST"} end,
}
p4.party = {hero_level = function() return 50 end}
-- p4.forge.vessel stands in for p4_mod_forge_vessel_count (tests/mod_forge_test.c):
-- IDs in commit's ranges only, and the count without reading any display name.
local function integer(value, low, high)
    assert(type(value) == "number" and value == math.floor(value) and value >= low and value <= high,
        "p4.forge.vessel: bad argument")
end
p4.forge = {
    vessel = function(character, item)
        integer(character, 1, 8)
        integer(item, 1, 0x7ffd)
        return if not test.joined[character] or item == test.refused then 0 else 1
    end,
    commit = function(request)
        test.committed = request
        if not test.joined[request.character] then return false, "Character has not joined" end
        if test.mode == "sync-refuse" then return false, "synchronous refusal" end
        local forged = test.mode ~= "async-refuse"
        if forged then test.owner[request.item] = request.character end
        test.result = {outcome = if forged then "forged" else "refused",
            reason = "asynchronous refusal", character = request.character, persona = request.persona,
            vessel = request.vessel, item = request.item, fee = request.fee}
        return true, nil
    end,
    result = function() local result = test.result; test.result = nil; return result end,
}
local function check(request)
    if request.slot ~= 0 then return nil, "slot" end
    if test.owner[request.item] ~= request.character then return nil, "not equipment this character can equip" end
    if test.worn[request.character] == request.item then return nil, "Item is already equipped" end
    if test.full then return nil, "The equipped item cannot return to the items: 99 are held" end
    return test.worn[request.character] or 0, nil
end
p4.equipment = {
    get = function(character, slot) return if slot == 0 then test.worn[character] or 0 else 0 end,
    check = check,
    equip = function(request)
        test.equips += 1
        if test.queued then return false, "Another equipment change is still pending" end
        local previous, why = check(request)
        if previous == nil then return false, why end
        test.queued = {character = request.character, slot = request.slot, item = request.item}
        return true, nil
    end,
    pending = function() return test.queued ~= nil end,
    result = function() local result = test.applied; test.applied = nil; return result end,
}
-- The safe point: revalidate, then exchange or refuse whole.
function test.apply(refusal)
    local request = assert(test.queued)
    test.queued = nil
    local previous, why = check(request)
    if refusal or previous == nil then
        test.applied = {outcome = "refused", reason = refusal or why, character = request.character,
            slot = request.slot, item = request.item, previous = 0}
        return
    end
    test.worn[request.character] = request.item
    test.owner[request.item] = nil
    test.applied = {outcome = "equipped", character = request.character, slot = request.slot,
        item = request.item, previous = previous}
end
p4.log = function() end
