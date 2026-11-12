--!capability calendar.hold@1
--!in p4.calendar
-- nil, or why the time is held: a story hold (retail activities refuse to spend it) or the slot's schedule still running; pass refuses too.
hold: () -> string?,
--!example
local why: string? = p4.calendar.hold()
--!refuse
p4.calendar.hold(1)
local held: boolean = p4.calendar.hold()
