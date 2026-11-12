--!capability scene.player-position@1
--!in p4.scene
-- Horizontal world coordinates in a ready field or floor, or nil when unavailable.
player_position: () -> (number?, number?),
--!example
local x, z = p4.scene.player_position()
--!refuse
p4.scene.player_position(1)
