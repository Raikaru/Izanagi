--!capability capabilities@1
-- p4.has and p4.capabilities: the capability keys this build provides (mod.ini `requires=` checks the same set).
--!in p4
-- True when this build provides the key at that revision or later; "events.ailment" means "events.ailment@1".
has: (key: string) -> boolean,
-- Every key this build provides, with its revision.
capabilities: () -> { [string]: number },
--!example
local provided: boolean = p4.has("capabilities@1")
local all: { [string]: number } = p4.capabilities()
local revision: number? = all["capabilities"]
--!refuse
local n: number = p4.has("capabilities")
p4.has(1)
local s: string = p4.capabilities()
