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
bundled_mods = [Path(path) for path in sys.argv[separator + 4:]]
# The shipped definitions are p4.d.luau plus one fragment per capability key
# (include/p4pc/p4.d/<key>.luau, tools/p4_capabilities.py). Each fragment's
# --!example must type-check and each of its --!refuse lines must not.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import p4_capabilities  # noqa: E402
fragments = p4_capabilities.load(definitions.parent / "p4.d")
composed_directory = tempfile.TemporaryDirectory(prefix="p4-mod-api-definitions-")
composed = Path(composed_directory.name) / "p4.d.luau"
composed.write_bytes(p4_capabilities.compose(definitions.read_text(encoding="utf-8"), fragments).encode("utf-8"))
definitions = composed

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
local remaining: number = p4.budget()
local digest: string = p4.sha256(buffer.create(4))
local written: boolean, writeWhy: string? = p4.artifacts.write("runs/r1/spoiler.txt", "text")
local back: buffer? = p4.artifacts.read("runs/r1/spoiler.txt", 1024)
local folder: string? = p4.artifacts.folder()
local stock: buffer = p4.tables.stock("PERSONA1")
local banks: {P4TableBank} = p4.tables.banks()
local loaded: number? = p4.tables.bank("UNIT2").bytes
local overlaid: boolean, refused: string? = p4.tables.overlay({PERSONA1 = stock})
local mine: {[string]: boolean} = p4.tables.active()
p4.tables.clear()
local active: string? = p4.ui.active()
local save_value: P4SaveValue = {count = 1, nested = {enabled = true}}
p4.save.register({
    schema = 1, max_bytes = 1024,
    snapshot = function(): P4SaveValue return save_value end,
    load = function(value: P4SaveValue, origin: "load" | "reset") save_value = value end,
    authorize_save = function() return true end,
    authorize_load = function(value: P4SaveValue) return false, "locked" end,
    published = function(value: P4SaveValue) save_value = value end,
})
local shown: boolean = p4.ui.status({"CHALLENGE 2 RULES", "CAP 15"})
p4.ui.status("ONE ROW", "hud")
p4.ui.status(nil)
local gate: P4Subscription = p4.authorize.on("fusion", function(context)
    local kind: number = context.kind
    return kind > 1, "pair and triangle only"
end)
gate:disable()
p4.authorize.on("encounter", function(context) return context.encounter ~= 0 end)
p4.events.on("exit", function(e) local tag: "exit" = e.name end)
local lineup: P4Subscription = p4.battle.formation(function(context): {number}?
    local first: number = context.party[1]
    return if first == 1 then {2} else nil
end)
lineup:remove()
local yosuke: P4PartyRecord? = p4.party.record(2)
if yosuke then
    local queued: boolean, why: string? = p4.party.set(2, {persona = yosuke.persona, level = yosuke.level,
        exp = yosuke.exp, stats = yosuke.stats, skills = yosuke.skills})
end
local session: number? = p4.lifecycle.session()
if session then
    local tagged: boolean = p4.lifecycle.tag("R1:B1")
    local crashed: boolean?, tag: string? = p4.lifecycle.crashed(session)
    local consumed: boolean = p4.lifecycle.consume(session)
end
local setting = p4.settings.get("feature.enabled")
local stored: boolean, why: string? = p4.settings.set("run.count", 3)
p4.settings.set("feature.enabled", setting == true)
local seconds: number = p4.clock.wall()
local updates: number = p4.clock.updates() + p4.clock.retail()
local scene: P4SceneState = p4.scene.state()
if scene.ready and scene.context == "field" then
    local queued: boolean, refusal: string? = p4.scene.warp({field = scene.field, room = scene.room, entry = scene.entry})
end
local request: P4RequestState, detail: string, outcome: number? = p4.scene.status()
local encounter: P4EncounterInfo? = p4.encounter.info(p4.encounter.count() - 1)
if encounter then p4.encounter.launch(encounter.id) end
local lease: P4CameraLease?, why_not: string?, holder: string? = p4.camera.acquire("free camera")
if lease then
    lease:input(true)
    local pose: P4CameraPose? = lease:get()
    lease:set({y = 100, pitch = -20, fov = 50})
    local active: boolean, released_why: string? = lease:active()
    lease:release()
end
local owner: string? = p4.camera.owner()
for _, group in p4.hud.groups() do p4.hud.hide(group, p4.hud.hidden(group) == false) end
local passed: boolean, pass_why: string? = p4.calendar.pass()
local paced: boolean, pace_why: string? = p4.pace.set(2)
local requested: number, effective: number, held: string? = p4.pace.state()
local nested: P4UIPage = p4.ui.page({
    key = "nested", title = "Nested", listed = false, count = function() return 0 end,
    row = function(index: number): P4UIRow return {key = "x", label = "X", kind = "info"} end,
    find = function(key: string): number? return nil end,
})
if p4.elements.active() then
    local element = p4.elements.of_skill(0x79)
    local label: string? = element and element.name
    local weapon: boolean = element ~= nil and element.weapon == true
    local held = p4.elements.of_weapon(39)
    local revision: number = p4.elements.revision()
end
local music: number = p4.audio.volume("music")
local applied: boolean = p4.audio.set_volume("voice", 60)
local tagged: boolean = p4.audio.tag_adxt(0x7188C0, "music")
local battle = p4.battle.menu()
if battle and battle.panel == "commands" then
    local chosen: boolean, driver: string? = p4.battle.choose("item")
elseif battle and battle.skills then
    p4.battle.point(#battle.skills)
end
local units = p4.battle.units()
if units then p4.battle.aim(units.enemies[1].unit) end
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
    'p4.tables.overlay("PERSONA1")',
    'local n: number = p4.sha256("text")',
    'local ok: number = p4.artifacts.write("a.txt", "x")',
    'p4.ui.claim_service("content-packs")',
    'p4.settings.set("key", buffer.create(1))',
    'local n: number = p4.settings.set("key", true)',
    'p4.authorize.on("made-up-operation", function(c) return true end)',
    'p4.authorize.on("fusion", function(c) local s: string = c.kind; return true end)',
    'p4.ui.status(7)',
    'p4.ui.status("x", "sidebar")',
    'p4.battle.formation(function(c) return "yosuke" end)',
    'p4.lifecycle.tag(5)',
    'p4.hud.hide("made-up-group", true)',
    'p4.scene.warp({room = 1})',
    'p4.camera.acquire(7)',
    'return p4.clock.epoch()',
    'local pose: P4CameraPose = p4.camera.acquire("x")',
    'local s: string = p4.elements.of_skill(1)',
    'p4.elements.of_weapon("Golf Club")',
    'p4.audio.set_volume("ambience", 50)',
    'local s: string = p4.audio.volume("music")',
    'p4.calendar.pass("evening")',
    'local n: number = p4.calendar.pass()',
    'p4.battle.choose("rush")',
    'local n: number = p4.battle.point(1)',
    'p4.pace.set("fast")',
    'local a: number, b: number, held: number = p4.pace.state()',
    'p4.party.set(2, 5)',
]

with tempfile.TemporaryDirectory(prefix="p4-mod-api-check-") as directory:
    cases = []

    def check(text, expected):
        cases.append((text, expected))

    check(positive, 0)
    check(native_ui_fixture.read_text(encoding="utf-8"), 0)
    check(example_fixture.read_text(encoding="utf-8"), 0)
    for bundled in bundled_mods:
        check(bundled.read_text(encoding="utf-8"), 0)
    for fragment in fragments:
        if fragment.examples:
            check("--!strict\n" + "\n".join(fragment.examples) + "\n", 0)
        for text in fragment.refusals:
            check("--!strict\n" + text + "\n", 1)
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

    # One analyzer process for every case (each launch is a process start, a
    # slow one under Wine), plus the single-source form other callers use.
    paths = []
    for index, (text, _) in enumerate(cases):
        path = Path(directory) / f"case{index:03}.luau"
        path.write_text(text, encoding="utf-8")
        paths.append(path)
    single = subprocess.run([*command, str(definitions), str(paths[0])], capture_output=True, text=True)
    assert single.returncode == 0 and single.stdout.count("\n") == 1, (single.returncode, single.stdout, single.stderr)
    assert json.loads(single.stdout) == {"diagnostics": [], "ok": True}, single.stdout
    result = subprocess.run([*command, str(definitions), *map(str, paths)], capture_output=True, text=True)
    reports = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(reports) == len(cases), (len(reports), len(cases), result.stderr)
    assert result.returncode == max(expected for _, expected in cases), (result.returncode, result.stderr)
    for path, (text, expected), report in zip(paths, cases, reports):
        assert report["ok"] == (expected == 0), (report, text)
        assert all(d["file"] == str(path) for d in report["diagnostics"]), (report, text)
        if expected:
            assert report["diagnostics"], (report, text)
            assert report["diagnostics"][0]["line"] >= 1, report
            assert report["diagnostics"][0]["column"] >= 1, report
print("mod_api_check: public generic types, scalar/buffer/events, native UI fixture, normal environments/coroutines and unavailable-global diagnostics")
