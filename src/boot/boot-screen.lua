-- boot-screen.lua
--
-- The Boot list. Ten slots stay in slot order. QueueOS is above them
-- and the buttons are below them. The host stand-in paints this.
-- Firmware will paint through its own shim. This file does not call
-- terminal.lua and it does not start Base.
--
-- The first selection is the newest launchable candidate, by the
-- publication string. Slot 1 is the selection when none can launch.
-- A 10 second timer runs only on that row, and only while the list
-- is showing. Keyboard or pointer activity starts the 10 seconds over.
-- Explore pauses it. An empty, UNSAFE, foreign, or unreadable row
-- is not started.

local Screen = {}

Screen.TIMEOUT = 10

local function selected_slot(state)
    return state.slots[state.selected]
end

function Screen.initial(slots)
    local best, best_ts
    for i = 1, #slots do
        local slot = slots[i]
        if slot.launchable then
            local ts = slot.published or ""
            if best == nil or ts > best_ts or (ts == best_ts and i < best) then
                best = i
                best_ts = ts
            end
        end
    end
    if best == nil then
        return 1
    end
    return best
end

function Screen.activity(state, now)
    local slot = selected_slot(state)
    if state.mode == "list" and slot and slot.launchable and type(now) == "number" then
        state.deadline = now + Screen.TIMEOUT
    else
        state.deadline = nil
    end
end

function Screen.new(slots, now)
    local state = {
        slots = slots,
        selected = Screen.initial(slots),
        mode = "list",
        json_scroll = 0,
        deadline = nil,
        launched = nil,
        quit = false,
    }
    Screen.activity(state, now)
    return state
end

function Screen.move(state, delta, now)
    if state.mode ~= "list" then
        return false
    end
    local n = #state.slots
    if n < 1 then
        return false
    end
    local i = state.selected + delta
    if i < 1 then
        i = 1
    elseif i > n then
        i = n
    end
    state.selected = i
    Screen.activity(state, now)
    return true
end

function Screen.select(state, index, now)
    if state.mode ~= "list" then
        return false
    end
    if type(index) ~= "number" or index < 1 or index > #state.slots then
        return false
    end
    state.selected = index
    Screen.activity(state, now)
    return true
end

function Screen.explore(state)
    if state.mode ~= "list" then
        return false
    end
    if selected_slot(state) == nil then
        return false
    end
    state.mode = "explore"
    state.json_scroll = 0
    state.deadline = nil
    return true
end

function Screen.back(state, now)
    if state.mode ~= "explore" then
        return false
    end
    state.mode = "list"
    state.json_scroll = 0
    Screen.activity(state, now)
    return true
end

function Screen.launch(state)
    if state.mode ~= "list" then
        return false
    end
    local slot = selected_slot(state)
    if slot == nil or not slot.launchable or slot.base == nil then
        return false
    end
    state.launched = state.selected
    state.deadline = nil
    return true
end

function Screen.tick(state, now)
    if state.launched ~= nil then
        return true
    end
    if state.mode ~= "list" or state.deadline == nil or type(now) ~= "number" then
        return false
    end
    local slot = selected_slot(state)
    if slot == nil or not slot.launchable then
        return false
    end
    if now >= state.deadline then
        return Screen.launch(state)
    end
    return false
end

-- Seconds still shown on the Launch button. Nil when the timer is not running.
function Screen.launch_seconds(state, now)
    if state.mode ~= "list" or state.deadline == nil or type(now) ~= "number" then
        return nil
    end
    local slot = selected_slot(state)
    if slot == nil or not slot.launchable then
        return nil
    end
    local left = state.deadline - now
    if left < 1 then
        return nil
    end
    return left
end

function Screen.explore_lines(state)
    local slot = selected_slot(state)
    if slot == nil or slot.view_lines == nil then
        return { "" }
    end
    return slot.view_lines
end

function Screen.clamp_scroll(state, line_count, window)
    window = window or 1
    if window < 1 then
        window = 1
    end
    local max_scroll = 0
    if line_count > window then
        max_scroll = line_count - window
    end
    if state.json_scroll < 0 then
        state.json_scroll = 0
    elseif state.json_scroll > max_scroll then
        state.json_scroll = max_scroll
    end
end

function Screen.scroll(state, delta, line_count, window)
    if state.mode ~= "explore" then
        return false
    end
    state.json_scroll = state.json_scroll + delta
    Screen.clamp_scroll(state, line_count, window)
    return true
end

local function num_text(n)
    if type(n) ~= "number" then
        return ""
    end
    return string.format("%.0f", n)
end

-- Printed after the terminal is restored. This does not start Base.
function Screen.report(state)
    local slot = state.slots[state.launched or 0]
    if slot == nil or slot.base == nil then
        return ""
    end
    local base = slot.base
    return table.concat({
        "launch slot " .. tostring(slot.index),
        "name " .. (slot.name or ""),
        "file " .. (base.file or ""),
        "offset " .. num_text(base.offset),
        "length " .. num_text(base.length),
        "sha256 " .. (base.sha256 or ""),
        "",
    }, "\n")
end

return Screen
