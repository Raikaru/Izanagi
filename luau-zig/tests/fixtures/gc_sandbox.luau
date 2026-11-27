-- Exercise collector activity through the library surface available to mods.
assert(collectgarbage == nil)
assert(type(gcinfo()) == "number" and gcinfo() >= 0)
local live = { marker = "live" }
local weak = setmetatable({ live }, { __mode = "v" })
for round = 1, 200 do
    local items = {}
    for i = 1, 200 do
        items[i] = { round, i, string.rep("x", 48) }
    end
    assert(weak[1] == live and live.marker == "live")
    assert(items[200][1] == round and items[200][2] == 200)
end
assert(weak[1] == live)
emit("gc-sandbox:live")
