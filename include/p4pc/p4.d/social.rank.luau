--!capability social.rank@1
-- p4.social.rank(arcana): a Social Link's rank 0..10 as the game answers it, legal in every callback mode.
--!in p4.social
-- A Social Link's rank 0..10 by the game's arcana number; 0 without a record.
rank: (arcana: number) -> number,
--!example
local yosukeRank: number = p4.social.rank(1)
--!refuse
local r: string = p4.social.rank(1)
p4.social.rank("magician")
