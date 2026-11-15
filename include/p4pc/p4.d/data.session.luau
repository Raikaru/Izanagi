--!capability data.session@1
-- main.luau changes the tables the game has loaded, for this run: a save load or a new game undoes every session edit, then re-applies the persisted ones that save carries (docs/modding/systems/data-and-edits.md).
export type P4DataSessionOptions = {
    -- Keep these edits in the save: loading it applies them again.
    persist: boolean?,
}
--!in p4.data
session: {
    -- fn(d) gets the same views as data.luau and writes the loaded tables now. An error or a broken rule
    -- undoes the whole call. Item names and (with added items) item rows change only in data.luau.
    edit: (fn: (d: P4DataEdit) -> (), options: P4DataSessionOptions?) -> (),
},
--!example
p4.data.session.edit(function(d)
    d.enemy("Lying Hablerie").hp = 999
end)
p4.data.session.edit(function(d)
    d.skill("Agilao").power = 120
end, { persist = true })
--!refuse
p4.data.session.edit(function(d) d.enemy("Lying Hablerie").hp = 999 end, { persist = "yes" })
