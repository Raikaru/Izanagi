--!strict
-- Authored modder example (not shipped; no game data): data.luau runs while the
-- game builds its tables, before the title screen, with p4.data, p4.log,
-- p4.has, p4.capabilities, p4.settings.get and p4.id only. Rows are found by
-- their disc names; mod_api_check type-checks this file against the shipped
-- definitions (docs/modding/systems/data-and-edits.md).
p4.data.edit(function(d: P4DataEdit)
    -- Fire is the castle's answer: Agilao hits harder and costs less.
    local agilao = d.skill("Agilao")
    agilao.power = 100
    agilao.cost = { sp = 3 }

    -- The castle's Hableries are weak to it and drop a Medicine half the time.
    local hablerie = d.enemy("Lying Hablerie")
    hablerie.hp = 300
    hablerie.affinities.fire = "weak"
    hablerie.drops[1] = { item = "Medicine", chance = 50 }

    -- Chie's Persona learns Bufula earlier.
    d.persona("Tomoe").learnset:add({ level = 12, skill = "Bufula" })

    -- Medicine is renamed, cheaper and sold from 4/20; the 5-Iron needs the
    -- weapon store to have bought three Supple Metal first.
    local medicine = d.item("Medicine")
    medicine.name = "Herbal Tonic"
    medicine.price = 100
    medicine.shop.from = { month = 4, day = 20 }
    d.item("5-Iron").shop.unlock = { { material = "Supple Metal", count = 3 } }

    -- Rare shadows show up more often on the third floor.
    d.floor("Yukiko's Castle 3F").encounters.rates.rare = 5
end)
