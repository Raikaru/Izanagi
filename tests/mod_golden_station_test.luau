-- The production mod runs between the synthetic provider and this test.
local f = FIXTURE
local function shown(expected, name)
    f.tick()
    f.tick()
    assert((f.status ~= nil) == expected, name)
end
shown(true, "station")
f.x = -593; f.pressed = true; f.tick()
assert(f.opened == 0, "moving inside station cannot open")
shown(true, "stationary after moving")
f.x = -444; shown(true, "approach boundary")
f.x = -443; shown(false, "outside approach")
f.pressed = true; f.tick(); assert(f.opened == 0, "outside cannot open")
f.x = -594; f.z = -662; shown(false, "outside on z")
f.z = -813; f.scene.room = 1; shown(false, "other room")
f.scene.room = 2; f.scene.field = 9; shown(false, "other field")
f.scene.field = 8; f.scene.context = "dungeon"; shown(false, "other context")
f.scene.context = "field"; f.scene.ready = false; shown(false, "not ready")
f.scene.ready = true; f.x = nil; shown(false, "missing position")
f.x = -594; f.day = 144; shown(false, "before 8/24")
f.day = 145; f.slot = 2; shown(false, "lunchtime")
f.slot = 4; shown(true, "after school")
f.slot = 5; shown(false, "evening")
f.slot = 3; f.save.load({ridden = 145}, "load"); shown(false, "already ridden today")
f.day = 146; shown(true, "next day")
f.hold = "the 12/05 investigation holds the day"; shown(false, "story hold")
f.pressed = true; f.tick(); assert(f.opened == 0, "a story hold cannot open")
f.hold = nil; shown(true, "hold over")
f.pressed = true; f.tick(); assert(f.opened == 1 and f.active == "ride", "inside opens ride")
f.pages.ride2.adjust("future", 0)
f.x = 0
shown(false, "leaving cancels a staged ride")
assert(f.passes == 0, "staged ride must be rechecked at the station")
f.pressed = true; f.tick(); assert(f.opened == 1, "leaving prevents reopening")
f.x = -594
local function startRide(key)
    f.tick(); f.tick()
    f.pressed = true; f.tick()
    f.tick() -- an open menu has no ready position
    f.pages.ride2.adjust(key or "future", 0)
    f.tick(); f.tick()
end
startRide()
assert(f.passes == 1 and f.teaches == 0 and f.save.snapshot().ridden == 145, "queued pass commits nothing")
assert(not f.save.authorize_save(), "save refused while calendar result is unknown")
f.request, f.detail = "refused", "calendar changed"
f.tick()
assert(f.teaches == 0 and f.save.snapshot().ridden == 145, "late refusal commits nothing")
startRide()
f.request, f.detail = "done", "the calendar sequence did not start"
f.tick()
assert(f.teaches == 0 and f.save.snapshot().ridden == 145, "unsuccessful done commits nothing")
startRide()
f.request, f.detail = "done", "passed"
f.tick()
assert(f.teaches == 0 and f.save.snapshot().ridden == 145, "grant waits for ready frame")
f.tick()
local saved = f.save.snapshot()
assert(f.teaches == 1 and saved.ridden == 146 and saved.futures["2"] == 1, "successful pass commits ride")
f.save.load({waiting = {"ride:2:1"}}, "load")
f.slotSkill, f.replaceResult = 2, -1
f.pages.replace2.adjust("slot1", 0)
f.tick()
assert(f.save.snapshot().waiting[1] == "ride:2:1", "failed replacement keeps grant")
f.replaceResult = 1
f.pages.replace2.adjust("slot1", 0)
f.tick()
assert(f.save.snapshot() == nil, "successful replacement resolves grant")
-- Immediate refusal must not advance the ride.
f.save.load(nil, "reset")
f.passAllowed = false
local passes = f.passes
startRide()
assert(f.passes == passes + 1 and f.save.snapshot() == nil, "immediate refusal costs nothing")
f.passAllowed = true

-- A story hold that begins while the menu is open passes no time.
passes = f.passes
f.tick(); f.tick()
f.pressed = true; f.tick()
f.tick()
f.hold = "the last day holds the time"
f.pages.ride2.adjust("future", 0)
f.tick(); f.tick()
assert(f.passes == passes and f.save.snapshot() == nil, "hold after opening passes no time")
f.hold = nil

-- The slot's schedule still running holds it like a story hold: the menu
-- stays closed, and a pass refused at the boundary commits nothing.
local openedBefore = f.opened
f.hold = "the schedule still runs this time of day"
f.tick(); f.tick()
f.pressed = true; f.tick(); assert(f.opened == openedBefore, "a running schedule cannot open")
f.hold = nil
f.tick(); f.tick()
passes = f.passes
local teachesBeforeHold = f.teaches
startRide()
assert(f.opened == openedBefore + 1, "schedule ending permits opening")
assert(f.passes == passes + 1 and f.request == "requested", "pass queued once the schedule ended")
f.request, f.detail = "refused", "the schedule still runs this time of day"
f.tick(); f.tick()
assert(f.teaches == teachesBeforeHold and f.save.snapshot() == nil, "boundary schedule hold commits nothing")

-- Missing or truncated learnsets must not be mistaken for an empty result.
f.persona, f.bankLoaded = 0xC0, false
passes = f.passes
startRide("memories")
assert(f.passes == passes and f.save.snapshot() == nil, "unloaded learnset refuses ride")
f.bankLoaded, f.bankShort = true, true
startRide("memories")
assert(f.passes == passes and f.save.snapshot() == nil, "short learnset refuses ride")

-- A remembered skill with full slots waits; declining it survives a reload
-- and prevents that skill from being offered by a later memories ride.
f.bankShort, f.addResult = false, 0
startRide("memories")
assert(f.passes == passes + 1, "valid memories pass queued")
f.request, f.detail = "done", "passed; an event followed"
f.scene.ready = false
f.tick()
assert(f.save.authorize_save(), "confirmed earned ride can be saved before field ready")
local earnedSave = f.save.snapshot()
assert(earnedSave.earnedRide.member == 2 and earnedSave.earnedRide.skill == 9
    and earnedSave.earnedRide.day == f.day and not earnedSave.ridden, "passed ride is durable before ready")
f.save.load(earnedSave, "load")
local teachesBefore = f.teaches
f.scene.ready = true
f.tick()
local committed = f.save.snapshot()
assert(committed.waiting[1] == "ride:2:9" and committed.ridden == f.day and not committed.earnedRide,
    "reloaded ride commits its daily use and full-slot grant")
assert(f.teaches == teachesBefore + 1, "reloaded ride attempts its grant once")
f.tick(); f.tick()
f.save.load(committed, "load")
f.tick()
assert(f.teaches == teachesBefore + 1 and #f.save.snapshot().waiting == 1,
    "committed ride is not repeated after ticks or reload")
local opened = f.opened
f.pressed = true; f.tick()
assert(f.opened == opened and f.save.snapshot().ridden == f.day, "reloaded daily ride cannot be used twice")
f.pages.replace2.adjust("decline", 0)
f.tick()
local declined = f.save.snapshot()
assert(declined.taught[1] == "ride:2:9", "decline recorded")
f.save.load(declined, "load")
f.day += 1
f.addResult = 1
local teaches = f.teaches
startRide("memories")
f.request, f.detail = "done", "passed"
f.tick(); f.tick()
assert(f.teaches == teaches and not f.save.snapshot().waiting, "declined memory is not reoffered after reload")
emit("golden station behavior: PASS")
