--!capability party.hero_level@1
-- p4.party.hero_level(): the protagonist's stored level, as the game's own level read returns it.
--!in p4.party
-- The protagonist's level as his unit record holds it (datCalcGetLevelById), or nil
-- and a reason when no game is loaded or he has no level yet. Unlike
-- p4.party.level("protagonist"), which derives it from his EXP, EXP awaiting a
-- level-up does not raise it.
hero_level: () -> (number?, string?),
--!example
local level: number?, why: string? = p4.party.hero_level()
--!refuse
local level: number = p4.party.hero_level()
local level: string? = p4.party.hero_level()
