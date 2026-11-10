--!strict
-- files/DATA/MOD_EXAMPLE.TXT is published through the same VFS as the disc:
-- p4.files.read returns the winning bytes, here the mod's own file.
local note = (string.gsub(buffer.tostring(p4.files.read("DATA/MOD_EXAMPLE.TXT", 256)), "%s+$", ""))
p4.log("EXAMPLE_FILE " .. note)

-- The scratch layout backs the live hook. A plain Lua number is snapshotted
-- into the mod sidecar and restored into scratch after the game loads a save.
local scratch = p4.mem.alloc(8)
local layout = p4.mem.layout<<{calls: number, last: number}>>({
    name = "ExampleTimeSlotCounter", size = 8,
    fields = {calls = {offset = 0, type = "u32"}, last = {offset = 4, type = "u32"}},
})
local state = layout:view(scratch)
local count = 0
p4.save.register({
    schema = 1,
    max_bytes = 32,
    snapshot = function()
        return count
    end,
    load = function(value)
        if value == nil then
            count = 0
        else
            assert(type(value) == "number" and value >= 0 and value <= 4294967295 and value % 1 == 0,
                "invalid saved example counter")
            count = value
        end
        state.calls = count
        p4.log("EXAMPLE_RESTORED count=" .. tostring(count))
    end,
})
-- [setting counting] in mod.ini is the native Config row; a live setting is
-- read at each call, so turning it off in the MODS hub takes effect at once.
p4.hook(0x001062f0, "vB", "after", function(context)
    if p4.settings.get("counting") ~= true then
        p4.log("EXAMPLE_PAUSED count=" .. tostring(count) .. " value=" .. tostring(context.args[1]))
        return
    end
    local nextCount = state.calls + 1
    state.calls = nextCount
    count = state.calls
    state.last = context.args[1] :: number
    p4.log("EXAMPLE_TIME_SLOT count=" .. tostring(state.calls) .. " value=" .. tostring(state.last))
end)
p4.log("EXAMPLE_READY counter=0 counting=" .. (if p4.settings.get("counting") == true then "on" else "off"))
