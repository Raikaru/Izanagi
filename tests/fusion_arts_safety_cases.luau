-- These cases exercise the actual bundled page callbacks, with test providers.
local page = assert(test.page)
local tick = assert(test.events.frame_start)
-- The character step counts one fighter's vessels per frame; a frame here also runs
-- the frames that count finishes in, so every step reads settled rows.
local function frame()
    tick()
    for _ = 1, 16 do
        local counting = false
        for i = 1, page.count() do
            if page.row(i).value == "..." then counting = true end
        end
        if not counting then return end
        tick()
    end
    error("the vessel count never finished")
end
local function row(key)
    local index = page.find(key)
    return if index then page.row(index) else nil
end
local function click(key)
    local current = assert(row(key), "missing visible row " .. key)
    assert(not current.disabled, "disabled row " .. key)
    page.adjust(key, 0)
    frame()
end
-- A direct callback that bypasses the native disabled-row check.
local function force(key)
    page.adjust(key, 0)
    frame()
end
local function start(character, vessel)
    click("character:" .. character)
    click("vessel:" .. vessel)
    click("persona:2")
end
local function forge(character, vessel)
    test.mode = "success"
    start(character, vessel)
    click("forge")
    assert(string.find(row("outcome").label, "FORGED", 1, true) == 1)
end
local function forgedItem(character)
    for item, owner in test.owner do
        if owner == character then return item end
    end
    error("nothing forged for " .. character)
end
page.opened()
test.events.field_enter()
-- The count is spread over frames: the first build shows every fighter still counting.
tick()
assert(row("character:1").value == "..." and row("character:1").disabled)
frame()
assert(row("character:1").value ~= "...")

-- Owning a compatible vessel is not enough to forge for an unrecruited fighter.
test.joined[2] = false
page.opened()
frame()
assert(row("character:2").disabled and row("character:2").value == "NO VESSEL")
force("character:2") -- Even bypassing the disabled row exposes no vessels.
assert(row("vessel:39") == nil)
click("back")
test.joined[2] = true
page.opened()
frame()
assert(not row("character:2").disabled and row("character:2").value ~= "NO VESSEL")

-- EQUIP NOW: offered for the fighter the weapon was forged for, and exchanged
-- through the provider with exactly that fighter, slot and item.
forge(1, 3)
local item = forgedItem(1)
local equip = assert(row("equip"))
assert(equip.label == "EQUIP NOW" and equip.value == "YU" and not equip.disabled)
assert(string.find(equip.help, "in place of", 1, true))
click("equip")
assert(test.queued and test.queued.character == 1 and test.queued.slot == 0 and test.queued.item == item)
assert(row("equip") == nil, "EQUIP NOW stays hidden while the exchange is pending")
test.apply()
frame()
assert(string.find(row("outcome").label, "EQUIPPED", 1, true) == 1)
assert(test.worn[1] == item)
assert(row("equip") == nil, "an equipped result is not offered again")
click("done")

-- Re-forging the weapon Yu now wears offers nothing to equip.
forge(1, 3)
equip = assert(row("equip"))
assert(equip.disabled and string.find(equip.help, "already equipped", 1, true))
click("done")

-- A later synchronous denial for another fighter must not expose the old item.
forge(1, 3)
click("done")
test.mode = "sync-refuse"
start(2, 39)
local equips = test.equips
click("forge")
assert(string.find(row("outcome").label, "synchronous refusal", 1, true))
assert(row("equip") == nil)
force("equip")
assert(test.equips == equips and test.queued == nil and test.worn[2] == 3)
click("done")

-- Nor a commit-stage authorizer denial.
forge(1, 3)
test.mode = "async-refuse"
click("done")
start(2, 39)
click("forge")
assert(string.find(row("outcome").label, "asynchronous refusal", 1, true))
assert(row("equip") == nil)
force("equip")
assert(test.equips == equips and test.queued == nil and test.worn[2] == 3)
click("done")

-- Reopening the page forgets an unequipped result.
forge(1, 3)
page.opened()
frame()
force("equip")
assert(test.equips == equips and test.queued == nil)

-- The provider's refusal disables EQUIP NOW with its reason; a forced click
-- reports the refusal and queues nothing. (Yu wears his old sword again.)
test.worn[1] = 1
test.full = true
forge(1, 3)
equip = assert(row("equip"))
assert(equip.disabled and string.find(equip.help, "99 are held", 1, true))
force("equip")
assert(test.queued == nil and string.find(row("outcome").label, "NOT EQUIPPED", 1, true) == 1)
assert(test.worn[1] == 1)
test.full = false
click("done")
forge(1, 3)

-- State that changed before the safe point is refused whole; the result stays
-- offered while it still fits.
local before = test.worn[1]
click("equip")
test.apply("No unequipped copy of item")
frame()
assert(string.find(row("outcome").label, "NOT EQUIPPED: No unequipped copy", 1, true) == 1)
assert(test.worn[1] == before)
assert(row("equip") and not row("equip").disabled)
click("done")

-- Recruitment can change after selecting a valid vessel but before submitting.
-- The provider's refusal must not offer an earlier result to that fighter.
start(2, 39)
test.joined[2] = false
click("forge")
assert(string.find(row("outcome").label, "has not joined", 1, true))
assert(row("equip") == nil)
click("done")
test.joined[2] = true

-- A weapon another mod renamed to a placeholder (the catalog cannot name it and
-- p4.inventory.count refuses it) is still a vessel by the forge's own check: it is
-- offered under its ID and forges. An item that check refuses is not offered.
click("character:1")
local renamed = assert(row("vessel:" .. test.unnamed), "a renamed weapon stays a vessel")
assert(renamed.label == "ITEM " .. test.unnamed and renamed.value == "x1" and not renamed.disabled)
assert(row("vessel:" .. test.refused) == nil and row("vessel:3") ~= nil)
click("back")
forge(1, test.unnamed)
assert(test.committed.character == 1 and test.committed.vessel == test.unnamed)
click("done")
