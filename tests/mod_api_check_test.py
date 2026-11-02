"""Check actual public definitions with the pinned native Luau Analysis CLI."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

separator = sys.argv.index("--driver-end")
command = sys.argv[1:separator]
definitions = Path(sys.argv[separator + 1])
native_ui_fixture = Path(sys.argv[separator + 2])
example_fixture = Path(sys.argv[separator + 3])

positive = """--!strict
local scratch: P4GuestBuffer = p4.mem.alloc(64)
p4.mem.write("u32", scratch, 4294967295)
local n: number = p4.mem.read("u32", scratch)
local wide: buffer = p4.u64(4294967295, 1)
p4.mem.write("u64", scratch.address + 8, wide)
local high: number, low: number = p4.split64(p4.mem.read("u64", scratch.address + 8))
local bytes: buffer = p4.files.read("DATA/ITEMTBL.BIN", 4096)
local size: number = buffer.len(bytes)
local layout: P4Layout<{value: number}> = p4.mem.layout<<{value: number}>>({
    name = "Record", size = 4, fields = {value = {offset = 0, type = "u32"}}
})
local view = layout:view(scratch)
view.value = n
local address: number = view.address
local hook: P4Subscription = p4.hook(0x100100, "II", "before", function(c)
    c.args[1] = 7
end)
hook:disable()
hook:remove()
p4.events.on("frame_start", function(e)
    local frame: number = e.frame
    p4.log(tostring(frame))
end)
p4.events.on("field_enter", function(e)
    local field: number = e.field
    local room: number = e.room
    p4.log(tostring(field + room))
end)
local down: boolean, pressed: boolean, released: boolean = p4.input.action("confirm")
local x: number, y: number, rx: number, ry: number = p4.input.sticks()
local ok, message = pcall(function() return getfenv(1) end)
local co = coroutine.create(function() coroutine.yield(1) end)
coroutine.resume(co)
local detail: P4UIPage = p4.ui.detail({
    key = "about", title = "About", detail = function() return "Native RAW body" end
})
local page: P4UIPage = p4.ui.page({
    key = "options", title = "Options", count = function() return 1 end,
    row = function(index: number): P4UIRow
        return {key = "about", label = "About", kind = "link", target = detail.key}
    end,
    find = function(key: string): number? return if key == "about" then 1 else nil end,
})
local accepted: boolean = page:open()
local refreshed: boolean = page:refresh()
local active: string? = p4.ui.active()
local save_value: P4SaveValue = {count = 1, nested = {enabled = true}}
p4.save.register({
    schema = 1, max_bytes = 1024,
    snapshot = function(): P4SaveValue return save_value end,
    load = function(value: P4SaveValue) save_value = value end,
})
scratch:release()
"""
negative = [
    'local n: string = p4.mem.read("u32", 0x100000)',
    'p4.mem.write("u64", 0x100000, 1)',
    'local s: string = p4.files.read("DATA/X.BIN")',
    'p4.events.on("field_enter", function(e) local s: string = e.field end)',
    'p4.hook(0x100100, "II", "before", function(c) c.args[1] = "not-scalar" end)',
    'p4.input.action("made-up-action")',
    'p4.native_loadlib("unsafe.dll")',
    'return os.clock()',
    'return debug.info(1, "n")',
    'return vector.create(1, 2, 3)',
    'return integer.create(1)',
    'return io.open("host-file")',
    'return require("native-library")',
    'return loadstring("untrusted bytecode")',
    'p4.ui.detail({key="bad",title="Bad",detail=function() return 7 end})',
    'p4.save.register({schema=1,max_bytes=128,snapshot=function() return buffer.create(2) end,load=function(v) end})',
    'p4.save.register({schema=1,max_bytes=\"large\",snapshot=function() return nil end,load=function(v) end})',
    'p4.ui.claim_service("content-packs")',
]

with tempfile.TemporaryDirectory(prefix="p4-mod-api-check-") as directory:
    source = Path(directory) / "mod.luau"

    def check(text, expected):
        source.write_text(text, encoding="utf-8")
        result = subprocess.run([*command, str(definitions), str(source)], capture_output=True, text=True)
        report = json.loads(result.stdout)
        assert result.returncode == expected, (result.returncode, report, result.stderr, text)
        assert report["ok"] == (expected == 0), report
        if expected:
            assert report["diagnostics"], report
            assert report["diagnostics"][0]["line"] >= 1, report
            assert report["diagnostics"][0]["column"] >= 1, report

    check(positive, 0)
    check(native_ui_fixture.read_text(encoding="utf-8"), 0)
    check(example_fixture.read_text(encoding="utf-8"), 0)
    # Every overload retains its literal-name/payload correlation and is
    # usable as a first-class function, despite Analysis' overload complexity.
    events = [
        ("frame_start", "P4FrameEvent", "frame"),
        ("field_enter", "P4FieldEnterEvent", "field"),
        ("field_leave", "P4FieldLeaveEvent", "field"),
        ("battle_start", "P4BattleStartEvent", "battle"),
        ("battle_end", "P4BattleEndEvent", "outcome"),
        ("menu_open", "P4MenuOpenEvent", "kind"),
        ("menu_close", "P4MenuCloseEvent", "kind"),
        ("calendar_advance", "P4CalendarEvent", "date"),
        ("save", "P4SaveEvent", "size"),
        ("load", "P4LoadEvent", "size"),
    ]
    valid_events = ["--!strict"]
    for index, (name, event_type, field) in enumerate(events):
        valid_events.append(f'p4.events.on("{name}", function(e) '
                            f'local n: number = e.{field}; local tag: "{name}" = e.name end)')
        valid_events.append(f'local f{index}: ("{name}", ({event_type}) -> ()) -> P4Subscription = p4.events.on')
        check(f'--!strict\np4.events.on("{name}", function(e) local bad: string = e.{field} end)\n', 1)
    check("\n".join(valid_events) + "\n", 0)
    check('--!strict\np4.events.on("not_an_event", function(e) end)\n', 1)
    check('--!strict\nlocal bogus: number = p4.events.on\n', 1)
    check('--!strict\nlocal spec: P4LayoutSpec = {name="bad",size=4,fields={x={offset=0,type="bogus"}}}\n', 1)
    for text in negative:
        check("--!strict\n" + text + "\n", 1)
print("mod_api_check: public generic types, scalar/buffer/events, native UI fixture, normal environments/coroutines and unavailable-global diagnostics")
