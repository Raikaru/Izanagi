-- Headless qualification of mods/p4pc.randomizer (the third chunk run by
-- luau_script_runner; receives the Engine table main.luau returns). Emits one
-- line per fact; tests/mod_randomizer_engine_test.py recomputes the vectors
-- independently and compares the pinned cross-platform digests.
local Engine = ...
local F = FIXTURE
local hex = Engine.hex
local TWO32 = 4294967296

local function seedOf(n: number): string
    -- Canonical 128-bit seed value n (n < 2^53), as 16 big-endian bytes.
    local hi = math.floor(n / TWO32)
    return string.rep("\0", 8) .. string.pack(">I4I4", hi, n - hi * TWO32)
end

local mode = F.args[1] or "vectors"
-- Every feature this engine implements.
local ALL = 0
for _, feature in Engine.features do
    ALL = bit32.bor(ALL, bit32.lshift(1, feature.id - 1))
end

-- Vectors: SHA-256, stream derivation, xoshiro128**, below(), shuffle, counters
-- and the canonical run definition.
if mode == "vectors" then
    for _, message in {"", "abc", string.rep("a", 55), string.rep("a", 56), string.rep("a", 64), string.rep("\255", 1000)} do
        emit(string.format("SHA %d %s %s", #message, hex(message), hex(Engine.sha256(message))))
    end
    local seeds = {string.rep("\0", 16), string.rep("\255", 16), Engine.unhex("0123456789abcdeffedcba9876543210") :: string}
    for _, seed in seeds do
        for _, label in {"", "persona-skills/0001", "encounter/0012/0007/0023/slot2"} do
            local r = Engine.stream(seed, label)
            local outputs = {}
            for i = 1, 8 do
                outputs[i] = string.format("%08x", Engine.next(r))
            end
            emit(string.format("STREAM %s %s %s", hex(seed), hex(label), table.concat(outputs, " ")))
        end
    end
    -- An all-zero digest is impossible to request, but a zero state is the
    -- documented s0 = 1 rule: exercise the generator from that state directly.
    local zero = {s0 = 1, s1 = 0, s2 = 0, s3 = 0}
    local outputs = {}
    for i = 1, 8 do
        outputs[i] = string.format("%08x", Engine.next(zero))
    end
    emit("ZEROSTATE " .. table.concat(outputs, " "))
    local high = {s0 = 0xffffffff, s1 = 0xfffffffe, s2 = 0x80000000, s3 = 0xffffffff}
    outputs = {}
    for i = 1, 8 do
        outputs[i] = string.format("%08x", Engine.next(high))
    end
    emit("HIGHSTATE " .. table.concat(outputs, " "))
    for _, n in {1, 2, 3, 7, 1000, 0x80000001, 0xffffffff, TWO32} do
        local r = Engine.stream(seeds[3], "below")
        local values = {}
        for i = 1, 12 do
            values[i] = string.format("%d", Engine.below(r, n))
        end
        emit(string.format("BELOW %d %s", n, table.concat(values, " ")))
    end
    for _, bad in {0, -1, 1.5, TWO32 + 1} do
        local ok = pcall(Engine.below, Engine.stream(seeds[1], "bad"), bad)
        emit(string.format("BELOW_REFUSED %s %s", tostring(bad), tostring(not ok)))
    end
    local list = {}
    for i = 1, 12 do
        list[i] = i
    end
    Engine.shuffle(Engine.stream(seeds[3], "shuffle"), list)
    emit("SHUFFLE " .. table.concat(list, " "))
    for _, case in {{0, 0, 1}, {0, 0xffffffff, 1}, {0x1fffff, 0xffffffff, 2}, {0x7fffffff, 0xfffffffe, 0xffffffff}} do
        local hi, lo = Engine.counterAdd(case[1], case[2], case[3])
        emit(string.format("COUNTER %08x%08x+%d %s", case[1], case[2], case[3], Engine.counterHex(hi, lo)))
    end
    emit("COUNTER_OVERFLOW_REFUSED " .. tostring(not pcall(Engine.counterAdd, 0xffffffff, 0xffffffff, 1)))
    local def = {algorithm = Engine.algorithm, catalog = Engine.catalog, seed = seeds[3], options = 5,
                 fingerprint = Engine.sha256("fixture input")}
    emit("CANONICAL " .. hex(Engine.canonical(def)))
    emit("DIGEST " .. hex(Engine.runDigest(def)))
    emit("CODE " .. Engine.runCode(def))
    return
end

-- Generation on the synthetic banks: pinned digests (identical on every
-- target), independence from pacing, and per-feature independence.
local function definition(seed: string, options: number, banks: any): any
    return {algorithm = Engine.algorithm, catalog = Engine.catalog, seed = seed, options = options,
            fingerprint = Engine.fingerprint(banks, options)}
end

local function counts(result: any): string
    local parts = {}
    for _, feature in Engine.features do
        local c = result.counts[feature.code]
        if c then
            table.insert(parts, string.format("%s=%d/%d", feature.code, c.changed, c.eligible))
        end
    end
    return table.concat(parts, ",")
end

local function generateWith(budgetMode: string, every: number, def: any, banks: any): any
    F.budgetMode, F.yieldEvery, F.paceCalls = budgetMode, every, 0
    local thread = coroutine.create(Engine.generate)
    local ok, value = coroutine.resume(thread, def, banks)
    local yields = 0
    while ok and coroutine.status(thread) ~= "dead" do
        yields += 1
        ok, value = coroutine.resume(thread)
    end
    F.budgetMode = "large"
    if not ok then
        error(value)
    end
    return value, yields
end

if mode == "synthetic" then
    local banks = F.syntheticBanks()
    F.banks = banks
    emit("SYNTH_INPUT " .. hex(Engine.fingerprint(banks, ALL)))
    for _, seed in {seedOf(0), seedOf(1), string.rep("\255", 16)} do
        for _, options in {1, 2, 4, 7, 8, 16, 32, 64, 128, 256, ALL} do
            local def = definition(seed, options, banks)
            local result = Engine.generate(def, banks)
            emit(string.format("GEN %s %d %s %s %s", hex(seed), options, hex(result.digest), hex(result.layerHash),
                counts(result)))
        end
    end
    -- Pacing never changes a result: yield at every 1st, 7th and 100th check.
    local def = definition(seedOf(9), ALL, banks)
    local reference = Engine.generate(def, banks)
    for _, every in {1, 7, 100} do
        local result, yields = generateWith("every", every, def, banks)
        emit(string.format("PACED %d %d %s", every, yields, tostring(result.layerHash == reference.layerHash
            and result.spoiler == reference.spoiler)))
    end
    -- Feature switches do not shift another feature's mappings.
    local alone, together = {}, {}
    for _, options in {1, 2, 4, 8, 16, 32, 64, 128, 256} do
        alone[options] = Engine.generate(definition(seedOf(9), options, banks), banks)
    end
    -- R6 profiles follow R5's placement: alone they match the run without R5.
    local withoutPlacement = Engine.generate(definition(seedOf(9), bit32.band(ALL, bit32.bnot(16)), banks), banks)
    -- R5 depends on R6 only: without it a move must already fit the slot's budget,
    -- so R5 alone matches the run without R6, and R5 with R6 the full run.
    local withoutProfiles = Engine.generate(definition(seedOf(9), bit32.band(ALL, bit32.bnot(32)), banks), banks)
    local placedScaled = Engine.generate(definition(seedOf(9), 16 + 32, banks), banks)
    local all = reference.layer
    local same = buffer.tostring(alone[4].layer.UNIT2) == buffer.tostring(all.UNIT2)
        and buffer.tostring(alone[2].layer.PERSONA0) == buffer.tostring(all.PERSONA0)
        and buffer.tostring(alone[16].layer.ENCOUNT3) == buffer.tostring(withoutProfiles.layer.ENCOUNT3)
        and buffer.tostring(placedScaled.layer.ENCOUNT3) == buffer.tostring(all.ENCOUNT3)
        and buffer.tostring(alone[64].layer.UNIT0) == buffer.tostring(all.UNIT0)
        and buffer.tostring(alone[128].layer.ENCOUNT4) == buffer.tostring(all.ENCOUNT4)
        and buffer.tostring(alone[256].layer.ITEMTBL) == buffer.tostring(all.ITEMTBL)
        and alone[256].counts.R9.changed > 0
        and buffer.tostring(alone[32].layer.R6) == buffer.tostring(withoutPlacement.layer.R6)
        and buffer.tostring(alone[32].layer.R6) ~= buffer.tostring(all.R6)
        and alone[32].counts.R6.changed > 0
    for _, key in {"FUSION0", "FUSION1", "FUSION2", "FUSION3"} do
        same = same and buffer.tostring(alone[8].layer[key]) == buffer.tostring(all[key])
    end
    -- R1 and R2 share PERSONA1: R1 owns the learn events, R2 the growth weights.
    local r1, r2, both = alone[1].layer.PERSONA1, alone[2].layer.PERSONA1, all.PERSONA1
    for id = 0, 255 do
        if buffer.readstring(r1, id * 70 + 6, 64) ~= buffer.readstring(both, id * 70 + 6, 64)
            or buffer.readstring(r2, id * 70, 6) ~= buffer.readstring(both, id * 70, 6) then
            same = false
        end
    end
    emit("INDEPENDENT " .. tostring(same))
    -- A different input fingerprint is a different run; a refused definition errors.
    emit("WRONG_ALGORITHM_REFUSED " .. tostring(not pcall(Engine.generate,
        {algorithm = 2, catalog = 1, seed = seedOf(0), options = 1, fingerprint = def.fingerprint}, banks)))
    return
end

-- The run lifecycle through main.luau's own callbacks (the fixture p4).
if mode == "lifecycle" then
    local banks = F.syntheticBanks()
    F.banks = banks
    local save = F.save
    assert(save and F.page, "main.luau registered its save block and page")
    local function has(prefix: string): boolean
        for _, line in F.logs do
            if string.sub(line, 1, #prefix) == prefix then
                return true
            end
        end
        return false
    end
    -- The seed page notes limited enemy variety only for R5 without R6.
    local function noted(): boolean
        local at = F.page.find("variety")
        return at ~= nil and F.page.count() == 12 and F.page.row(at).value == "ENEMY VARIETY IS LIMITED WITHOUT R6"
            and F.page.row(at - 1).key == "next"
    end
    F.settings.randomize, F.settings.r5_enemies = true, true
    local alone = noted()
    F.settings.r6_enemy_stats = true
    local scaled = noted()
    F.settings.randomize, F.settings.r5_enemies, F.settings.r6_enemy_stats = false, false, false
    emit(string.format("VARIETY_NOTE %s %s %s", tostring(alone), tostring(scaled), tostring(noted())))
    -- Settings OFF: a reset arms nothing and no callback is enabled.
    save.load(nil, "reset")
    emit("OFF_FRAMES " .. tostring(F.frames(10)))
    emit("OFF_FIELD_CALLBACKS " .. tostring(F.fire("field_enter")))
    emit("OFF_OVERLAYS " .. tostring(F.overlays))
    -- ON: speculative generation during the title, then the New Game arms.
    F.settings.randomize = true
    for _, feature in Engine.features do
        F.settings[feature.key] = true
    end
    save.load(nil, "reset")
    -- Frames whose budget is already spent do no work and never fail.
    F.budgetMode = "starved"
    F.frames(50)
    local starved = not has("RANDO_INPUT") and not has("RANDO_GENERATE")
    -- The game's budget from here through the New Game's publication: no one
    -- callback (generation steps, arming, overlay, hooks, spoiler) may near 4096.
    F.budgetMode, F.maxCallback = "game", 0
    -- The stock banks, the fingerprint and the first generation steps; then the
    -- New Game arms before its generation is done and publishes in a later frame.
    F.frames(3)
    F.fire("field_enter")
    local frames = F.frames(100000)
    emit("TITLE_FRAMES_DONE " .. tostring(frames < 100000) .. " " .. tostring(starved))
    -- The same with callbacks that start just above the pacing margin: the
    -- finished generation leaves too little for publishing in its own frame,
    -- which a later frame does.
    local tight = has("RANDO_RUN new_game")
    F.callbackFuel = 2200
    save.load(nil, "reset")
    local before = #F.logs
    F.frames(3)
    F.fire("field_enter")
    F.frames(100000)
    local published = false
    for index = before + 1, #F.logs do
        published = published or string.sub(F.logs[index], 1, 17) == "RANDO_RUN new_gam"
    end
    F.callbackFuel = 4096
    emit("TIGHT_PUBLISHED " .. tostring(tight and published))
    emit("CALLBACK_WORST " .. tostring(F.maxCallback < 3500) .. " " .. F.maxCallback .. " " .. F.maxCallbackAt)
    F.budgetMode = "large"
    local def = definition(Engine.unhex(F.settings.seed) :: string, ALL, banks)
    local expected = Engine.generate(def, banks)
    local applied = F.overlay.PERSONA0 == buffer.tostring(expected.layer.PERSONA0)
        and F.overlay.PERSONA1 == buffer.tostring(expected.layer.PERSONA1)
        and F.overlay.UNIT2 == buffer.tostring(expected.layer.UNIT2)
    emit("NEW_GAME_APPLIED " .. tostring(applied) .. " " .. tostring(has("RANDO_RUN new_game")))
    local folder = "runs/" .. string.sub(hex(expected.digest), 1, 16)
    emit("SPOILER_WRITTEN " .. tostring(F.artifacts[folder .. "/spoiler.txt"] == expected.spoiler
        and F.artifacts[folder .. "/manifest.json"] == expected.manifest))
    -- The written text is dropped from memory; an export regenerates it first.
    F.artifacts = {}
    F.page.adjust("export", 0)
    F.frames(100000)
    emit("SPOILER_REEXPORTED " .. tostring(has("RANDO_SPOILER_REGENERATING")
        and F.artifacts[folder .. "/spoiler.txt"] == expected.spoiler
        and F.artifacts[folder .. "/manifest.json"] == expected.manifest))
    -- A second field does not re-arm.
    local before = F.overlays
    F.fire("field_enter")
    emit("REARM " .. tostring(F.overlays ~= before))
    -- Save, reset (the native commit restored the banks), load: the run returns.
    local value = save.snapshot()
    local allowed = save.authorize_load(value)
    F.overlay = {}
    save.load(nil, "reset")
    save.load(value, "load")
    emit("LOAD_AUTHORIZED " .. tostring(allowed))
    emit("LOAD_APPLIED " .. tostring(F.overlay.PERSONA1 == buffer.tostring(expected.layer.PERSONA1)
        and F.overlay.UNIT2 == buffer.tostring(expected.layer.UNIT2)))
    F.frames(100000)
    emit("LOAD_VERIFIED " .. tostring(has("RANDO_VERIFY ok")))
    -- A tampered block is refused before any commit.
    local tampered = table.clone(value)
    tampered.algorithm = 99
    local refused, why = save.authorize_load(tampered)
    emit("TAMPERED_REFUSED " .. tostring(refused == false) .. " " .. tostring(why ~= nil))
    -- Settings changed after the run locked do not touch it.
    F.settings.r3_affinities = false
    emit("LOCKED " .. tostring(save.snapshot().options == ALL))
    -- New Game+: a carried compendium refuses randomization visibly.
    -- The starter (and the party) may be cached on a fresh game; an ordinary Persona may not.
    F.memory[0x007981cc + 1 * 48] = 1
    save.load(nil, "reset")
    F.frames(100000)
    F.fire("field_enter")
    local starterOnly = not has("RANDO_NEW_GAME_PLUS_REFUSED")
    F.memory[0x007981cc + 7 * 48] = 1
    save.load(nil, "reset")
    F.frames(100000)
    F.fire("field_enter")
    emit("NG_PLUS_REFUSED " .. tostring(starterOnly and has("RANDO_NEW_GAME_PLUS_REFUSED")) .. " " .. tostring(F.status))
    F.memory[0x007981cc + 1 * 48] = nil
    F.memory[0x007981cc + 7 * 48] = nil
    -- The page's seed editor persists at the next frame, never in the callback.
    F.page.adjust("zero", 0)
    emit("SEED_PENDING " .. tostring(F.settings.seed))
    F.frames(10)
    emit("SEED_PERSISTED " .. tostring(F.settings.seed))
    -- R6 live: a floor battle's ordinary enemies take the run's profile for
    -- (floor table, enemy); a formation outside the floor's table stays stock.
    for _, feature in Engine.features do
        F.settings[feature.key] = feature.code == "R6"
    end
    save.load(nil, "reset")
    F.frames(100000)
    F.fire("field_enter")
    F.frames(100000)
    local live = Engine.generate(definition(Engine.unhex(F.settings.seed) :: string, 32, banks), banks)
    local records = live.layer.R6
    local t, enemy = buffer.readu16(records, 0), buffer.readu16(records, 2)
    local formation, story = 0, 0
    for slot = 0, 59 do
        local id = buffer.readu16(banks.ENCOUNT3, t * 0xf8 + 8 + slot * 4)
        for member = 0, 4 do
            if id ~= 0 and buffer.readu16(banks.ENCOUNT0, id * 24 + 8 + member * 2) == enemy then
                formation = id
            end
        end
    end
    for id = 1, buffer.len(banks.ENCOUNT0) // 24 - 1 do
        local held = false
        for slot = 0, 59 do
            held = held or buffer.readu16(banks.ENCOUNT3, t * 0xf8 + 8 + slot * 4) == id
        end
        if not held then
            story = id
            break
        end
    end
    local entries = buffer.create(64)
    buffer.writeu16(entries, 3 * 10, t)
    F.regions[0x02000000], F.memory[0x0076450c] = entries, 0x02000000
    F.regions[0x03000000], F.memory[0x00764508] = banks.ENCOUNT3, 0x03000000
    F.regions[0x04000000], F.memory[0x007644b4] = banks.UNIT0, 0x04000000
    local unit = 0x05000000
    local row = enemy * 60
    local stockHp, stockSt, stockExp = buffer.readu16(banks.UNIT0, row + 4), buffer.readu8(banks.UNIT0, row + 8),
        buffer.readu16(banks.UNIT0, row + 0x1e)
    local function hook(address: number, phase: string, context: any): any
        F.fire(string.format("hook:%08x:%s", address, phase), context)
        return context
    end
    local function battle(id: number): (number, number, number)
        F.memory[unit + 8], F.memory[unit + 0xa] = stockHp, 1
        hook(0x00231630, "before", {args = {id}})
        hook(0x00231af0, "after", {args = {unit, 1, enemy}, result = 1})
        -- The field unit's group was created at its spawn; contact tears the field
        -- down (field_leave) before the battle reads the unit.
        F.fire("field_leave")
        return hook(0x00231f80, "after", {args = {unit}, result = stockHp}).result,
            hook(0x00232b40, "after", {args = {unit, 0}, result = stockSt}).result,
            hook(0x00243950, "after", {args = {unit}, result = stockExp}).result
    end
    hook(0x00161630, "after", {args = {40, 0, 3, 1}, result = formation})
    local hp, st, exp = battle(formation)
    local applied = formation ~= 0 and hp == buffer.readu16(records, 8) and st == buffer.readu8(records, 18)
        and exp == buffer.readu16(records, 14) and F.memory[unit + 8] == hp and (hp ~= stockHp or st ~= stockSt)
    -- The battle's end reports how many reads its profiles scaled (max HP, strength, EXP).
    F.fire("battle_end")
    applied = applied and string.find(F.logs[#F.logs], "RANDO_R6_BATTLE reads=3", 1, true) ~= nil
    -- Freeing the group (datUnitFree on its allocation) ends its units' profiles
    -- and, with none left, the readers.
    hook(0x002319c0, "before", {args = {unit - 0x10}})
    local freed = F.fire(string.format("hook:%08x:after", 0x00231f80), {args = {unit}, result = stockHp}) == 0
    local storyHp, storySt, storyExp = battle(story)
    emit(string.format("R6_LIVE %s %s %s", tostring(applied),
        tostring(story ~= 0 and storyHp == stockHp and storySt == stockSt and storyExp == stockExp), tostring(freed)))
    -- A reset ends the run: every R6 hook is disabled again.
    save.load(nil, "reset")
    emit("R6_HOOKS_OFF " .. tostring(F.fire(string.format("hook:%08x:after", 0x00231f80),
        {args = {unit}, result = 1}) == 0))
    -- R10 live: an offered Shuffle Time's Persona cards are redrawn with the
    -- run's stream for that award from the list each card came from; the award
    -- ordinal travels in the save and continues after a load.
    for _, feature in Engine.features do
        F.settings[feature.key] = feature.code == "R10"
    end
    save.load(nil, "reset")
    F.frames(100000)
    F.fire("field_enter")
    F.frames(100000)
    local metadata = banks.PERSONA0
    F.regions[0x06000000], F.memory[0x007644c4] = metadata, 0x06000000
    local roster = buffer.create(12 * 48)
    F.regions[0x00797f8c] = roster
    local player = 20
    F.calls[0x00104c70] = function(id: number): number
        return player
    end
    -- Two cards of list C (not owned, at most the player's level, inside the window).
    local pool: {number} = {}
    for id = 0, 255 do
        local flags, level = buffer.readu16(metadata, id * 14), buffer.readu8(metadata, id * 14 + 3)
        if bit32.band(flags, 0xdb) == 0 and level <= player - 5 and level >= player - 10 then
            table.insert(pool, id)
        end
    end
    local selection = buffer.create(24)
    buffer.writei16(selection, 0, pool[1])
    buffer.writei16(selection, 2, pool[2])
    buffer.writei32(selection, 8, 2)
    F.regions[0x07000000] = selection
    local function shuffleTime(): string
        buffer.writei16(selection, 0, pool[1])
        buffer.writei16(selection, 2, pool[2])
        F.fire(string.format("hook:%08x:after", 0x0036ee60), {args = {0x07000000, player + 10, 0}, result = 1})
        return string.format("%d,%d", buffer.readi16(selection, 0), buffer.readi16(selection, 2))
    end
    local first = shuffleTime()
    local inPool = 0
    for _, card in string.split(first, ",") do
        if table.find(pool, tonumber(card) :: number) then
            inPool += 1
        end
    end
    local block = save.snapshot()
    local second = shuffleTime()
    -- Reload the block saved between the two awards: the second award repeats.
    save.load(nil, "reset")
    save.load(block, "load")
    F.frames(100000)
    local again = shuffleTime()
    emit(string.format("R10_LIVE %s %s %s", tostring(inPool == 2 and first ~= second),
        tostring(block.shuffle == "0000000000000001"), tostring(again == second)))
    -- R11 live: the starter joins the roster once, at the first field from 04/16
    -- (date index 15), through the game's grant; a load never grants it again.
    for _, feature in Engine.features do
        F.settings[feature.key] = feature.code == "R11"
    end
    save.load(nil, "reset")
    F.frames(100000)
    F.fire("field_enter", {name = "field_enter", task = 0, field = 7, room = 2, date = 10, period = 0})
    F.frames(100000)
    local starterRun = Engine.generate(definition(Engine.unhex(F.settings.seed) :: string, 1024, banks), banks)
    local starter = buffer.readu16(starterRun.layer.R11, 0)
    local granted: {number} = {}
    F.calls[0x0010aa80] = function(id: number): number
        return if table.find(granted, id) then 0 else -1
    end
    F.calls[0x0010b060] = function(id: number): number
        table.insert(granted, id)
        return 0x08000000
    end
    local function field(date: number)
        F.fire("field_enter", {name = "field_enter", task = 0, field = 7, room = 2, date = date, period = 0})
    end
    field(14)
    local early = #granted
    field(15)
    field(16)
    local saved = save.snapshot()
    save.load(nil, "reset")
    save.load(saved, "load")
    F.frames(100000)
    field(17)
    emit(string.format("R11_LIVE %s %s %s", tostring(early == 0), tostring(#granted == 1 and granted[1] == starter),
        tostring(saved.starter == "granted")))
    return
end

-- Corpus: many seeds through generation and validation on the user's banks.
if mode == "corpus" then
    local banks = F.diskBanks(F.args[2])
    if banks == nil then
        emit("CORPUS_SKIPPED no extracted banks")
        return
    end
    F.banks = banks
    local count = tonumber(F.args[3]) or 10
    emit("CORPUS_INPUT " .. hex(Engine.fingerprint(banks, ALL)))
    local print = Engine.fingerprint(banks, ALL)
    for n = 0, count - 1 do
        local def = {algorithm = Engine.algorithm, catalog = Engine.catalog, seed = seedOf(n), options = ALL,
                     fingerprint = print}
        local ok, result = pcall(Engine.generate, def, banks)
        if ok then
            emit(string.format("CORPUS %d %s %s %s", n, hex(result.digest), hex(result.layerHash), counts(result)))
            if result.layer.R11 then
                emit(string.format("STARTER %d %d", n, buffer.readu16(result.layer.R11, 0)))
            end
        else
            emit(string.format("CORPUS_FAILED %d %s", n, tostring(result)))
        end
    end
    -- R4 with triangle fusions, for the first seeds and the stock charts: the
    -- closure reaches at least what pair fusions alone reach, and every Persona
    -- it adds is the exact result of the pair or triangle fusion it recorded
    -- (fclFusePair / fclFuseTriple, ingredients excluded).
    local model = Engine.fusionModel(banks)
    local function triangleCheck(tag: string, charts: {[string]: buffer})
        local witness: {[number]: {number}} = {}
        local pairs = Engine.fusionReach(model, charts.FUSION0)
        local full = Engine.fusionReach(model, charts.FUSION0, charts.FUSION2, charts.FUSION3, witness)
        local np, nf, ok, triples = 0, 0, true, 0
        for id, at in pairs do
            np += 1
            ok = ok and full[id] ~= nil and full[id] <= at
        end
        for id in full do
            nf += 1
            local from = witness[id]
            if from and #from == 3 then
                triples += 1
                ok = ok and Engine.fuseTriple(model, charts.FUSION2, charts.FUSION3, from[1], from[2], from[3]) == id
            elseif from then
                ok = ok and Engine.fusePair(model, charts.FUSION0, from[1], from[2]) == id
            end
        end
        emit(string.format("TRIANGLE %s %s pair=%d full=%d by_triangle=%d", tag, tostring(ok), np, nf, triples))
    end
    triangleCheck("stock", banks)
    for n = 0, math.min(count, 4) - 1 do
        local result = Engine.generate({algorithm = Engine.algorithm, catalog = Engine.catalog, seed = seedOf(n),
                                        options = ALL, fingerprint = print}, banks)
        triangleCheck(tostring(n), setmetatable(table.clone(result.layer), {__index = banks}) :: any)
    end
    -- Every feature alone, for the first seeds: it generates and validates on its
    -- own, and its banks are exactly the ones the all-features run produced.
    for n = 0, math.min(count, 4) - 1 do
        local all = Engine.generate({algorithm = Engine.algorithm, catalog = Engine.catalog, seed = seedOf(n),
                                     options = ALL, fingerprint = print}, banks)
        local noPlacement = bit32.band(ALL, bit32.bnot(16))
        local withoutPlacement = Engine.generate({algorithm = Engine.algorithm, catalog = Engine.catalog,
            seed = seedOf(n), options = noPlacement, fingerprint = Engine.fingerprint(banks, noPlacement)}, banks)
        local noProfiles = bit32.band(ALL, bit32.bnot(32))
        local withoutProfiles = Engine.generate({algorithm = Engine.algorithm, catalog = Engine.catalog,
            seed = seedOf(n), options = noProfiles, fingerprint = Engine.fingerprint(banks, noProfiles)}, banks)
        for _, feature in Engine.features do
            local options = bit32.lshift(1, feature.id - 1)
            local ok, alone = pcall(Engine.generate, {algorithm = Engine.algorithm, catalog = Engine.catalog,
                seed = seedOf(n), options = options, fingerprint = Engine.fingerprint(banks, options)}, banks)
            local same = ok
            if ok then
                for key, bytes in alone.layer do
                    if key == "R6" then
                        -- R6 profiles follow R5's placement: alone they match the run without R5.
                        same = same and buffer.tostring(bytes) == buffer.tostring(withoutPlacement.layer[key])
                    elseif key == "ENCOUNT3" then
                        -- R5 alone uses the R6-off budget rule: it matches the run without R6.
                        same = same and buffer.tostring(bytes) == buffer.tostring(withoutProfiles.layer[key])
                    elseif key ~= "PERSONA1" then
                        same = same and buffer.tostring(bytes) == buffer.tostring(all.layer[key])
                    else
                        for id = 0, 255 do
                            local owned = if feature.code == "R1" then buffer.readstring(bytes, id * 70 + 6, 64)
                                else buffer.readstring(bytes, id * 70, 6)
                            local reference = if feature.code == "R1" then buffer.readstring(all.layer.PERSONA1, id * 70 + 6, 64)
                                else buffer.readstring(all.layer.PERSONA1, id * 70, 6)
                            same = same and owned == reference
                        end
                    end
                end
            end
            emit(string.format("ALONE %d %s %s %s", n, feature.code, tostring(same), if ok then "" else tostring(alone)))
            if ok and feature.code == "R5" then
                emit(string.format("ALONE_R5 %d %d", alone.counts.R5.changed, alone.counts.R5.eligible))
            end
        end
    end
    -- The game's budget: one 4096-unit callback per frame; no stretch between
    -- two yields may come near it, whatever the input.
    local function paced(work: () -> any): (number, number, number, number)
        F.budgetMode, F.maxUsed, F.maxGap, F.lastPace = "game", 0, 0, 0
        local thread = coroutine.create(work)
        local frames, worst, start = 0, 0, safepoints()
        while coroutine.status(thread) ~= "dead" do
            frames += 1
            F.resumeAt = safepoints()
            local ok, failure = coroutine.resume(thread)
            assert(ok, failure)
            worst = math.max(worst, safepoints() - F.resumeAt)
        end
        F.budgetMode = "large"
        return frames, worst, safepoints() - start, F.maxGap
    end
    local frames, worst, total, gap = paced(function() return Engine.fingerprint(banks, ALL) end)
    emit(string.format("BUDGET fingerprint frames=%d worst=%d total=%d gap=%d", frames, worst, total, gap))
    -- A fresh Stock table: the engine's per-bank-set caches start cold, as in the game.
    local cold = table.clone(banks)
    local base = heap()
    F.maxHeap = base
    frames, worst, total, gap = paced(function()
        return Engine.generate({algorithm = Engine.algorithm, catalog = Engine.catalog, seed = seedOf(0),
                                options = ALL, fingerprint = print}, cold)
    end)
    emit(string.format("BUDGET generate frames=%d worst=%d total=%d gap=%d", frames, worst, total, gap))
    -- Allocated bytes (live and uncollected) above the start, against the game's 8 MiB per mod.
    emit(string.format("BUDGET heap growth=%d", F.maxHeap - base))
    -- One spoiler sample for the report.
    local def = {algorithm = Engine.algorithm, catalog = Engine.catalog, seed = seedOf(0), options = ALL, fingerprint = print}
    local result = Engine.generate(def, banks)
    emit("SPOILER_LINES " .. tostring(#string.split(result.spoiler, "\n")))
    if F.args[4] then
        emit("SPOILER_BEGIN")
        for _, line in string.split(result.spoiler, "\n") do
            emit(line)
        end
        emit("SPOILER_END")
    end
    return
end

-- Expected live rows for the in-game witness: the run the game should apply
-- for (seed, options) on this disc, and the generated rows of chosen personas.
if mode == "expect" then
    local banks = F.diskBanks(F.args[2])
    assert(banks, "no extracted banks")
    local seed = Engine.unhex(F.args[3]) :: string
    local options = tonumber(F.args[4]) :: number
    local def = {algorithm = Engine.algorithm, catalog = Engine.catalog, seed = seed, options = options,
                 fingerprint = Engine.fingerprint(banks, options)}
    local result = Engine.generate(def, banks)
    emit(string.format("EXPECT_RUN %s %s %s", Engine.runCode(def), hex(result.digest), hex(result.layerHash)))
    local strides = {PERSONA0 = 14, PERSONA1 = 70, UNIT2 = 32}
    for index = 5, #F.args do
        local id = tonumber(F.args[index]) :: number
        for _, key in {"PERSONA0", "PERSONA1", "UNIT2"} do
            local bank = result.layer[key] or banks[key]
            emit(string.format("EXPECT_ROW %s %d %s", key, id, hex(buffer.readstring(bank, id * strides[key], strides[key]))))
            emit(string.format("STOCK_ROW %s %d %s", key, id, hex(buffer.readstring(banks[key], id * strides[key], strides[key]))))
        end
    end
    -- The pair evaluator over the generated and the stock pair chart.
    local model = Engine.fusionModel(banks)
    local fusions = {{46, 26}, {14, 7}, {172, 61}, {33, 44}, {6, 54}, {1, 14}, {67, 146}, {110, 152}}
    for _, pair in fusions do
        emit(string.format("EXPECT_FUSE %d %d %d %d", pair[1], pair[2],
            Engine.fusePair(model, result.layer.FUSION0 or banks.FUSION0, pair[1], pair[2]),
            Engine.fusePair(model, banks.FUSION0, pair[1], pair[2])))
    end
    -- The triangle evaluator over the generated and the stock triangle charts
    -- (the probe's TRIPLES: equal-level ties included).
    local TRIPLES = {{27, 59, 128}, {27, 102, 156}, {29, 44, 114}, {29, 54, 167}, {29, 67, 172}, {29, 74, 148}, {29, 110, 152}, {29, 114, 128}}
    local first, second = result.layer.FUSION2 or banks.FUSION2, result.layer.FUSION3 or banks.FUSION3
    for _, triple in TRIPLES do
        emit(string.format("EXPECT_FUSE3 %d %d %d %d %d", triple[1], triple[2], triple[3],
            Engine.fuseTriple(model, first, second, triple[1], triple[2], triple[3]),
            Engine.fuseTriple(model, banks.FUSION2, banks.FUSION3, triple[1], triple[2], triple[3])))
    end
    -- The generated floor tables and enemy profiles, for the dungeon witness.
    emit("EXPECT_ENCOUNT3 " .. hex(buffer.tostring(result.layer.ENCOUNT3 or banks.ENCOUNT3)))
    emit("STOCK_ENCOUNT3 " .. hex(buffer.tostring(banks.ENCOUNT3)))
    emit("STOCK_ENCOUNT0 " .. hex(buffer.tostring(banks.ENCOUNT0)))
    emit("EXPECT_R6 " .. (if result.layer.R6 then hex(buffer.tostring(result.layer.R6)) else ""))
    emit("EXPECT_R11 " .. (if result.layer.R11 then tostring(buffer.readu16(result.layer.R11, 0)) else "none"))
    emit("EXPECT_ITEMTBL " .. hex(buffer.tostring(result.layer.ITEMTBL or banks.ITEMS)))
    emit("EXPECT_UNIT0 " .. hex(buffer.tostring(result.layer.UNIT0 or banks.UNIT0)))
    emit("STOCK_UNIT0 " .. hex(buffer.tostring(banks.UNIT0)))
    emit("STOCK_ITEMTBL " .. hex(buffer.tostring(banks.ITEMS)))
    -- The banks the field and battle code read live.
    for _, key in {"ENCOUNT3", "ENCOUNT4", "UNIT0"} do
        emit(string.format("EXPECT_BANK %s %s %s", key, hex(sha256(result.layer[key] or banks[key])),
            hex(sha256(banks[key]))))
    end
    emit("SPOILER_BEGIN")
    for _, line in string.split(result.spoiler, "\n") do
        emit(line)
    end
    emit("SPOILER_END")
    return
end

error("unknown mode " .. tostring(mode))
