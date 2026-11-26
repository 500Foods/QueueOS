-- Firmware Boot list. The host stand-in stays in tests/boot-tui.lua.
-- This file uses the same mark, geometry, and timer rules, and draws
-- through the firmware table Boot installs. It does not start Base.
-- The medium shade in the mark is drawn as the light shade by the shim,
-- in white, the same ink as Q and OS. Outside the mark the field is
-- that light shade. Columns 1 and the last column stay solid. The
-- mark's border is solid spaces. The list border is black spaces, ten
-- columns wider than the mark. The titles sit one blank row above the
-- slots, and the text is indented one column inside that border. One
-- solid red box holds each button. Eight columns of the field sit
-- between those outer edges when the row is wide enough. Space and
-- Enter sit under the captions in the field color. The countdown is
-- that image's number, on the gap row under the list. The pointer is
-- a yellow block. A move restores one cell. Columns 1 and the last
-- column are drawn again after the picture, then the block. The
-- firmware caret stays off.

local Mark = require("wordmark")
local Geo = require("screen-geo")
local Screen = require("boot-screen")
local Json = require("json-text")

local SLOT_COUNT = 10
local POINTER = utf8.char(0x2588)
local draw_app

local function char_len(b)
    if b >= 0xF0 then
        return 4
    elseif b >= 0xE0 then
        return 3
    elseif b >= 0xC0 then
        return 2
    end
    return 1
end

local function fit(text, width)
    if width < 1 then
        return ""
    end
    local i = 1
    local w = 0
    local last = 0
    while i <= #text and w < width do
        local len = char_len(text:byte(i))
        if i + len - 1 > #text then
            len = 1
        end
        w = w + 1
        i = i + len
        last = i - 1
    end
    local shown = text:sub(1, last)
    if w < width then
        shown = shown .. string.rep(" ", width - w)
    end
    return shown
end

local function wrap_one(line, width)
    if width < 1 or line == "" then
        return { "" }
    end
    local out = {}
    local buf = {}
    local col = 0
    local i = 1
    while i <= #line do
        local len = char_len(line:byte(i))
        if i + len - 1 > #line then
            len = 1
        end
        if col >= width then
            out[#out + 1] = table.concat(buf)
            buf = {}
            col = 0
        end
        buf[#buf + 1] = line:sub(i, i + len - 1)
        col = col + 1
        i = i + len
    end
    if #buf > 0 then
        out[#out + 1] = table.concat(buf)
    end
    if #out == 0 then
        out[1] = ""
    end
    return out
end

local function wrap_lines(lines, width)
    local out = {}
    for n = 1, #lines do
        local parts = wrap_one(lines[n], width)
        for p = 1, #parts do
            out[#out + 1] = parts[p]
        end
    end
    if #out == 0 then
        out[1] = ""
    end
    return out
end

local function split_lines(text)
    local out = {}
    local start = 1
    if text == nil or text == "" then
        return { "" }
    end
    while true do
        local nl = text:find("\n", start, true)
        if nl == nil then
            out[#out + 1] = text:sub(start)
            break
        end
        out[#out + 1] = text:sub(start, nl - 1)
        start = nl + 1
    end
    return out
end

local function status_word(slot)
    if slot.kind == "candidate" then
        if slot.name ~= "" then
            return slot.name
        end
        return "QueueOS"
    end
    if slot.kind == "unsafe" then
        return "UNSAFE"
    end
    if slot.kind == "empty" then
        return "empty"
    end
    if slot.kind == "foreign" then
        return "not QueueOS"
    end
    return "unreadable"
end

local function prepare(slot)
    local lines = {
        string.format("slot %d  %s", slot.index or 0, status_word(slot)),
    }
    if slot.kind == "unreadable" or type(slot.json) ~= "string" then
        lines[#lines + 1] = "This slot is not JSON."
    else
        if slot.kind == "unsafe" then
            lines[#lines + 1] = "The hash does not match."
        end
        lines[#lines + 1] = ""
        local value = Json.parse(slot.json)
        if value == nil then
            lines[#lines + 1] = "This slot is not JSON."
        else
            local parts = split_lines(Json.pretty(value))
            for i = 1, #parts do
                lines[#lines + 1] = parts[i]
            end
        end
    end
    slot.view_lines = lines
end

local function remember(row, col, text, attr)
    local drawn
    local x
    local i
    if draw_app == nil or text == nil or text == "" then
        return
    end
    drawn = draw_app.drawn
    if drawn == nil or row < 1 or col < 1 then
        return
    end
    if drawn[row] == nil then
        drawn[row] = {}
    end
    x = col
    i = 1
    while i <= #text do
        local len = char_len(text:byte(i))
        if i + len - 1 > #text then
            len = 1
        end
        drawn[row][x] = { text:sub(i, i + len - 1), attr }
        x = x + 1
        i = i + len
    end
end

local function write_at(row, col, text, attr)
    firmware.write(row, col, text, attr)
    remember(row, col, text, attr)
end

local function restore_pointer(app)
    local cell = app.ptr
    if cell == nil then
        return
    end
    firmware.write(cell.y, cell.x, cell.ch, cell.attr)
    app.ptr = nil
end

local function draw_pointer(app, rows, cols)
    local saved
    local ch = Screen.FILL
    local attr = "field"
    if app.px == nil or app.py == nil then
        return
    end
    if app.py < 1 or app.px < 1 or app.py > rows or app.px > cols then
        return
    end
    if app.drawn ~= nil and app.drawn[app.py] ~= nil then
        saved = app.drawn[app.py][app.px]
    end
    if saved ~= nil then
        ch = saved[1]
        attr = saved[2]
    elseif app.px == 1 or app.px == cols then
        ch = " "
        attr = "field"
    end
    app.ptr = { y = app.py, x = app.px, ch = ch, attr = attr }
    firmware.write(app.py, app.px, POINTER, "pointer")
end

local function write_word(row, cols)
    local parts = {
        { "Q", "white" },
        { "ueue", "white" },
        { "OS", "white" },
    }
    local x = 1 + math.floor((cols - 7) / 2)
    local i
    if cols < 7 or row < 1 then
        return
    end
    for i = 1, #parts do
        write_at(row, x, parts[i][1], parts[i][2])
        x = x + #parts[i][1]
    end
end

local function write_banner(geo)
    local i
    for i = 1, Mark.height do
        local x = geo.art_left or geo.left
        local runs = Mark.runs(Mark.lines[i])
        local r
        for r = 1, #runs do
            local run = runs[r]
            local attr = "white"
            if run.kind == "frame" then
                attr = "field"
            end
            write_at(geo.art_row + i - 1, x, run.text, attr)
            x = x + run.cols
        end
    end
end

local function blank_band(geo, rows, cols)
    local band = geo.button_h or 4
    local x, n = Screen.field_span(geo.left, geo.width, cols)
    local line
    local r
    if n < 1 then
        return
    end
    line = string.rep(Screen.FILL, n)
    for r = 0, band - 1 do
        local y = geo.button_row + r
        if y >= 1 and y <= rows then
            write_at(y, x, line, "field")
        end
    end
end

local function write_buttons(app, geo, parts, rows, cols, draw)
    local placed = Screen.buttons(geo, parts)
    local band = geo.button_h or 4
    local i
    local function put(dy, col, text, attr)
        local row = geo.button_row + dy
        if text ~= "" and row >= 1 and row <= rows then
            write_at(row, col, text, attr)
        end
    end
    local function cover(box)
        local x0 = box.x
        local x1 = box.x1
        local dy0 = box.label_dy
        local dy1 = box.hint_dy or box.label_dy
        local dy
        if box.frame then
            x0 = box.frame.x
            x1 = box.frame.x1
            dy0 = 0
            dy1 = band - 1
        end
        for dy = dy0, dy1 do
            local row = geo.button_row + dy
            if row >= 1 and row <= rows then
                app.hotspots[#app.hotspots + 1] = {
                    y = row, x0 = x0, x1 = x1, kind = box.kind,
                }
            end
        end
    end
    app.hotspots = app.hotspots or {}
    if draw then
        local b
        blank_band(geo, rows, cols)
        for b = 1, #placed do
            local frame = placed[b].frame
            if frame then
                local rule = string.rep(" ", frame.x1 - frame.x + 1)
                local dy
                for dy = 0, band - 1 do
                    put(dy, frame.x, rule, "field")
                end
            end
        end
    end
    for i = 1, #placed do
        local box = placed[i]
        if draw then
            put(box.label_dy, box.label_x, box.label, box.attr)
            if box.hint_dy then
                put(box.hint_dy, box.hint_x, box.hint, "field")
            end
        end
        cover(box)
    end
end

-- Red margins outside the block. A full-width console has no margin,
-- and column 1 is then the list. The last cell of the last row scrolls.
local function solid_edges(app, rows, cols)
    local geo = app.geo
    local left = geo and geo.left or 1
    local right = left + (geo and geo.width or cols) - 1
    local y
    if cols < 2 or rows < 1 then
        return
    end
    if left > 1 then
        for y = 1, rows do
            write_at(y, 1, " ", "field")
        end
    end
    if right < cols then
        for y = 1, rows - 1 do
            write_at(y, cols, " ", "field")
        end
    end
end

local function hotspot_at(app, x, y)
    local i
    for i = 1, #(app.hotspots or {}) do
        local spot = app.hotspots[i]
        if y == spot.y and x >= spot.x0 and x <= spot.x1 then
            return spot
        end
    end
    return nil
end

local function fill_field(rows, cols)
    local x, n
    local line
    local y
    if firmware.field ~= nil then
        firmware.field()
        return
    end
    x, n = Screen.field_span(1, cols, cols)
    if n < 1 then
        return
    end
    line = string.rep(Screen.FILL, n)
    for y = 1, rows do
        firmware.write(y, x, line, "field")
    end
end

local function button_parts(state, app)
    local slot
    local explore_attr
    local launch_attr
    if state.mode == "explore" then
        local attr = "white"
        if app.hover == "back" then
            attr = "hover"
        end
        return { { label = "Back to Images", attr = attr, kind = "back" } }
    end
    slot = state.slots[state.selected]
    explore_attr = "white"
    launch_attr = "field"
    if slot and slot.launchable then
        launch_attr = "white"
    end
    if app.hover == "explore" then
        explore_attr = "hover"
    end
    if app.hover == "launch" and slot and slot.launchable then
        launch_attr = "hover"
    end
    return {
        { label = "Explore Image", attr = explore_attr, kind = "explore" },
        { label = Screen.launch_label(state, nil), attr = launch_attr, kind = "launch" },
    }
end

local function write_countdown(state, geo, rows, cols, now)
    local row = Geo.countdown_row(geo)
    local x, n
    local text
    local at
    if geo == nil or row == nil or row < 1 or row > rows then
        return
    end
    x, n = Screen.field_span(geo.left, geo.width, cols)
    if n < 1 then
        return
    end
    text = Screen.countdown_text(state, now)
    write_at(row, x, string.rep(Screen.FILL, n), "field")
    if #text > 0 and #text <= n then
        at = math.floor((n - #text) / 2)
        write_at(row, x + at, text, "white")
    end
end

local function redraw_slot(state, app, index, rows)
    local geo = app.geo
    local spot
    local i
    local left, width
    local attr
    local slot
    if geo == nil or index == nil or index == 0 then
        return
    end
    for i = 1, #(app.hotspots or {}) do
        local item = app.hotspots[i]
        if item.kind == "slot" and item.index == index then
            spot = item
            break
        end
    end
    if spot == nil or spot.y < 1 or spot.y > rows then
        return
    end
    left, width = Screen.inner(geo)
    slot = state.slots[index]
    if slot == nil then
        return
    end
    attr = "list"
    if index == state.selected then
        attr = "bar"
    elseif index == app.hover_slot then
        attr = "list-hover"
    end
    if (geo.width or 0) >= 6 then
        write_at(spot.y, spot.x0, " ", attr)
        write_at(spot.y, spot.x1, " ", attr)
    end
    write_at(spot.y, left, fit(Screen.line(slot, Screen.place(state.slots, index)), width), attr)
end

local function write_labels(app, geo, rows, state)
    local placed = Screen.buttons(geo, button_parts(state, app))
    local i
    for i = 1, #placed do
        local box = placed[i]
        local y = geo.button_row + box.label_dy
        if y >= 1 and y <= rows then
            write_at(y, box.label_x, box.label, box.attr)
        end
    end
end

local function write_scrollbar(app, geo, rows, cols, count, window, scroll)
    local col = Screen.bar_column(geo)
    local cells
    local i
    if col == nil then
        return
    end
    cells = Screen.scrollbar(count, window, scroll)
    for i = 1, #cells do
        local y = geo.head_row + i - 1
        local cell = cells[i]
        local attr = "list"
        if y >= 1 and y <= rows and not (y == rows and col == cols) then
            if cell.role == "up" or cell.role == "down" then
                attr = "arrow"
            elseif cell.role == "thumb" then
                attr = "bar"
            end
            write_at(y, col, cell.glyph, attr)
            app.hotspots[#app.hotspots + 1] = {
                y = y, x0 = col, x1 = col, kind = cell.role,
            }
        end
    end
end

local function write_frame(geo, rows)
    local w = geo.width
    local top = geo.box_row
    local height = Geo.frame_rows(geo.slots)
    local line
    local y
    if w < 2 or top == nil then
        return
    end
    line = string.rep(" ", w)
    for y = top, top + height - 1 do
        if y >= 1 and y <= rows then
            write_at(y, geo.left, line, "list")
        end
    end
end

local function paint(state, app, rows, cols, now, timer_only)
    local geo = Geo.place(rows, cols, Mark, SLOT_COUNT)
    local fresh = not app.painted or app.paint_rows ~= rows or app.paint_cols ~= cols
    local top
    local room
    local left
    local width
    app.hotspots = {}
    app.geo = geo
    draw_app = app
    app.drawn = app.drawn or {}
    if fresh then
        app.ptr = nil
        app.drawn = {}
        firmware.clear()
        app.painted = true
        app.paint_rows = rows
        app.paint_cols = cols
        fill_field(rows, cols)
        if geo.banner then
            write_banner(geo)
        else
            write_word(geo.art_row, cols)
        end
    elseif not timer_only then
        restore_pointer(app)
    end
    top = geo.list_row
    room = geo.slots
    left, width = Screen.inner(geo)
    if state.mode == "explore" then
        local view = Geo.frame_rows(geo.slots) - 2
        local lines = wrap_lines(Screen.explore_lines(state), width)
        local first
        local i
        Screen.clamp_scroll(state, #lines, view)
        app.explore_lines = #lines
        app.explore_room = view
        first = state.json_scroll + 1
        write_frame(geo, rows)
        for i = 1, view do
            local y = geo.head_row + i - 1
            if y >= 1 and y <= rows and y ~= geo.button_row then
                local line = lines[first + i - 1] or ""
                write_at(y, left, fit(line, width), "list")
            end
        end
        write_scrollbar(app, geo, rows, cols, #lines, view, state.json_scroll)
        write_countdown(state, geo, rows, cols, now)
        write_buttons(app, geo, button_parts(state, app), rows, cols, true)
        solid_edges(app, rows, cols)
        draw_pointer(app, rows, cols)
        return
    end
    if not timer_only then
        local order, scroll, window = Screen.window(state, app.list_scroll, room)
        local i
        app.list_scroll = scroll
        local x0, x1 = Screen.row_span(geo, left, width)
        write_frame(geo, rows)
        if geo.head_row >= 1 and geo.head_row <= rows then
            if geo.width >= 6 then
                write_at(geo.head_row, x0, " ", "list")
                write_at(geo.head_row, x1, " ", "list")
            end
            write_at(geo.head_row, left, fit(Screen.header(), width), "list")
        end
        for i = 1, window do
            local pri = scroll + i
            local index = order[pri]
            local slot = state.slots[index]
            local y = top + i - 1
            if slot and y >= 1 and y <= rows and y ~= geo.button_row then
                local attr = "list"
                if index == state.selected then
                    attr = "bar"
                elseif index == app.hover_slot then
                    attr = "list-hover"
                end
                if geo.width >= 6 then
                    write_at(y, x0, " ", attr)
                    write_at(y, x1, " ", attr)
                end
                write_at(y, left, fit(Screen.line(slot, pri), width), attr)
                app.hotspots[#app.hotspots + 1] = {
                    y = y,
                    x0 = x0,
                    x1 = x1,
                    kind = "slot",
                    index = index,
                }
            end
        end
    else
        local order, scroll, window = Screen.window(state, app.list_scroll, room)
        local x0, x1 = Screen.row_span(geo, left, width)
        local i
        app.list_scroll = scroll
        for i = 1, window do
            local pri = scroll + i
            local index = order[pri]
            local y = top + i - 1
            if index and y >= 1 and y <= rows then
                app.hotspots[#app.hotspots + 1] = {
                    y = y,
                    x0 = x0,
                    x1 = x1,
                    kind = "slot",
                    index = index,
                }
            end
        end
    end
    write_countdown(state, geo, rows, cols, now)
    write_buttons(app, geo, button_parts(state, app), rows, cols, not timer_only)
    if not timer_only then
        solid_edges(app, rows, cols)
    end
    draw_pointer(app, rows, cols)
end

local function on_key(state, app, name, now)
    local room = 1
    if app.geo then
        room = app.geo.slots
    end
    if state.mode == "explore" and app.explore_room then
        room = app.explore_room
    end
    if name == "q" then
        state.quit = true
    elseif state.mode == "explore" then
        if name == "esc" or name == "b" then
            Screen.back(state, now)
        elseif name == "up" or name == "k" then
            Screen.scroll(state, -1, app.explore_lines or 1, room)
        elseif name == "down" or name == "j" then
            Screen.scroll(state, 1, app.explore_lines or 1, room)
        elseif name == "pageup" then
            Screen.scroll(state, -room, app.explore_lines or 1, room)
        elseif name == "pagedown" then
            Screen.scroll(state, room, app.explore_lines or 1, room)
        elseif name == "home" then
            state.json_scroll = 0
        elseif name == "end" then
            Screen.scroll(state, 1000000, app.explore_lines or 1, room)
        end
    elseif name == "esc" then
        state.quit = true
    elseif name == "e" or name == "space" then
        Screen.explore(state)
    elseif name == "enter" then
        Screen.launch(state)
    elseif name == "up" or name == "k" then
        Screen.move(state, -1, now)
    elseif name == "down" or name == "j" then
        Screen.move(state, 1, now)
    elseif name == "pageup" then
        Screen.move(state, -math.max(1, room), now)
    elseif name == "pagedown" then
        Screen.move(state, math.max(1, room), now)
    elseif name == "home" then
        Screen.edge(state, false, now)
    elseif name == "end" then
        Screen.edge(state, true, now)
    end
end

local function on_pointer(state, app, event, now)
    local spot = hotspot_at(app, event.x, event.y)
    app.px = event.x
    app.py = event.y
    Screen.wheel_scroll(state, event.wheel or 0, app.explore_lines or 1,
        app.explore_room or 1)
    if event.click then
        local room = app.explore_room or 1
        local lines = app.explore_lines or 1
        if spot and spot.kind == "slot" then
            Screen.select(state, spot.index, now)
        elseif spot and spot.kind == "explore" then
            Screen.explore(state)
        elseif spot and spot.kind == "launch" then
            Screen.launch(state)
        elseif spot and spot.kind == "back" then
            Screen.back(state, now)
        elseif spot then
            Screen.scroll_kind(state, spot.kind, lines, room)
        end
        return
    end
    local hover_slot = 0
    local hover = nil
    if spot and spot.kind == "slot" then
        hover_slot = spot.index
    elseif spot then
        hover = spot.kind
    end
    app.hover_slot = hover_slot
    app.hover = hover
end

local function shown_second(left)
    if type(left) ~= "number" then
        return nil
    end
    return math.floor(left)
end

-- Move the block, and repaint a row or a label only when the hotspot
-- under it changed. The mark and the field stay where they are.
local function nudge(state, app, rows, cols, now, prev_slot, prev_hover, shown)
    local left = shown_second(Screen.launch_seconds(state, now))
    local slot_moved = state.mode == "list" and (app.hover_slot or 0) ~= prev_slot
    local hover_moved = app.hover ~= prev_hover or slot_moved
    restore_pointer(app)
    if slot_moved then
        if prev_slot ~= 0 then
            redraw_slot(state, app, prev_slot, rows)
        end
        if (app.hover_slot or 0) ~= 0 then
            redraw_slot(state, app, app.hover_slot, rows)
        end
    end
    if hover_moved and app.geo ~= nil then
        write_labels(app, app.geo, rows, state)
    end
    if left ~= shown and app.geo ~= nil then
        write_countdown(state, app.geo, rows, cols, now)
    end
    solid_edges(app, rows, cols)
    draw_pointer(app, rows, cols)
    return left
end

local function run(slots)
    local i
    local state
    local app
    local shown
    for i = 1, #slots do
        prepare(slots[i])
    end
    state = Screen.new(slots, firmware.now())
    app = { hotspots = {}, hover = nil, hover_slot = 0, list_scroll = 0 }
    shown = shown_second(Screen.launch_seconds(state, firmware.now()))
    paint(state, app, firmware.rows(), firmware.cols(), firmware.now(), false)
    while not state.quit and state.launched == nil do
        local event = firmware.wait()
        local now = firmware.now()
        local rows = firmware.rows()
        local cols = firmware.cols()
        local left
        local fast = false
        if event then
            Screen.activity(state, now)
            if event.key then
                on_key(state, app, event.key, now)
            elseif event.pointer then
                local prev_slot = app.hover_slot or 0
                local prev_hover = app.hover
                local prev_mode = state.mode
                local prev_sel = state.selected
                local prev_scroll = state.json_scroll
                on_pointer(state, app, event, now)
                fast = state.mode == prev_mode
                    and state.selected == prev_sel
                    and state.json_scroll == prev_scroll
                    and state.launched == nil
                    and rows == app.paint_rows
                    and cols == app.paint_cols
                if fast then
                    shown = nudge(state, app, rows, cols, now, prev_slot, prev_hover, shown)
                end
            end
        end
        if Screen.tick(state, now) then
            break
        end
        if not fast then
            left = shown_second(Screen.launch_seconds(state, now))
            if event or left ~= shown then
                local timer_only = (not event) and left ~= shown
                shown = left
                paint(state, app, rows, cols, now, timer_only)
            end
        end
    end
    return state.launched or 0
end

return run(slots)
