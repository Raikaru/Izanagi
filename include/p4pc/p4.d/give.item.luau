--!capability give.item@1
-- p4.give.item and p4.take.item: change a stack (1..99 per call) through the game's accessors; returns the change, the requested change and "queued ..." during a battle, or 0 and a refusal.
--!in p4.give
item: (item: P4ItemArgument, count: number?) -> (number, string?),
--!in p4.take
item: (item: P4ItemArgument, count: number?) -> (number, string?),
--!example
local given: number, why: string? = p4.give.item("Medicine", 3)
local taken: number = p4.take.item(1)
--!refuse
local ok: boolean = p4.give.item("Medicine", 3)
p4.take.item("Medicine", "all")
