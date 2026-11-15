--!strict
-- Authored mod-test input for p4.calendar.pass (calendar.pass@1, mod_calendar_game).
-- In the first ready field after the card's save loads it logs the date and
-- slot, passes the time slot once (and checks that a second request while the
-- first is in flight is refused), then logs the status and the read-back.
-- Not shipped; not a retail asset.
local DATE = 0x00797B7A -- s16 date index
local SLOT = 0x00797B7C -- u8 time slot
local PENDING_FLAG = 0x00797B80 -- u32 datGetPendingFlag
local PENDING_DATE = 0x00797B84 -- s16 datGetPendingDateIndex
local PENDING_SLOT = 0x00797B86 -- u8 datGetPendingTimeSlot
local frame = 0
local phase = "ready"
local loaded = false

local function log(text: string) p4.log("CALENDAR f=" .. tostring(frame) .. " " .. text) end
local function now(): string
    local s = p4.scene.state()
    return string.format("date=%d slot=%d ctx=%s ready=%s field=%d room=%d", p4.mem.read("i16", DATE),
        p4.mem.read("u8", SLOT), s.context, tostring(s.ready), s.field, s.room)
end
local function staged(): string
    return string.format("pending=%d pending_date=%d pending_slot=%d", p4.mem.read("u32", PENDING_FLAG),
        p4.mem.read("i16", PENDING_DATE), p4.mem.read("u8", PENDING_SLOT))
end

p4.save.register({
    schema = 1, max_bytes = 16,
    snapshot = function(): P4SaveValue return nil end,
    load = function(_value: P4SaveValue, origin: "load" | "reset") if origin == "load" then loaded = true end end,
})
p4.events.on("calendar_advance", function(e: P4CalendarEvent)
    log(string.format("EVENT calendar_advance %d/%d -> %d/%d", e.old_date, e.old_period, e.date, e.period))
end)

p4.events.on("frame_start", function(e: P4FrameEvent)
    frame = e.frame
    if phase == "ready" then
        if loaded and p4.scene.state().ready then
            if p4.settings.get("clear_staging") == true then
                -- Covers the request's own staging: clear the game's staged slot
                -- through its own setter (datSetPendingFlag, 0x00106320) first.
                p4.call(0x00106320, "vI", 0)
                log("CLEARED " .. staged())
            end
            log("READY " .. now() .. " " .. staged())
            local ok, why = p4.calendar.pass()
            log("PASS ok=" .. tostring(ok) .. " why=" .. tostring(why) .. " " .. staged())
            local again, busy = p4.calendar.pass()
            log("PASS_AGAIN ok=" .. tostring(again) .. " why=" .. tostring(busy))
            phase = if ok then "wait" else "done"
        end
    elseif phase == "wait" then
        local state, detail = p4.scene.status()
        if state == "done" or state == "refused" then
            log("PASS_DONE " .. state .. " " .. detail .. " " .. now())
            phase = "midnight"
        end
    elseif phase == "midnight" then
        -- From the last slot of the day the next one is tomorrow: refused.
        if p4.scene.state().ready then
            local ok, why = p4.calendar.pass()
            log("PASS_MIDNIGHT ok=" .. tostring(ok) .. " why=" .. tostring(why) .. " " .. now() .. " " .. staged())
            phase = "done"
        end
    elseif phase == "done" then
        log("DONE")
        phase = "idle"
    end
end)
