--!capability story.dungeon_cleared@1
-- p4.story.dungeon_cleared(n): read the game's saved rescue-clear flag for dungeon 1..7.
--!in p4.story
-- The current saved rescue-clear flag for dungeon 1..7; errors outside that range.
dungeon_cleared: (dungeon: number) -> boolean,
--!example
local cleared: boolean = p4.story.dungeon_cleared(1)
--!refuse
local cleared: string = p4.story.dungeon_cleared(1)
p4.story.dungeon_cleared("castle")
