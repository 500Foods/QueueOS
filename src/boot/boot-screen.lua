-- boot-screen.lua
--
-- The Boot list. Rows are drawn in boot priority: newest launchable
-- slot first, an equal publication string keeps the lower slot index,
-- then the rows that cannot launch, in slot order. The selected value
-- stays the disk slot. QueueOS is above the list and the buttons are
-- below it. The host stand-in paints this. Firmware paints through its
-- own shim. This file does not call terminal.lua and it does not start
-- Base.
--
-- The first selection is the newest launchable candidate, by the
-- publication string. Slot 1 is the selection when none can launch.
-- A launchable row arms its own countdown, and only while the list is
-- showing. Under 10, or a value that is not a number, does not arm and
-- does not advance. Over 30 is 30. Keyboard or pointer activity starts
-- that countdown over. Explore pauses it. An empty, UNSAFE, foreign,
-- or unreadable row is not started.

local Screen = {}

-- Seconds this row will count, or nil when it does not arm.
function Screen.countdown_of(slot)
    local n = slot and slot.countdown
    if type(n) ~= "number" then
        return nil
    end
    n = math.floor(n)
    if n < 10 then
        return nil
    end
    if n > 30 then
        return 30
    end
    return n
end

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
    local seconds = Screen.countdown_of(slot)
    if state.mode == "list" and slot and slot.launchable and seconds ~= nil
        and type(now) == "number" then
        state.deadline = now + seconds
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

-- Disk slot indexes, newest launchable first. Ties keep the lower index.
-- Rows that cannot launch follow, in slot order.
function Screen.order(slots)
    local launch = {}
    local rest = {}
    local out = {}
    local i
    for i = 1, #slots do
        if slots[i] and slots[i].launchable then
            launch[#launch + 1] = i
        else
            rest[#rest + 1] = i
        end
    end
    table.sort(launch, function(a, b)
        local ta = slots[a].published or ""
        local tb = slots[b].published or ""
        if ta ~= tb then
            return ta > tb
        end
        return a < b
    end)
    for i = 1, #launch do
        out[#out + 1] = launch[i]
    end
    for i = 1, #rest do
        out[#out + 1] = rest[i]
    end
    return out
end

-- Visible slice of Screen.order. scroll is the number of rows above the window.
function Screen.window(state, scroll, room)
    local order = Screen.order(state.slots)
    local count = #order
    local window = room or 1
    local pos = Screen.place(state.slots, state.selected)
    if window < 1 then
        window = 1
    end
    if window > count then
        window = count
    end
    if pos < 1 then
        pos = 1
    end
    scroll = scroll or 0
    if window >= count then
        scroll = 0
    else
        if pos <= scroll then
            scroll = pos - 1
        elseif pos > scroll + window then
            scroll = pos - window
        end
        if scroll < 0 then
            scroll = 0
        elseif scroll > count - window then
            scroll = count - window
        end
    end
    return order, scroll, window
end

-- 1-based row in Screen.order, or 0 when the index is not in the list.
function Screen.place(slots, index)
    local order = Screen.order(slots)
    local i
    for i = 1, #order do
        if order[i] == index then
            return i
        end
    end
    return 0
end

function Screen.move(state, delta, now)
    if state.mode ~= "list" then
        return false
    end
    local order = Screen.order(state.slots)
    local n = #order
    local pos
    if n < 1 or type(delta) ~= "number" then
        return false
    end
    pos = Screen.place(state.slots, state.selected)
    if pos < 1 then
        pos = 1
    end
    pos = pos + delta
    if pos < 1 then
        pos = 1
    elseif pos > n then
        pos = n
    end
    state.selected = order[pos]
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

-- Home and End follow the drawn order. The stored selection is the disk slot.
function Screen.edge(state, last, now)
    local order = Screen.order(state.slots)
    local index
    if #order < 1 then
        return false
    end
    index = order[1]
    if last then
        index = order[#order]
    end
    return Screen.select(state, index, now)
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

-- Whole seconds still left. Nil when the timer is not running.
-- The Launch button does not carry this number.
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

-- Right border of the JSON frame. A narrow frame keeps that cell for text.
function Screen.bar_column(geo)
    local width = geo and geo.width or 0
    if width < 6 then
        return nil
    end
    return (geo.left or 1) + width - 1
end

-- One cell per JSON row. Arrows occupy the ends. The thumb is the
-- visible window, and the black cells beside it page by that window.
-- Wheel-up is a negative count. One notch moves three lines.
Screen.UP = utf8.char(0x2191)
Screen.DOWN = utf8.char(0x2193)

function Screen.scrollbar(count, window, scroll)
    local rows = window or 0
    local out = {}
    local track
    local thumb
    local start
    local max_scroll
    local i
    if rows < 2 then
        return out
    end
    if type(count) ~= "number" or count < 0 then
        count = 0
    end
    if type(scroll) ~= "number" or scroll < 0 then
        scroll = 0
    end
    if window < 1 then
        window = 1
    end
    out[1] = { glyph = Screen.UP, role = "up" }
    track = rows - 2
    max_scroll = 0
    if count > window then
        max_scroll = count - window
    end
    if scroll > max_scroll then
        scroll = max_scroll
    end
    thumb = track
    start = 0
    if track > 0 and count > window then
        thumb = math.floor((track * window) / count)
        if thumb < 1 then
            thumb = 1
        end
        if thumb > track then
            thumb = track
        end
        if max_scroll > 0 and track > thumb then
            start = math.floor((scroll * (track - thumb)) / max_scroll)
        end
    end
    for i = 0, track - 1 do
        local role = "thumb"
        if i < start then
            role = "page-up"
        elseif i >= start + thumb then
            role = "page-down"
        end
        out[#out + 1] = { glyph = " ", role = role }
    end
    out[#out + 1] = { glyph = Screen.DOWN, role = "down" }
    return out
end

function Screen.scroll_kind(state, kind, line_count, window)
    if kind == "up" then
        return Screen.scroll(state, -1, line_count, window)
    end
    if kind == "down" then
        return Screen.scroll(state, 1, line_count, window)
    end
    if kind == "page-up" then
        return Screen.scroll(state, -(window or 1), line_count, window)
    end
    if kind == "page-down" then
        return Screen.scroll(state, window or 1, line_count, window)
    end
    return false
end

function Screen.wheel_scroll(state, wheel, line_count, window)
    if type(wheel) ~= "number" or wheel == 0 then
        return false
    end
    wheel = math.floor(wheel)
    if wheel > 4 then
        wheel = 4
    elseif wheel < -4 then
        wheel = -4
    end
    return Screen.scroll(state, wheel * 3, line_count, window)
end

local function num_text(n)
    if type(n) ~= "number" then
        return ""
    end
    return string.format("%.0f", n)
end

-- The button word. The seconds are Screen.countdown_text.
function Screen.launch_label(state, now)
    return "Launch Image"
end

function Screen.countdown_text(state, now)
    local left = Screen.launch_seconds(state, now)
    if type(left) ~= "number" then
        return ""
    end
    return string.format("%.0f", math.floor(left))
end

local MONTHS = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
}
local WEEK = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" }
local COL_PRI = 3
local COL_NAME = 12
local COL_VER = 8
local COL_REL = 12
local COL_BOOT = 24

local function cell(text, width)
    text = text or ""
    if #text > width then
        text = text:sub(1, width)
    end
    return text .. string.rep(" ", width - #text)
end

local function ymd(stamp)
    if type(stamp) ~= "string" then
        return nil
    end
    local y, m, d = stamp:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)")
    if y == nil then
        return nil
    end
    y = tonumber(y)
    m = tonumber(m)
    d = tonumber(d)
    if y == nil or m == nil or d == nil then
        return nil
    end
    if m < 1 or m > 12 or d < 1 or d > 31 then
        return nil
    end
    return y, m, d
end

-- Sakamoto. 0 is Sunday.
local function weekday(y, m, d)
    local t = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }
    local n
    if m < 3 then
        y = y - 1
    end
    n = y + math.floor(y / 4) - math.floor(y / 100) + math.floor(y / 400) + t[m] + d
    return n % 7
end

local function release_text(stamp)
    local y, m, d = ymd(stamp)
    if y == nil then
        return ""
    end
    return string.format("%04d-%s-%02d", y, MONTHS[m], d)
end

local function boot_text(stamp)
    local y, m, d = ymd(stamp)
    local hh, mm
    local w
    if y == nil or type(stamp) ~= "string" then
        return ""
    end
    hh, mm = stamp:match("T(%d%d):(%d%d)")
    if hh == nil then
        return ""
    end
    if tonumber(hh) > 23 or tonumber(mm) > 59 then
        return ""
    end
    w = weekday(y, m, d)
    return string.format("%04d-%s-%02d (%s) %s:%s", y, MONTHS[m], d, WEEK[w + 1], hh, mm)
end

local function columns_of(slot)
    local name = ""
    local version = ""
    local release = ""
    local booted = ""
    if type(slot) ~= "table" then
        return name, version, release, booted
    end
    name = slot.name or ""
    version = slot.version or ""
    release = release_text(slot.published)
    booted = boot_text(slot.booted)
    if slot.kind == "empty" then
        return "empty", "", "", ""
    end
    if slot.kind == "unreadable" then
        return "unreadable", "", "", ""
    end
    if slot.kind == "foreign" then
        if name == "" then
            name = "not QueueOS"
        end
    elseif slot.kind == "unsafe" then
        if name == "" then
            name = "UNSAFE"
        end
    elseif name == "" then
        name = "QueueOS"
    end
    return name, version, release, booted
end

function Screen.header()
    return cell("Pri", COL_PRI)
        .. "  " .. cell("Name", COL_NAME)
        .. "  " .. cell("Version", COL_VER)
        .. "  " .. cell("Release", COL_REL)
        .. "  " .. cell("Last Successful Boot", COL_BOOT)
end

-- pri is the 1-based row in boot-priority order, not the disk slot.
function Screen.line(slot, pri)
    local name, version, release, booted = columns_of(slot)
    local text = string.format("%d", pri or 0)
    if #text > COL_PRI then
        text = text:sub(#text - COL_PRI + 1)
    end
    text = string.rep(" ", COL_PRI - #text) .. text
    return text
        .. "  " .. cell(name, COL_NAME)
        .. "  " .. cell(version, COL_VER)
        .. "  " .. cell(release, COL_REL)
        .. "  " .. cell(booted, COL_BOOT)
end

-- Light shade, U+2591. The mark draws "ueue" with the medium shade.
-- This is the lesser fill for the field outside the mark and the buttons.
Screen.FILL = utf8.char(0x2591)

function Screen.spaces(width)
    if width == nil or width < 1 then
        return ""
    end
    return string.rep(" ", width)
end

-- Solid spaces. The list paints them black. The mark paints its own.
function Screen.frame_top(width)
    return Screen.spaces(width)
end

function Screen.frame_bottom(width)
    return Screen.spaces(width)
end

-- Text columns. One space stays black inside each border. A narrow
-- screen with no room for that indent keeps the border and the text
-- against it. A screen with no room for a border uses the whole column.
function Screen.inner(geo)
    local width = geo.width or 0
    local left = geo.left or 1
    if width < 4 then
        return left, width
    end
    if width < 6 then
        return left + 1, width - 2
    end
    return left + 2, width - 4
end

-- The cells just inside the border. The row color covers them, and the
-- text returned by inner sits one column in from each.
function Screen.row_span(geo, left, width)
    if (geo.width or 0) >= 6 then
        return left - 1, left + width
    end
    return left, left + width - 1
end

-- Columns 1 and the last column stay the solid field. Writing the last
-- cell of the last row would scroll the console.
function Screen.field_span(origin, width, cols)
    local x1 = origin or 1
    local x2 = x1 + (width or 0) - 1
    if x1 < 2 then
        x1 = 2
    end
    if cols ~= nil and cols > 1 and x2 > cols - 1 then
        x2 = cols - 1
    end
    if x2 < x1 then
        return 0, 0
    end
    return x1, x2 - x1 + 1
end

function Screen.button_hint(kind)
    if kind == "explore" then
        return "Space"
    end
    if kind == "launch" then
        return "Enter"
    end
    if kind == "back" then
        return "Esc"
    end
    return ""
end

-- Each button has its own red space border when the band has four rows.
-- Eight columns of the field sit between those outer edges when the row
-- is wide enough, and never fewer than four. One extra space sits on
-- each side of the caption, inside the border. Below that height there
-- is no border. The key stays in the field color. Back names Esc.
function Screen.buttons(geo, parts)
    local prefer = 8
    local least = 4
    local band = geo.button_h or 4
    local framed = band >= 4
    local pad = framed and 1 or 0
    local bodies = 0
    local between = prefer
    local total = 0
    local boxes = {}
    local placed = {}
    local x
    local limit
    local i
    for i = 1, #parts do
        local label_dy = framed and 1 or 0
        local hint = ""
        local width = #(parts[i].label or "")
        if band >= label_dy + 2 then
            hint = Screen.button_hint(parts[i].kind)
        end
        if #hint > width then
            width = #hint
        end
        width = width + 2
        boxes[i] = {
            label = parts[i].label or "",
            hint = hint,
            width = width,
            label_dy = label_dy,
            hint_dy = hint ~= "" and (label_dy + 1) or nil,
            attr = parts[i].attr,
            kind = parts[i].kind,
        }
        bodies = bodies + width + pad * 2
    end
    if #boxes > 1 then
        local room = (geo.width or 0) - bodies
        local fit = math.floor(room / (#boxes - 1))
        if fit < least then
            between = least
        elseif fit < prefer then
            between = fit
        end
    end
    total = bodies
    if #boxes > 1 then
        total = total + between * (#boxes - 1)
    end
    x = geo.left
    if total <= geo.width then
        x = geo.left + math.floor((geo.width - total) / 2)
    end
    limit = geo.left + geo.width - 1
    for i = 1, #boxes do
        local box = boxes[i]
        local content = x + pad
        local x1 = content + box.width - 1
        local outer_x1 = x1 + pad
        local item
        if outer_x1 > limit then
            break
        end
        item = {
            x = content,
            x1 = x1,
            label_x = content + math.floor((box.width - #box.label) / 2),
            hint_x = content + math.floor((box.width - #box.hint) / 2),
            label = box.label,
            hint = box.hint,
            label_dy = box.label_dy,
            hint_dy = box.hint_dy,
            framed = framed,
            attr = box.attr,
            kind = box.kind,
        }
        if framed then
            item.frame = { x = x, x1 = outer_x1 }
        end
        placed[#placed + 1] = item
        x = outer_x1 + 1 + between
    end
    return placed
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
