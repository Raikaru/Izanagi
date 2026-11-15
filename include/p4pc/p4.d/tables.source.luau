--!capability tables.source@1
-- p4.tables.source: a read-only copy of a raw file a selected derived table producer recorded.
--!in p4.tables
-- A copy (at most 4 MiB) of a raw file the named derived table producer
-- recorded while composing the active catalog, or nil; never live guest bytes.
source: (producer: string, name: string) -> buffer?,
--!example
local recorded: buffer? = p4.tables.source("p4pc:golden", "init/cmpTable.bin")
local size: number = if recorded then buffer.len(recorded) else 0
--!refuse
local s: string = p4.tables.source("p4pc:golden", "init/cmpTable.bin")
p4.tables.source(7)
