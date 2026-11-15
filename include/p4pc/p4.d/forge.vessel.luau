--!capability forge.vessel@1
-- p4.forge.vessel(character, item): how many unequipped copies p4.forge.commit would accept as the vessel.
--!in p4.forge
-- The unequipped count of item when it is a weapon (a known item row in
-- equipment band 0) that character, a recruited fighting party member, may
-- equip; 0 for anything else. This is commit's own vessel check, so the item's display name
-- plays no part: a weapon another mod renamed to a placeholder still counts.
-- IDs only (character 1-8, item 1-32765); refused in synchronous callbacks and
-- snapshots.
vessel: (character: number, item: number) -> number,
--!example
local copies: number = p4.forge.vessel(1, 2)
--!refuse
local copies: string = p4.forge.vessel(1, 2)
p4.forge.vessel(1)
