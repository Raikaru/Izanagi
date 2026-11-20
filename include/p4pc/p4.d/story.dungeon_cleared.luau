--!capability story.dungeon_cleared@2
-- Saved post-boss completion; revision 2 adds Yomotsu Hirasaka at index 8.
--!in p4.story
-- Current-save completion, indices 1..8 in native dungeon order; errors outside that range.
dungeon_cleared: (dungeon: number) -> boolean,
--!example
local cleared: boolean = p4.story.dungeon_cleared(1)
local finalCleared: boolean = p4.story.dungeon_cleared(8)
--!refuse
local cleared: string = p4.story.dungeon_cleared(1)
p4.story.dungeon_cleared("castle")
