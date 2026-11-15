--!strict
-- Authored modder example (not shipped; no game data), the run-time half of
-- data_rebalance: read the tables every mod's data.luau produced, report
-- fields another mod also set, and make the castle's Hableries grow during a
-- run. mod_api_check type-checks it against the shipped definitions.

-- The disc name still finds the renamed row; its key never changes.
local medicine: P4Item = p4.data.item("Medicine")
p4.log(tostring(medicine.key) .. " is sold as " .. tostring(medicine.name) .. " for " .. tostring(medicine.price))

-- Two mods that set one field: the later one in the load order wins, never silently.
for _, c in p4.data.conflicts() do
    p4.log(tostring(c.key) .. " " .. c.field .. ": " .. table.concat(c.owners, ", ") .. " -> " .. c.winner)
end

-- Every won or lost battle makes them a little tougher for the rest of this
-- run; persist keeps the growth in the save, and loading it applies it again.
p4.events.on("battle_end", function(_event: P4BattleEndEvent)
    p4.data.session.edit(function(d: P4DataEdit)
        local hablerie = d.enemy("Lying Hablerie")
        hablerie.hp = math.min(hablerie.hp + 10, 999)
    end, { persist = true })
end)
