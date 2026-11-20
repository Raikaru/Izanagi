--!strict
-- Show a HUD line for each battle that begins while the toggle is on.
p4.events.on("battle_start", function(_event)
    if p4.settings.get("notice") then
        if not p4.ui.status("A BATTLE HAS STARTED", "hud") then
            p4.log("No room for the battle notice")
        end
    end
end)

p4.events.on("battle_end", function(_event)
    p4.ui.status(nil)
end)
