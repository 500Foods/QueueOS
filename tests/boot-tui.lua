#!/usr/bin/env lua
-- Host stand-in for the Boot list. It reads ten slots and does not start Base.
-- No arguments: ten empty slots. A path is a database disk. --check skips the terminal.

local USAGE = [[
Usage: boot-tui.lua [image]

  (no arguments)   ten empty slots, for trying the screen
  image            a database disk, qcow2 or raw
  --check          read sample slots without a terminal
  --help           show this help

The list is wider than the mark. Explore shows that slot's JSON.
Back returns to the list. Launch prints the chosen Base.

q quits. Up and down move. Enter launches. e explores. Esc goes back
from Explore, and quits from the list.
]]
local SLOT_BYTES = 4096
local SLOT_COUNT = 10
local REGION = SLOT_BYTES * SLOT_COUNT
local QCOW_MAGIC = "QFI\251"
local function script_dir()
    local src = arg[0] or ""
    local dir = src:match("^(.*)/")
    if not dir or dir == "" then
        return "."
    end
    return dir
end
local function repo_root()
    local dir = script_dir()
    if dir == "tests" then
        return "."
    end
    local parent = dir:match("^(.*)/tests$")
    if parent and parent ~= "" then
        return parent
    end
    return "."
end
local function boot_dir()
    return repo_root() .. "/src/boot"
end
local Read = dofile(boot_dir() .. "/slot-read.lua")
local Screen = dofile(boot_dir() .. "/boot-screen.lua")
local Mark = dofile(boot_dir() .. "/wordmark.lua")
local Geo = dofile(boot_dir() .. "/screen-geo.lua")
local function quote(text)
    return "'" .. tostring(text):gsub("'", "'\\''") .. "'"
end
local function read_file(path, n)
    local handle, err = io.open(path, "rb")
    if not handle then
        return nil, err or "cannot open file"
    end
    local data
    if n then
        data = handle:read(n)
    else
        data = handle:read("*a")
    end
    handle:close()
    return data or ""
end
-- SHA-256 of the JSON bytes. The firmware hasher is not in this process.
local function sha256_hex(bytes)
    local path = os.tmpname()
    local out, oerr = io.open(path, "wb")
    if not out then
        os.remove(path)
        return nil, oerr or "cannot write a temporary file"
    end
    out:write(bytes)
    out:close()
    local handle = io.popen("sha256sum " .. quote(path) .. " 2>/dev/null")
    if not handle then
        os.remove(path)
        return nil, "sha256sum is not available"
    end
    local line = handle:read("*l") or ""
    handle:close()
    os.remove(path)
    local hex = line:match("^([0-9a-f]+)")
    if hex == nil or #hex ~= 64 then
        return nil, "sha256sum returned no hash"
    end
    return hex
end
local hash_cache = {}
local function hash_cached(bytes)
    local hit = hash_cache[bytes]
    if hit then
        return hit
    end
    local hex, err = sha256_hex(bytes)
    if hex then
        hash_cache[bytes] = hex
    end
    return hex, err
end
local function with_hash(opts)
    local one = {}
    if opts then
        for k, v in pairs(opts) do
            one[k] = v
        end
    end
    if one.hash == nil then
        one.hash = hash_cached
    end
    return one
end
local function run_capture(cmd)
    local handle = io.popen(cmd .. " 2>&1")
    if not handle then
        return nil, "cannot run command"
    end
    local text = handle:read("*a") or ""
    local ok = handle:close()
    if ok == true then
        return text
    end
    if text == "" then
        text = "command failed"
    end
    return nil, text
end
-- qcow2 is a host container. Convert a copy and read that. The source
-- is only opened for reading.
local function image_bytes(path)
    local head, err = read_file(path, 4)
    if head == nil then
        return nil, err
    end
    if #head == 4 and head == QCOW_MAGIC then
        local tmp = os.tmpname()
        local cmd = "qemu-img convert -O raw -- " .. quote(path) .. " " .. quote(tmp)
        local _, cerr = run_capture(cmd)
        if cerr then
            os.remove(tmp)
            return nil, "qemu-img convert failed: " .. cerr
        end
        local data, rerr = read_file(tmp, REGION)
        os.remove(tmp)
        if data == nil then
            return nil, rerr
        end
        return data
    end
    return read_file(path, REGION)
end
local function stdout_is_tty()
    local ok = os.execute("test -t 1")
    return ok == true
end
local function plain_list(slots, selected)
    local order = Screen.order(slots)
    io.stdout:write("QueueOS\n\n")
    io.stdout:write(Screen.header() .. "\n")
    for i = 1, #order do
        local index = order[i]
        local mark = " "
        if index == selected then
            mark = ">"
        end
        io.stdout:write(mark .. " " .. Screen.line(slots[index], i) .. "\n")
    end
end
local function load_terminal()
    local ok, mod = pcall(require, "terminal")
    if ok then
        return mod
    end
    local function capture(flag)
        local handle = io.popen("luarocks --lua-version=5.5 path " .. flag .. " 2>/dev/null")
        if not handle then
            return ""
        end
        local text = handle:read("*a") or ""
        handle:close()
        return (text:gsub("%s+$", ""))
    end
    local lr_path = capture("--lr-path")
    local lr_cpath = capture("--lr-cpath")
    if lr_path ~= "" then
        package.path = lr_path .. ";" .. package.path
    end
    if lr_cpath ~= "" then
        package.cpath = lr_cpath .. ";" .. package.cpath
    end
    local again, loaded = pcall(require, "terminal")
    if not again then
        io.stderr:write("terminal.lua is not available for this Lua.\n")
        io.stderr:write("Install with: luarocks --lua-version=5.5 install terminal\n")
        os.exit(1)
    end
    return loaded
end
local RED = "red"
local ATTR_SILVER = { fg = 252, bg = RED }
local ATTR_WHITE = { fg = "white", bg = RED, brightness = "bright" }
local ATTR_LIST = { fg = "white", bg = "black", brightness = "bright" }
local ATTR_BAR = { fg = "white", bg = RED, brightness = "bright" }
local ATTR_LIST_HOVER = { fg = "yellow", bg = "black", brightness = "bright" }
local ATTR_HOVER = { fg = "yellow", bg = RED, brightness = "bright" }
local ATTR_ARROW = { fg = "red", bg = "black", brightness = "bright" }
local ATTR_DIM = { fg = 252, bg = RED }
local function write_at(t, row, col, text, attr)
    if text == "" then
        return
    end
    t.output.write(
        t.cursor.position.set_seq(row, col),
        t.text.push_seq(attr),
        text,
        t.text.pop_seq())
end
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

local WORD_COLS = 7

local function write_word(t, row, cols)
    if cols < WORD_COLS or row < 1 then
        return
    end
    local parts = {
        { "Q", ATTR_WHITE },
        { "ueue", ATTR_WHITE },
        { "OS", ATTR_WHITE },
    }
    local x = 1 + math.floor((cols - WORD_COLS) / 2)
    for i = 1, #parts do
        write_at(t, row, x, parts[i][1], parts[i][2])
        x = x + #parts[i][1]
    end
end

local function write_banner(t, geo)
    for i = 1, Mark.height do
        local x = geo.art_left or geo.left
        local runs = Mark.runs(Mark.lines[i])
        for r = 1, #runs do
            local run = runs[r]
            local attr = ATTR_WHITE
            if run.kind == "frame" then
                attr = ATTR_SILVER
            end
            write_at(t, geo.art_row + i - 1, x, run.text, attr)
            x = x + run.cols
        end
    end
end

local function geometry(rows, cols)
    return Geo.place(rows, cols, Mark, SLOT_COUNT)
end

local function write_frame(t, geo, rows)
    local w, top = geo.width, geo.box_row
    local height = Geo.frame_rows(geo.slots)
    local line = string.rep(" ", w)
    local y
    if w < 2 or top == nil then
        return
    end
    for y = top, top + height - 1 do
        if y >= 1 and y <= rows then
            write_at(t, y, geo.left, line, ATTR_LIST)
        end
    end
end

local function write_buttons(t, app, geo, parts, rows, cols)
    local placed = Screen.buttons(geo, parts)
    local band = geo.button_h or 4
    local ends = {}
    local i
    if cols < 1 or rows < 1 then
        return ends
    end
    for i = 1, #placed do
        local box = placed[i]
        local frame = box.frame
        local y = geo.button_row + box.label_dy
        if frame then
            local rule = string.rep(" ", frame.x1 - frame.x + 1)
            local dy
            for dy = 0, band - 1 do
                local fy = geo.button_row + dy
                if fy >= 1 and fy <= rows then
                    write_at(t, fy, frame.x, rule, ATTR_SILVER)
                end
            end
        end
        local x0, x1, dy0, dy1 = box.x, box.x1, box.label_dy, box.hint_dy or box.label_dy
        if frame then
            x0, x1, dy0, dy1 = frame.x, frame.x1, 0, band - 1
        end
        for dy = dy0, dy1 do
            local hy = geo.button_row + dy
            if hy >= 1 and hy <= rows then
                app.hotspots[#app.hotspots + 1] = { y = hy, x0 = x0, x1 = x1, kind = box.kind }
            end
        end
        if y >= 1 and y <= rows then
            write_at(t, y, box.label_x, box.label, box.attr)
        end
        if box.hint_dy then
            y = geo.button_row + box.hint_dy
            if y >= 1 and y <= rows then
                write_at(t, y, box.hint_x, box.hint, ATTR_SILVER)
            end
        end
        ends[box.kind] = frame and frame.x1 or box.x1
    end
    return ends
end
local function write_countdown(t, state, geo, rows, cols, now)
    local row = Geo.countdown_row(geo)
    local x, n = Screen.field_span(geo.left, geo.width, cols)
    local text = Screen.countdown_text(state, now)
    if row == nil or row < 1 or row > rows or n < 1 then
        return
    end
    write_at(t, row, x, string.rep(Screen.FILL, n), ATTR_SILVER)
    if #text > 0 and #text <= n then
        write_at(t, row, x + math.floor((n - #text) / 2), text, ATTR_WHITE)
    end
end
local function paint(t, state, app, rows, cols, now)
    app.hotspots = {}
    local geo = geometry(rows, cols)
    app.geo = geo
    t.cursor.position.set(1, 1)
    t.text.attr({ bg = RED, fg = 252 })
    t.clear.screen()
    local fx, fn = Screen.field_span(1, cols, cols)
    if fn > 0 then
        local shade = string.rep(Screen.FILL, fn)
        for row = 1, rows do
            write_at(t, row, fx, shade, ATTR_SILVER)
        end
    end
    if geo.banner then
        write_banner(t, geo)
    else
        write_word(t, geo.art_row, cols)
    end
    local top = geo.list_row
    local room = geo.slots
    local left, width = Screen.inner(geo)
    if state.mode == "explore" then
        local view = Geo.frame_rows(room) - 2
        local lines = wrap_lines(Screen.explore_lines(state), width)
        Screen.clamp_scroll(state, #lines, view)
        app.explore_lines = #lines
        app.explore_room = view
        local first = state.json_scroll + 1
        write_frame(t, geo, rows)
        for i = 1, view do
            local y = geo.head_row + i - 1
            if y >= 1 and y <= rows and y ~= geo.button_row then
                local line = lines[first + i - 1] or ""
                write_at(t, y, left, fit(line, width), ATTR_LIST)
            end
        end
        local attr = ATTR_WHITE
        if app.hover == "back" then
            attr = ATTR_HOVER
        end
        write_countdown(t, state, geo, rows, cols, now)
        write_buttons(t, app, geo, {
            { label = "Back to Images", attr = attr, kind = "back" },
        }, rows, cols)
        local col = Screen.bar_column(geo)
        local cells = Screen.scrollbar(#lines, view, state.json_scroll)
        if col ~= nil then
            for i = 1, #cells do
                local cell = cells[i]
                local y = geo.head_row + i - 1
                local bar_attr = ATTR_LIST
                if cell.role == "up" or cell.role == "down" then
                    bar_attr = ATTR_ARROW
                elseif cell.role == "thumb" then
                    bar_attr = ATTR_BAR
                end
                if y >= 1 and y <= rows and y ~= geo.button_row then
                    write_at(t, y, col, cell.glyph, bar_attr)
                    app.hotspots[#app.hotspots + 1] = {
                        y = y, x0 = col, x1 = col, kind = cell.role,
                    }
                end
            end
        end
        return
    end
    local order, scroll, window = Screen.window(state, app.list_scroll, room)
    app.list_scroll = scroll
    app.list_window = window
    local x0, x1 = Screen.row_span(geo, left, width)
    write_frame(t, geo, rows)
    if geo.head_row >= 1 and geo.head_row <= rows then
        if geo.width >= 6 then
            write_at(t, geo.head_row, x0, " ", ATTR_LIST)
            write_at(t, geo.head_row, x1, " ", ATTR_LIST)
        end
        write_at(t, geo.head_row, left, fit(Screen.header(), width), ATTR_LIST)
    end
    for i = 1, window do
        local pri = scroll + i
        local index = order[pri]
        local slot = state.slots[index]
        local y = top + i - 1
        if slot and y >= 1 and y <= rows and y ~= geo.button_row then
            local attr = ATTR_LIST
            if index == state.selected then
                attr = ATTR_BAR
            elseif index == app.hover_slot then
                attr = ATTR_LIST_HOVER
            end
            if geo.width >= 6 then
                write_at(t, y, x0, " ", attr)
                write_at(t, y, x1, " ", attr)
            end
            write_at(t, y, left, fit(Screen.line(slot, pri), width), attr)
            app.hotspots[#app.hotspots + 1] = {
                y = y, x0 = x0, x1 = x1, kind = "slot", index = index,
            }
        end
    end
    local explore_attr = ATTR_WHITE
    local slot = state.slots[state.selected]
    local launch_attr = ATTR_DIM
    if slot and slot.launchable then
        launch_attr = ATTR_WHITE
    end
    if app.hover == "explore" then
        explore_attr = ATTR_HOVER
    end
    if app.hover == "launch" and slot and slot.launchable then
        launch_attr = ATTR_HOVER
    end
    write_countdown(t, state, geo, rows, cols, now)
    write_buttons(t, app, geo, {
        { label = "Explore Image", attr = explore_attr, kind = "explore" },
        { label = Screen.launch_label(state, now), attr = launch_attr, kind = "launch" },
    }, rows, cols)
end
local function hotspot_at(app, x, y)
    for i = 1, #(app.hotspots or {}) do
        local spot = app.hotspots[i]
        if y == spot.y and x >= spot.x0 and x <= spot.x1 then
            return spot
        end
    end
    return nil
end
local function run_tui(state)
    local t = load_terminal()
    local keys = t.input.keymap.default_keys
    local key_map = t.input.keymap.default_key_map
    local Mouse = dofile(script_dir() .. "/tq-mouse.lua")
    local app = {
        hotspots = {},
        hover = nil,
        hover_slot = 0,
    }
    t.text.push({ bg = RED, fg = 252 })
    t.cursor.position.set(1, 1)
    t.clear.screen()
    t.cursor.visible.set(false)
    local rows, cols = t.size()
    local function redraw(now)
        rows, cols = t.size()
        paint(t, state, app, rows, cols, now)
        t.output.flush()
    end
    redraw(os.time())
    Mouse.enable(t)
    local ok, err = xpcall(function()
        while not state.quit and state.launched == nil do
            local seq = t.input.readansi(0.2)
            local now = os.time()
            if seq == nil then
                if Screen.tick(state, now) then
                    break
                end
                local r, c = t.size()
                local shown = Screen.launch_seconds(state, now)
                if type(shown) == "number" then
                    shown = math.floor(shown)
                end
                if r ~= rows or c ~= cols or shown ~= app.shown_seconds then
                    app.shown_seconds = shown
                    redraw(now)
                end
            else
                Screen.activity(state, now)
                local acted = true
                if Mouse.is_mouse(seq) then
                    local event = Mouse.parse(seq)
                    if event == nil then
                        acted = false
                    elseif event.wheel == 64 or event.wheel == 96 then
                        if not event.release then
                            if state.mode == "explore" then
                                Screen.wheel_scroll(state, -1, app.explore_lines or 1, app.explore_room or 1)
                            else
                                Screen.move(state, -1, now)
                            end
                        end
                    elseif event.wheel == 65 or event.wheel == 97 then
                        if not event.release then
                            if state.mode == "explore" then
                                Screen.wheel_scroll(state, 1, app.explore_lines or 1, app.explore_room or 1)
                            else
                                Screen.move(state, 1, now)
                            end
                        end
                    elseif event.motion then
                        local spot = hotspot_at(app, event.x, event.y)
                        local hover_slot = 0
                        local hover = nil
                        if spot and spot.kind == "slot" then
                            hover_slot = spot.index
                        elseif spot then
                            hover = spot.kind
                        end
                        if hover_slot == app.hover_slot and hover == app.hover then
                            acted = false
                        end
                        app.hover_slot = hover_slot
                        app.hover = hover
                    elseif event.release and event.btn == 0 then
                        local spot = hotspot_at(app, event.x, event.y)
                        if spot and spot.kind == "slot" then
                            Screen.select(state, spot.index, now)
                        elseif spot and spot.kind == "explore" then
                            Screen.explore(state)
                        elseif spot and spot.kind == "launch" then
                            Screen.launch(state)
                        elseif spot and spot.kind == "back" then
                            Screen.back(state, now)
                        elseif spot then
                            Screen.scroll_kind(state, spot.kind, app.explore_lines or 1, app.explore_room or 1)
                        end
                    else
                        acted = false
                    end
                else
                    local name = key_map[seq]
                    local room = 1
                    if app.geo then
                        room = app.geo.slots
                    end
                    if seq == "q" or name == keys.ctrl_c then
                        state.quit = true
                    elseif state.mode == "explore" then
                        local explore_room = app.explore_room or room
                        if name == keys.escape or seq == "b" then
                            Screen.back(state, now)
                        elseif name == keys.up or seq == "k" then
                            Screen.scroll(state, -1, app.explore_lines or 1, explore_room)
                        elseif name == keys.down or seq == "j" then
                            Screen.scroll(state, 1, app.explore_lines or 1, explore_room)
                        elseif name == keys.pageup then
                            Screen.scroll(state, -explore_room, app.explore_lines or 1, explore_room)
                        elseif name == keys.pagedown then
                            Screen.scroll(state, explore_room, app.explore_lines or 1, explore_room)
                        elseif name == keys.home then
                            state.json_scroll = 0
                        elseif name == keys["end"] then
                            Screen.scroll(state, 1000000, app.explore_lines or 1, explore_room)
                        end
                    elseif name == keys.escape then
                        state.quit = true
                    elseif seq == "e" or seq == " " then
                        Screen.explore(state)
                    elseif name == keys.enter or seq == "\r" then
                        Screen.launch(state)
                    elseif name == keys.up or seq == "k" then
                        Screen.move(state, -1, now)
                    elseif name == keys.down or seq == "j" then
                        Screen.move(state, 1, now)
                    elseif name == keys.pageup then
                        Screen.move(state, -math.max(1, room), now)
                    elseif name == keys.pagedown then
                        Screen.move(state, math.max(1, room), now)
                    elseif name == keys.home then
                        Screen.edge(state, false, now)
                    elseif name == keys["end"] then
                        Screen.edge(state, true, now)
                    end
                end
                if state.quit or state.launched ~= nil then
                    break
                end
                if acted then
                    app.shown_seconds = Screen.launch_seconds(state, now)
                    redraw(now)
                end
            end
        end
    end, debug.traceback)
    Mouse.disable(t)
    if not ok then
        error(err, 0)
    end
end
local function open_screen(slots)
    local state = Screen.new(slots, os.time())
    if not stdout_is_tty() then
        plain_list(slots, state.selected)
        return 0
    end
    local t = load_terminal()
    local wrapped = t.initwrap(function()
        run_tui(state)
    end, {
        displaybackup = true,
        filehandle = io.stdout,
        skip_width_detection = true,
        disable_sigint = true,
        autotermrestore = true,
    })
    wrapped()
    if state.launched ~= nil then
        io.stdout:write(Screen.report(state))
    end
    return 0
end
local BASE_HASH = string.rep("ab", 32)

local function generation(name, published, arch, role, countdown)
    countdown = countdown or 10
    local json = string.format(
        '{"OS":"QueueOS","name":"%s","version":"0.1","published":"%s","countdown":%.0f,"binaries":[{"role":"%s","arch":"%s","offset":8388608,"length":4096,"sha256":"%s","file":"Base.efi"}]}',
        name, published, countdown, role or "base", arch or "x86-64", BASE_HASH)
    local hex, err = hash_cached(json)
    if hex == nil then
        return nil, err
    end
    return Read.pack(json, hex), json
end

local function check()
    local failures = 0
    local temps = {}
    local function expect(cond, message)
        if not cond then
            io.stderr:write("boot-tui: " .. message .. "\n")
            failures = failures + 1
        end
    end
    local function finish(code)
        for i = 1, #temps do
            os.remove(temps[i])
        end
        return code
    end
    local function temp_path()
        local path = os.tmpname()
        temps[#temps + 1] = path
        return path
    end
    local empty_hash, herr = hash_cached(Read.EMPTY_JSON)
    expect(herr == nil, herr or "sha256sum failed")
    expect(empty_hash == Read.EMPTY_HASH, "SHA-256 of {} changed")
    expect(Mark.consistent() and Mark.height == 13 and Mark.width == 80, "wordmark is 13 by 80")
    local picture = table.concat(Mark.lines, "\n")
    expect(picture:find("█", 1, true) and picture:find("▒", 1, true)
        and not picture:find("┌", 1, true), "wordmark blocks")
    local filled = geometry(24, 80)
    expect(filled.banner and filled.gap == 1 and filled.slots == 1 and filled.box_row == 15
        and filled.list_row == 18 and filled.button_row == 21 and filled.width == 80
        and filled.button_h == 4, "24x80")
    local tall = geometry(40, 120)
    expect(tall.banner and tall.gap == 3 and tall.slots == 10 and tall.box_row == 18
        and tall.art_row == 2 and tall.list_row == 21 and tall.button_row == 35
        and tall.left == 16 and tall.art_left == 21 and tall.width == 90, "40x120")
    expect(tall.slots + 4 == Mark.height + 1 and tall.width == Mark.width + 10, "list is ten past the mark")
    local short = geometry(20, 80)
    expect(short.banner and short.gap == 0 and short.slots == 1 and short.list_row == 17
        and short.button_row == 19 and short.button_h == 2, "20x80")
    local narrow = geometry(30, 40)
    expect(not narrow.banner and narrow.gap == 3 and narrow.slots == 10 and narrow.art_row == 3
        and narrow.box_row == 7 and narrow.list_row == 10 and narrow.button_row == 24
        and narrow.left == 1 and narrow.width == 40, "30x40")
    local placed = Screen.buttons({ left = 1, width = 80, button_h = 4 }, {
        { label = "Explore Image", attr = "white", kind = "explore" },
        { label = "Launch Image", attr = "white", kind = "launch" },
    })
    expect(placed.frame == nil and placed[1].frame and placed[2].frame
        and placed[1].hint == "Space" and placed[2].hint == "Enter"
        and placed[1].x1 - placed[1].x + 1 == #placed[1].label + 2
        and placed[2].frame.x == placed[1].frame.x1 + 9
        and Screen.button_hint("back") == "Esc", "eight columns and side padding")
    local bare = Screen.buttons({ left = 1, width = 40, button_h = 2 }, {
        { label = "Explore Image", attr = "white", kind = "explore" },
        { label = "Launch Image", attr = "white", kind = "launch" } })
    expect(bare[1].frame == nil and bare[2].x == bare[1].x1 + 9
        and Geo.countdown_row(tall) == tall.list_row + tall.slots + 2
        and Geo.countdown_row(filled) == filled.button_row - 1
        and Geo.countdown_row(short) == nil, "gap and countdown row")
    local sample_line = Screen.line({ kind = "candidate", name = "QueueOS", version = "1.0.0", published = "2026-11-25T00:00:00Z", booted = "2026-11-25T13:14:00Z", tag = "" }, 1)
    expect(sample_line:find("QueueOS", 1, true) and sample_line:find("1.0.0", 1, true)
        and sample_line:find("2026-Nov-25 (Wed) 13:14", 1, true), "sample columns")
    local inner_left, inner_w = Screen.inner({ left = 1, width = 80 })
    expect(Screen.header():find("Last Successful Boot", 1, true)
        and not Screen.header():find("Tag", 1, true)
        and Screen.frame_top(Mark.width) == string.rep(" ", Mark.width)
        and inner_left == 3 and inner_w == 76, "header, spaces, indent")
    local quiet_line = Screen.line({ kind = "candidate", name = "stub", version = "0", published = "2026-11-25T00:00:00Z", booted = "", tag = "" }, 1)
    expect(quiet_line:find("stub", 1, true) and not quiet_line:find("(Wed)", 1, true), "stub stays blank")
    local parsed, perr = dofile(boot_dir() .. "/json-text.lua").parse('{"OS":"QueueOS","n":1}')
    expect(parsed ~= nil, perr or "JSON did not parse")
    local Json = dofile(boot_dir() .. "/json-text.lua")
    expect(Json.string(Json.field(parsed, "OS")) == "QueueOS", "OS field")
    expect(Json.number(Json.field(parsed, "n")) == 1, "number field")
    expect(Json.parse("{") == nil, "truncated JSON should fail")
    expect(Json.parse('"\\u0041"') == "A", "unicode escape")
    local pretty = Json.pretty(parsed)
    expect(pretty:find('"OS"', 1, true) ~= nil, "pretty keeps the key")
    expect(pretty:find("\n", 1, true) ~= nil, "pretty breaks lines")
    local opts = with_hash()
    local empty_blob = Read.pack(Read.EMPTY_JSON, Read.EMPTY_HASH)
    local empty_slot = Read.classify(empty_blob, opts)
    expect(empty_slot and empty_slot.kind == "empty", "empty slot")
    expect(empty_slot and not empty_slot.launchable, "empty is not launchable")
    local dawn_blob, dawn_json = generation("dawn", "2026-11-25T00:00:00Z")
    expect(dawn_blob ~= nil, "dawn slot")
    local dawn = Read.classify(dawn_blob, opts)
    expect(dawn and dawn.kind == "candidate" and dawn.launchable, "dawn launches")
    expect(dawn and dawn.base and dawn.base.file == "Base.efi", "file label")
    expect(dawn and dawn.base and dawn.base.offset == 8388608, "base offset")
    local stored = dawn and dawn.json_text
    local lines = dawn and table.concat(dawn.view_lines, "\n") or ""
    expect(lines:find('"name": "dawn"', 1, true) ~= nil, "explore shows the name")
    expect(dawn and dawn.json_text == stored, "explore does not rewrite the slot")
    expect(dawn and dawn.json_text == dawn_json, "stored JSON is the packed line")
    local bad_hash = Read.EMPTY_HASH:sub(1, 63) .. "0"
    if bad_hash == Read.EMPTY_HASH then
        bad_hash = Read.EMPTY_HASH:sub(1, 63) .. "1"
    end
    local unsafe_blob = Read.pack(dawn_json, bad_hash)
    local unsafe = Read.classify(unsafe_blob, opts)
    expect(unsafe and unsafe.kind == "unsafe" and not unsafe.launchable, "flipped hash is UNSAFE")
    expect(unsafe and unsafe.name == "dawn", "UNSAFE still shows the name")
    local garbage = Read.classify("not json\nsha256 " .. Read.EMPTY_HASH .. "\n", opts)
    expect(garbage and garbage.kind == "unreadable", "garbage is unreadable")
    local other_json = '{"OS":"Other"}'
    local other_hex = hash_cached(other_json)
    local foreign = Read.classify(Read.pack(other_json, other_hex), opts)
    expect(foreign and foreign.kind == "foreign", "other OS is not QueueOS")
    local no_base_json = '{"OS":"QueueOS","name":"bare","published":"2026-11-26T00:00:00Z"}'
    local bare = Read.classify(Read.pack(no_base_json, hash_cached(no_base_json)), opts)
    expect(bare and bare.kind == "candidate" and not bare.launchable, "no Base is not launchable")
    local arm_blob = generation("arm", "2026-11-27T00:00:00Z", "aarch64")
    local arm = Read.classify(arm_blob, opts)
    expect(arm and arm.kind == "candidate" and not arm.launchable, "other arch is not launchable")

    local older_blob = generation("older", "2026-11-24T00:00:00Z")
    local bytes = older_blob .. dawn_blob
    local rest = string.rep("\0", REGION - #bytes)
    local slots = Read.slots_from_bytes(bytes .. unsafe_blob .. rest, opts)
    expect(slots ~= nil, "slots from bytes")
    if slots then
        slots[3] = unsafe
        slots[4] = garbage
        slots[4].index = 4
        slots[5] = foreign
        slots[5].index = 5
        slots[6] = bare
        slots[6].index = 6
        slots[7] = arm
        slots[7].index = 7
        local bad_empty = Read.pack(Read.EMPTY_JSON, bad_hash)
        slots[8] = Read.classify(bad_empty, opts)
        slots[8].index = 8
        expect(slots[8].kind == "unsafe", "{} with a bad hash is UNSAFE")
        local no_line = Read.classify(no_base_json .. "\n", opts)
        slots[9] = no_line
        slots[9].index = 9
        expect(no_line.kind == "unsafe", "missing hash line is UNSAFE")
        expect(slots[10].kind == "unreadable", "zero slot is unreadable")
        expect(#slots == 10, "ten slots")
        local picked = Screen.initial(slots)
        expect(picked == 2, "newest launchable slot is selected, got " .. tostring(picked))
        expect(Read.initial(slots) == picked, "reader and screen agree")
        local state = Screen.new(slots, 1000)
        expect(state.selected == 2 and state.deadline == 1010
            and Screen.launch_label(state, 1000) == "Launch Image"
            and Screen.countdown_text(state, 1000) == "10", "dawn starts with 10 seconds")
        expect(not Screen.tick(state, 1009) and Screen.launch_seconds(state, 1000) == 10, "timer holds")
        Screen.explore(state)
        expect(state.mode == "explore" and state.deadline == nil and not Screen.tick(state, 5000), "Explore pauses")
        local view = Screen.explore_lines(state)
        Screen.scroll(state, 1, #view, 1)
        expect(state.json_scroll == 1, "Explore scrolls")
        local bar = Screen.scrollbar(30, 10, 0)
        local endbar = Screen.scrollbar(30, 10, 20)
        expect(bar[1].role == "up" and bar[2].role == "thumb" and bar[#bar].role == "down"
            and endbar[#endbar - 1].role == "thumb"
            and Screen.wheel_scroll(state, 1, 30, 10) and state.json_scroll == 4, "scrollbar and wheel")
        Screen.back(state, 3000)
        expect(state.mode == "list" and state.json_scroll == 0 and state.deadline == 3010, "Back rearms")
        local priority = Screen.order(slots)
        expect(priority[1] == 2 and priority[2] == 1 and priority[3] == 3, "rows follow boot priority")
        Screen.move(state, 1, 4000)
        expect(state.selected == 1 and state.deadline == 4010, "older row stays timed")
        Screen.move(state, 1, 4100)
        expect(state.selected == 3 and state.deadline == nil and not Screen.launch(state), "UNSAFE is not timed")
        Screen.select(state, 1, 5000)
        expect(not Screen.tick(state, 5009) and Screen.tick(state, 5010) and state.launched == 1, "timer launches slot 1")
        local report = Screen.report(state)
        expect(report:find("launch slot 1\n", 1, true) == 1
            and report:find("file Base.efi\n", 1, true)
            and report:find("offset 8388608\n", 1, true)
            and report:find("\nsha256 " .. BASE_HASH .. "\n", 1, true), "report names the base")
        local pair = { { launchable = true, published = "b", countdown = 10 }, { launchable = true, published = "a", countdown = 30 } }
        local image = Screen.new(pair, 1000)
        expect(image.selected == 1 and image.deadline == 1010, "first image counts from 10")
        Screen.select(image, 2, 2000)
        expect(image.deadline == 2030, "second image counts from 30")
        pair[2].countdown = 45
        Screen.activity(image, 3000)
        expect(image.deadline == 3030 and Screen.countdown_of({ countdown = 9 }) == nil, "cap 30, under 10 stays")

        local same_a = generation("a", "2026-11-25T00:00:00Z")
        local same_b = generation("b", "2026-11-25T00:00:00Z")
        local tied = Read.slots_from_bytes(same_a .. same_b .. string.rep("\0", REGION - #same_a - #same_b), opts)
        expect(tied and Screen.initial(tied) == 1, "equal timestamps keep the lower slot")

        local quiet = Screen.new(Read.slots_from_bytes(string.rep("\0", REGION), opts), 10)
        expect(quiet.selected == 1 and quiet.deadline == nil, "no launchable row, slot 1, no timer")
    end

    local empties = {}
    for i = 1, SLOT_COUNT do
        empties[i] = empty_blob
    end
    local ten = Read.slots_from_bytes(table.concat(empties), opts)
    expect(ten and Screen.initial(ten) == 1, "ten empty slots select slot 1")
    if ten then
        for i = 1, SLOT_COUNT do
            expect(ten[i].kind == "empty", "factory slot " .. tostring(i) .. " is empty")
        end
    end

    local raw_path = temp_path()
    local raw_out = io.open(raw_path, "wb")
    expect(raw_out ~= nil, "temporary raw image")
    if raw_out then
        raw_out:write(table.concat(empties))
        raw_out:close()
        local raw_slots, rerr = Read.slots_from_bytes(image_bytes(raw_path), opts)
        expect(rerr == nil and raw_slots and raw_slots[1].kind == "empty", rerr or "raw image")
        local qcow_path = temp_path()
        local _, cerr = run_capture("qemu-img convert -f raw -O qcow2 -- " .. quote(raw_path) .. " " .. quote(qcow_path))
        expect(cerr == nil, cerr or "qemu-img")
        local before = read_file(qcow_path)
        local converted, conv_err = image_bytes(qcow_path)
        local after = read_file(qcow_path)
        expect(conv_err == nil, conv_err or "qcow2 read")
        expect(before ~= nil and before == after, "qcow2 was not modified")
        if converted then
            local qslots = Read.slots_from_bytes(converted, opts)
            expect(qslots and qslots[1].kind == "empty" and qslots[10].kind == "empty", "qcow2 slots")
        end
    end

    if failures > 0 then
        return finish(1)
    end
    io.stdout:write("boot-tui check ok\n")
    return finish(0)
end

local function main()
    local command = arg[1]
    if command == "--help" or command == "-h" then
        io.stdout:write(USAGE)
        return 0
    end
    if command == "--check" then
        if arg[2] ~= nil then
            io.stderr:write(USAGE)
            return 1
        end
        return check()
    end
    if arg[2] ~= nil then
        io.stderr:write(USAGE)
        return 1
    end

    local slots, err
    if command == nil then
        local blobs = {}
        local empty_blob, perr = Read.pack(Read.EMPTY_JSON, Read.EMPTY_HASH)
        if empty_blob == nil then
            io.stderr:write("boot-tui: " .. perr .. "\n")
            return 1
        end
        for i = 1, SLOT_COUNT do
            blobs[i] = empty_blob
        end
        slots, err = Read.slots_from_bytes(table.concat(blobs), with_hash())
    else
        local data
        data, err = image_bytes(command)
        if data ~= nil then
            slots, err = Read.slots_from_bytes(data, with_hash())
        end
    end
    if slots == nil then
        io.stderr:write("boot-tui: " .. (err or "cannot read slots") .. "\n")
        return 1
    end
    return open_screen(slots)
end

os.exit(main() or 0)
