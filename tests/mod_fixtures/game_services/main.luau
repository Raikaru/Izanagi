--!strict
-- Authored mod-test input for the API 1.4 game services (mod_practice_game).
-- From the first ready field it runs one scripted scenario at frame_start and
-- logs every result: clock, HUD rows, HUD hides, the camera lease, a warp and
-- an encounter launch with its return. Not shipped; not a retail asset.
local frame = 0
local phase = "ready"
local waitUntil = 0
local lease: P4CameraLease? = nil
local ENCOUNTER = 20

local function log(text: string) p4.log("SERVICES f=" .. tostring(frame) .. " " .. text) end
local function scene(): string
    local s = p4.scene.state()
    return string.format("ctx=%s ready=%s field=%d room=%d entry=%s pending=%s", s.context, tostring(s.ready),
        s.field, s.room, tostring(s.entry), tostring(s.pending))
end
log("init " .. scene())

-- The harness boots a field, then loads the card's save: the scenario starts
-- in the first ready field after that load.
local loaded = false
p4.save.register({
    schema = 1, max_bytes = 16,
    snapshot = function(): P4SaveValue return nil end,
    load = function(value: P4SaveValue, origin: "load" | "reset") if origin == "load" then loaded = true end end,
})
p4.events.on("field_enter", function(e: P4FieldEnterEvent) log("EVENT field_enter " .. e.field .. ":" .. e.room) end)
p4.events.on("battle_end", function(e: P4BattleEndEvent) log("EVENT battle_end outcome=" .. e.outcome) end)

local function go(next: string, delay: number)
    phase = next
    waitUntil = frame + delay
end

local steps: { [string]: () -> () } = {}
steps.ready = function()
    if loaded and p4.scene.state().ready then log("READY " .. scene()); go("clock", 30) end
end
steps.clock = function()
    log(string.format("CLOCK updates=%d retail=%d", p4.clock.updates(), p4.clock.retail()))
    log("HUD rows=" .. tostring(p4.ui.status({ "SERVICES HUD ONE", "SERVICES HUD TWO" }, "hud")))
    go("clock2", 60)
end
steps.clock2 = function()
    log(string.format("CLOCK updates=%d wall=%s", p4.clock.updates(), tostring(p4.clock.wall() >= 0)))
    go("hide", 10)
end
steps.hide = function()
    for _, group in p4.hud.groups() do
        local ok, why = p4.hud.hide(group, true)
        log("HIDE " .. group .. " ok=" .. tostring(ok) .. " why=" .. tostring(why) .. " hidden=" .. tostring(p4.hud.hidden(group)))
    end
    go("unhide", 90)
end
steps.unhide = function()
    for _, group in p4.hud.groups() do p4.hud.hide(group, false) end
    log("UNHIDE hidden=" .. tostring(p4.hud.hidden("calendar")))
    go("camera", 30)
end
steps.camera = function()
    local why: string?, owner: string?
    lease, why, owner = p4.camera.acquire("witness camera")
    local second, why2, owner2 = p4.camera.acquire("witness second")
    log("CAMERA acquire=" .. tostring(lease ~= nil) .. " why=" .. tostring(why) .. " second=" .. tostring(second ~= nil) ..
        " why2=" .. tostring(why2) .. " owner2=" .. tostring(owner2))
    go("camera_set", 3)
end
steps.camera_set = function()
    local held = lease
    local pose = held and held:get()
    if not held or not pose then log("CAMERA no pose"); go("warp", 0); return end
    log(string.format("CAMERA pose y=%.2f pitch=%.2f fov=%.2f", pose.y, pose.pitch, pose.fov))
    log("CAMERA set=" .. tostring(held:set({ y = pose.y + 250, pitch = pose.pitch - 25, fov = math.min(pose.fov + 20, 120) })) ..
        " bad=" .. tostring(held:set({ fov = 200 })) .. " input=" .. tostring(held:input(true)))
    go("camera_release", 90)
end
steps.camera_release = function()
    local held = lease
    if held then
        local active = held:active()
        local pose = held:get()
        held:release()
        local after, why = held:active()
        log(string.format("CAMERA active=%s y=%.2f released=%s why=%s owner=%s", tostring(active),
            pose and pose.y or -1, tostring(not after), tostring(why), tostring(p4.camera.owner())))
    end
    go("warp", 30)
end
steps.warp = function()
    local s = p4.scene.state()
    log("WARP bad=" .. tostring((p4.scene.warp({ field = 999, room = 999 }))))
    local target = if s.field == 7 and s.room == 1 then 2 else 1
    local ok, why = p4.scene.warp({ field = 7, room = target, entry = 0 })
    log("WARP 7:" .. target .. " ok=" .. tostring(ok) .. " why=" .. tostring(why))
    go(if ok then "warp_wait" else "encounter", 0)
end
steps.warp_wait = function()
    local state, detail = p4.scene.status()
    if state == "done" or state == "refused" then log("WARP_DONE " .. state .. " " .. detail .. " " .. scene()); go("settle", 0) end
end
steps.settle = function()
    if p4.scene.state().ready then log("SETTLED " .. scene()); go("encounter", 30) end
end
steps.encounter = function()
    local info = p4.encounter.info(ENCOUNTER)
    log("ENCOUNTER count=" .. p4.encounter.count() .. " info=" .. tostring(info ~= nil) ..
        " missing=" .. tostring(p4.encounter.info(p4.encounter.count()) == nil))
    local ok, why = p4.encounter.launch(ENCOUNTER)
    log("ENCOUNTER launch ok=" .. tostring(ok) .. " why=" .. tostring(why))
    go(if ok then "encounter_wait" else "done", 0)
end
steps.encounter_wait = function()
    local state, detail, outcome = p4.scene.status()
    if state == "done" or state == "refused" then
        log("ENCOUNTER_DONE " .. state .. " " .. detail .. " outcome=" .. tostring(outcome) .. " " .. scene())
        go("returned", 0)
    end
end
steps.returned = function()
    if p4.scene.state().ready then log("RETURNED " .. scene()); go("done", 0) end
end
steps.done = function()
    log("DONE")
    phase = "idle"
end
steps.idle = function() end

p4.events.on("frame_start", function(e: P4FrameEvent)
    frame = e.frame
    if frame >= waitUntil then steps[phase]() end
end)
