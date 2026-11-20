--!strict
-- Type-check example only: substitute names from your own game's tables
-- before using it. data.luau runs while the game builds its tables, with
-- p4.data, p4.log, p4.has, p4.capabilities, p4.settings.get and p4.id.
p4.data.edit(function(d: P4DataEdit)
    -- A fictional skill hits harder and costs less.
    local skill = d.skill("Example Skill")
    skill.power = 100
    skill.cost = { sp = 3 }

    -- A fictional enemy is weak to fire and drops an item half the time.
    local enemy = d.enemy("Example Enemy")
    enemy.hp = 300
    enemy.affinities.fire = "weak"
    enemy.drops[1] = { item = "Example Item", chance = 50 }

    -- A fictional Persona learns a skill earlier.
    d.persona("Example Persona").learnset:add({ level = 12, skill = "Example Skill" })

    -- Rename an item, change its shop date, and adjust a gear unlock.
    local item = d.item("Example Item")
    item.name = "Herbal Tonic"
    item.price = 100
    item.shop.from = { month = 4, day = 20 }
    d.item("Example Gear").shop.unlock = { { material = "Example Material", count = 3 } }

    -- Rare shadows show up more often on the third floor.
    d.floor("Example Floor 3F").encounters.rates.rare = 5
end)
