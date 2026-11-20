--!capability data.item_names@1
--!covers p4.data.edit
-- Retail items take new display names in data.luau (`d.item(...).name = "..."`) or data/edits.xml (`<item name= rename=/>`): menus, shops and messages show them, keys keep the disc's name (docs/modding/systems/data-and-edits.md).
--!example
p4.data.edit(function(d)
    d.item("Medicine").name = "Herbal Tonic"
end)
--!refuse
p4.data.edit(function(d) d.item("Medicine").name = 5 end)
