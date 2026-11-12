--!capability data.original-enemy-level@1
-- Selected disc UNIT.TBL, before replacements or edits; SHA-256 identifies the
-- complete packed file, not an independently authenticated retail pressing.
export type P4OriginalEnemyLevel = { id: number, level: number, source: string, sha256: string }
--!in p4.data
-- Original-disc enemy IDs only; raises if unavailable or outside that domain.
original_enemy_level: (id: number) -> P4OriginalEnemyLevel,
--!example
local original: P4OriginalEnemyLevel = p4.data.original_enemy_level(1)
local level: number = original.level
--!refuse
p4.data.original_enemy_level("Slime")
