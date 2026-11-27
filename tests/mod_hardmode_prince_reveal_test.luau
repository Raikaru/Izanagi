-- Fictional affinity rows exercise the shipped summon hook, without game data.
-- Run after mod_hardmode_fixture.luau and the hard-mode main.luau.
local F = FIXTURE
local save = F.save
local run = {mode = "harder", stats = true, ai = true, rewards = true, technical = true, bosses = true,
    revision = 7, adopted = false, physsplit = "1.0.0", epoch = 1}
save.load(nil, "reset")
assert(save.authorize_load(run))
save.load(run, "load")

local function reveal(row: any): {number}
    F.rows[269] = row
    F.callLog = {}
    F.hookFns[0x00231630]({args = {0x202}})
    F.calls[0x0029d050] = 0x1000
    F.calls[0x00231f80] = 2000
    F.memMap[0x1030] = 0x2000
    F.memMap[0x20A2] = 1
    F.memMap[0x2A64] = 0x3000
    F.memMap[0x3002] = 258
    F.memMap[0x3008] = 800
    F.memMap[0x106E] = 356
    F.hookFns[0x001df310]({args = {}})
    F.hookFns[0x001ea220]({args = {}})
    local bits = {}
    for _, call in F.callLog do
        if call.address == 0x0010f3d0 then
            assert(call.args[1] == 269)
            table.insert(bits, call.args[2])
        end
    end
    return bits
end

local function fictional(element: string): any
    local affinities = {}
    for _, name in {"physical", "fire", "ice", "electric", "wind", "almighty", "light", "dark"} do
        affinities[name] = if name == element then "weak" else "normal"
    end
    return {affinities = affinities}
end
local fire, wind = reveal(fictional("fire")), reveal(fictional("wind"))
assert(#fire == 1 and fire[1] == 1 and #wind == 1 and wind[1] == 4, "runtime weakness not revealed")
local multiple = fictional("fire")
multiple.affinities.wind = "weak"
local bits = reveal(multiple)
assert(#bits == 2 and bits[1] == 1 and bits[2] == 4, "multiple weaknesses not revealed")
assert(#reveal(fictional("none")) == 0, "non-weak row revealed a weakness")
assert(#reveal(nil) == 0, "failed lookup revealed a weakness")
assert(string.find(table.concat(F.logs, "\n"), "HARDMODE_REVEAL disabled: cannot read Prince affinities", 1, true),
    "failed lookup was not logged")
local malformed = fictional("fire")
malformed.affinities.dark = nil
assert(#reveal(malformed) == 0, "partial row revealed a weakness")
emit("mod_hardmode_prince_reveal: fictional fire=1 wind=4 multiple=1,4 none=0 missing=closed malformed=closed")
