-- Headless regression check of mods/p4pc.hardmode's window-boss weakness rule
-- (the third chunk run by luau_script_runner, after
-- tests/mod_hardmode_fixture.luau and main.luau). For a Golden-off and a
-- Golden-on set of loaded enemy rows it runs an OFF load, a HARD New Game and a
-- loaded run saved on the former HARDER tier, each to its first battle, and
-- emits every row afterwards for tests/mod_hardmode_weakness_test.py.
--
-- The rows stand in for the tables the game has loaded. Golden off: the
-- retail UNIT.TBL kinds of these enemies (decoded from the disc, 2026-11-10:
-- none of the window bosses is weak to anything). Golden on: Golden's
-- documented changes (docs/design: Shadow Yukiko gains an Ice weakness, the
-- Avenger Knight a Fire weakness, Shadow Kanji nulls Electric). The Charming
-- Prince (269) is weak to Ice in both and must stay so.
local F = FIXTURE
local save = F.save

local ROWS: {[string]: {[number]: {[string]: string}}} = {
    golden_off = {
        [310] = {physical = "resist", light = "null", dark = "null"},
        [258] = {fire = "drain", light = "null", dark = "null"},
        [259] = {light = "null", dark = "null"},
        [269] = {ice = "weak", wind = "resist", light = "null", dark = "null"},
    },
    golden_on = {
        [310] = {physical = "resist", fire = "weak", light = "null", dark = "null"},
        [258] = {fire = "drain", ice = "weak", light = "null", dark = "null"},
        [259] = {electric = "null", light = "null", dark = "null"},
        [269] = {ice = "weak", wind = "resist", light = "null", dark = "null"},
    },
}
-- A save block written by a run on the former HARDER tier.
local HARDER = {mode = "harder", stats = true, ai = true, rewards = true, technical = true, bosses = true,
    revision = 7, adopted = false, physsplit = "1.0.0", epoch = 1}

-- A load or a New Game undoes every session edit: the game's rows come back.
local function install(name: string)
    F.rows = {}
    for id, affinities in ROWS[name] do
        local copy = {}
        for element, kind in affinities do
            copy[element] = kind
        end
        F.rows[id] = {affinities = copy}
    end
end

local function report(name: string, mode: string)
    local ids = {}
    for id in F.rows do
        table.insert(ids, id)
    end
    table.sort(ids)
    for _, id in ids do
        local entries = {}
        for element, kind in F.rows[id].affinities do
            table.insert(entries, element .. "=" .. kind)
        end
        table.sort(entries)
        emit(string.format("ROW %s %s %d %s", name, mode, id, table.concat(entries, ",")))
    end
end

for _, name in {"golden_off", "golden_on"} do
    -- OFF: no run, the rows stay as loaded.
    F.settings.mode, F.settings.adopt_on_load = "Off", false
    install(name)
    save.load(nil, "reset")
    save.load(nil, "load")
    F.fire("battle_start", {})
    report(name, "off")

    -- HARD New Game: the reset marks it pending, the first battle arms it.
    F.settings.mode = "Hard"
    install(name)
    save.load(nil, "reset")
    F.fire("battle_start", {})
    F.fire("battle_start", {}) -- a later battle of the same run edits nothing more
    report(name, "hard")

    -- A run saved on the former HARDER tier, loaded.
    install(name)
    save.load(nil, "reset")
    assert(save.authorize_load(HARDER), "the HARDER block was refused")
    save.load(HARDER, "load")
    F.fire("battle_start", {})
    report(name, "harder")
end
emit("EDITS " .. tostring(F.sessionEdits))
