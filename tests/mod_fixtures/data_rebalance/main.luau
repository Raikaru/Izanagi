--!strict
-- Type-check example: substitute names from your own game's tables before use.
-- Read the tables produced by data.luau, report conflicting fields, and grow
-- a fictional enemy during a run.

-- The disc name still finds the renamed row; its key never changes.
local item: P4Item = p4.data.item("Example Item")
p4.log(tostring(item.key) .. " is sold as " .. tostring(item.name) .. " for " .. tostring(item.price))

-- Two mods that set one field: the later one in the load order wins, never silently.
for _, c in p4.data.conflicts() do
    p4.log(tostring(c.key) .. " " .. c.field .. ": " .. table.concat(c.owners, ", ") .. " -> " .. c.winner)
end

-- Every won or lost battle makes them a little tougher for the rest of this
-- run; persist keeps the growth in the save, and loading it applies it again.
p4.events.on("battle_end", function(_event: P4BattleEndEvent)
    p4.data.session.edit(function(d: P4DataEdit)
        local enemy = d.enemy("Example Enemy")
        enemy.hp = math.min(enemy.hp + 10, 999)
    end, { persist = true })
end)
