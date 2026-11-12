--!strict
-- Authored mod-test input for the real native Config/hub/rendering path.
-- No force-open/menu gate bypass; the native scripted lane navigates the hub.
local flag = false
local adjustments = 0
local options: P4UIPage? = nil
local detail = p4.ui.detail({
    key = "details",
    title = "Native UI detail",
    detail = function(): string
        return "Native font wrapping and paging\n" .. string.rep("Authored fixture text, not a retail asset.\n", 180)
    end,
    opened = function() p4.log("UI_OPEN details") end,
    closed = function() p4.log("UI_CLOSE details") end,
})
options = p4.ui.page({
    key = "options",
    title = "Native UI controls",
    count = function(): number return 5 end,
    row = function(index: number): P4UIRow
        if index == 1 then
            return {key = "flag", label = "Fixture flag", kind = "toggle", checked = flag,
                    help = "Synthetic UI state only; no game memory is changed."}
        elseif index == 2 then
            return {key = "details", label = "Wrapped detail", kind = "link", target = detail.key,
                    help = "The native font must wrap the full body and page it."}
        elseif index == 3 then
            return {key = "marker", label = "Emit native action marker", kind = "action"}
        elseif index == 4 then
            return {key = "count", label = "Adjustments", kind = "info", value = tostring(adjustments)}
        elseif index == 5 then
            return {key = "failure", label = "Fail callback witness", kind = "action",
                    help = "Explicit mod-test recovery case. This action requests controlled restart or quit."}
        end
        error("Unexpected native UI fixture row")
    end,
    find = function(key: string): number?
        if key == "flag" then return 1 end
        if key == "details" then return 2 end
        if key == "marker" then return 3 end
        if key == "count" then return 4 end
        if key == "failure" then return 5 end
        return nil
    end,
    adjust = function(key: string, direction: number)
        if key == "flag" then
            flag = not flag
            adjustments += 1
            p4.log("UI_ADJUST flag=" .. tostring(flag))
        elseif key == "marker" and direction == 0 then
            adjustments += 1
            p4.log("UI_ACTION marker")
        elseif key == "failure" and direction == 0 then
            error("UI_CALLBACK_BAD")
        end
        local page = options
        if page then assert(page:refresh()) end
    end,
    opened = function() p4.log("UI_OPEN options") end,
    closed = function() p4.log("UI_CLOSE options") end,
})
p4.log("UI_READY " .. options.key .. " " .. detail.key)
