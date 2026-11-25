#!/usr/bin/env lua
-- boot-tui.lua
--
-- Host stand-in for the Boot list. It reads ten generation slots from a
-- disk image and draws them. It does not start Base, and it does not
-- build a QEMU command. Boot.efi is still the ESP scaffold.
--
-- No arguments: ten empty slots, so the chrome can be tried before a
-- writer exists. A path is a database disk. qcow2 (magic QFI) is converted
-- to a temporary raw file, which is the byte layout Boot sees, and that
-- copy is deleted. images/ is not opened for writing.
--
-- The screen is one stack: the framed QueueOS mark, the ten slots, then
-- Explore and Launch. Spare rows and columns center that stack. A terminal
-- too small for the frame uses the one-line word and scrolls the slots.
-- Explore replaces the list with a scrollable view of that slot's JSON
-- and replaces the buttons with Back. The mouse selects a row.
-- A launchable row counts down from 10 when nothing is typed and the
-- pointer is still. Launch prints the chosen Base and returns.

local USAGE = [[
Usage: boot-tui.lua [image]

  (no arguments)   ten empty slots, for trying the screen
  image            a database disk, qcow2 or raw
  --check          read sample slots without a terminal
  --help           show this help

The framed QueueOS mark is centered at the top of the stack. The ten
slots sit in the middle, in a column the width of the mark. Explore and
Launch sit under the list. A larger terminal centers the whole stack.
Explore shows that slot's JSON. Back returns to the list. Launch prints
the chosen Base and does not start it.

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
    io.stdout:write("QueueOS\n\n")
    for i = 1, #slots do
        local mark = " "
        if i == selected then
            mark = ">"
        end
        io.stdout:write(mark .. " " .. slots[i].label .. "\n")
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
local ATTR_ROW = { fg = 252, bg = RED }
local ATTR_SEL = { fg = "white", bg = RED, brightness = "bright" }
local ATTR_HOVER = { fg = "white", bg = RED, underline = true }
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
        { "ueue", ATTR_SILVER },
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
        local x = geo.left
        local runs = Mark.runs(Mark.lines[i])
        for r = 1, #runs do
            local run = runs[r]
            local attr = ATTR_SILVER
            if run.kind == "solid" then
                attr = ATTR_WHITE
            end
            write_at(t, geo.art_row + i - 1, x, run.text, attr)
            x = x + run.cols
        end
    end
end

local function geometry(rows, cols)
    return Geo.place(rows, cols, Mark, SLOT_COUNT)
end

local function button_text(label)
    return "[ " .. label .. " ]"
end

local function write_buttons(t, app, geo, parts)
    local gap = 2
    local total = 0
    for i = 1, #parts do
        if i > 1 then
            total = total + gap
        end
        total = total + #parts[i].text
    end
    local x = geo.left
    if total <= geo.width then
        x = geo.left + math.floor((geo.width - total) / 2)
    end
    local ends = {}
    local limit = geo.left + geo.width - 1
    for i = 1, #parts do
        local part = parts[i]
        local end_col = x + #part.text - 1
        if end_col > limit then
            break
        end
        write_at(t, geo.button_row, x, part.text, part.attr)
        app.hotspots[#app.hotspots + 1] = {
            y = geo.button_row,
            x0 = x,
            x1 = end_col,
            kind = part.kind,
        }
        ends[part.kind] = end_col
        x = end_col + 1 + gap
    end
    return ends
end

local function paint(t, state, app, rows, cols, now)
    app.hotspots = {}
    local geo = geometry(rows, cols)
    app.geo = geo
    t.cursor.position.set(1, 1)
    t.text.attr({ bg = RED, fg = 252 })
    t.clear.screen()
    for row = 1, rows do
        write_at(t, row, 1, string.rep(" ", cols), ATTR_ROW)
    end
    if geo.banner then
        write_banner(t, geo)
    else
        write_word(t, geo.art_row, cols)
    end

    local top = geo.list_row
    local room = geo.slots
    if state.mode == "explore" then
        local lines = wrap_lines(Screen.explore_lines(state), geo.width)
        Screen.clamp_scroll(state, #lines, room)
        app.explore_lines = #lines
        app.explore_room = room
        local first = state.json_scroll + 1
        for i = 1, room do
            local y = top + i - 1
            if y >= 1 and y <= rows and y ~= geo.button_row then
                local line = lines[first + i - 1] or ""
                write_at(t, y, geo.left, fit(line, geo.width), ATTR_ROW)
            end
        end
        local attr = ATTR_WHITE
        if app.hover == "back" then
            attr = ATTR_HOVER
        end
        local ends = write_buttons(t, app, geo, {
            { text = button_text("Back"), attr = attr, kind = "back" },
        })
        if state.json_scroll > 0 or first + room - 1 < #lines then
            local hint = "scroll"
            local hint_col = geo.left + geo.width - #hint
            local back_end = ends.back or geo.left
            if hint_col >= back_end + 2 then
                write_at(t, geo.button_row, hint_col, hint, ATTR_DIM)
            end
        end
        return
    end

    local window = math.min(SLOT_COUNT, room)
    local scroll = app.list_scroll or 0
    if window >= SLOT_COUNT then
        scroll = 0
    else
        if state.selected <= scroll then
            scroll = state.selected - 1
        elseif state.selected > scroll + window then
            scroll = state.selected - window
        end
        local max_scroll = SLOT_COUNT - window
        if scroll < 0 then
            scroll = 0
        elseif scroll > max_scroll then
            scroll = max_scroll
        end
    end
    app.list_scroll = scroll
    app.list_window = window
    local right = geo.left + geo.width - 1
    for i = 1, window do
        local index = scroll + i
        local slot = state.slots[index]
        local y = top + i - 1
        if slot and y >= 1 and y <= rows and y ~= geo.button_row then
            local mark = " "
            if index == state.selected then
                mark = ">"
            end
            local attr = ATTR_ROW
            if index == state.selected then
                attr = ATTR_SEL
            elseif index == app.hover_slot then
                attr = ATTR_HOVER
            end
            write_at(t, y, geo.left, fit(mark .. " " .. slot.label, geo.width), attr)
            app.hotspots[#app.hotspots + 1] = {
                y = y, x0 = geo.left, x1 = right, kind = "slot", index = index,
            }
        end
    end

    local launch = "Launch"
    local left_seconds = Screen.launch_seconds(state, now)
    if left_seconds then
        launch = "Launch " .. tostring(left_seconds)
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
    write_buttons(t, app, geo, {
        { text = button_text("Explore"), attr = explore_attr, kind = "explore" },
        { text = button_text(launch), attr = launch_attr, kind = "launch" },
    })
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
                                Screen.scroll(state, -3, app.explore_lines or 1, app.explore_room or 1)
                            else
                                Screen.move(state, -1, now)
                            end
                        end
                    elseif event.wheel == 65 or event.wheel == 97 then
                        if not event.release then
                            if state.mode == "explore" then
                                Screen.scroll(state, 3, app.explore_lines or 1, app.explore_room or 1)
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
                        local explore_room = room
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
                    elseif seq == "e" then
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
                        Screen.select(state, 1, now)
                    elseif name == keys["end"] then
                        Screen.select(state, #state.slots, now)
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

local function generation(name, published, arch, role)
    local json = string.format(
        '{"OS":"QueueOS","name":"%s","version":"0.1","published":"%s","binaries":[{"role":"%s","arch":"%s","offset":8388608,"length":4096,"sha256":"%s","file":"Base.efi"}]}',
        name, published, role or "base", arch or "x86-64", BASE_HASH)
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

    expect(Mark.consistent(), "wordmark lines share one width")
    expect(Mark.height == 13 and Mark.width == 80, "wordmark is 13 rows by 80 columns")
    local picture = table.concat(Mark.lines, "\n")
    expect(picture:find("█", 1, true) ~= nil, "wordmark uses solid blocks")
    expect(picture:find("▒", 1, true) ~= nil, "wordmark uses shade blocks")
    local filled = geometry(24, 80)
    expect(filled.banner and filled.art_row == 1 and filled.list_row == 14, "24x80 puts the mark on top")
    expect(filled.button_row == 24 and filled.slots == 10 and filled.left == 1, "24x80 fills the screen")
    expect(filled.width == 80, "24x80 uses the mark width")
    local tall = geometry(40, 120)
    expect(tall.banner and tall.art_row == 9 and tall.list_row == 22, "extra rows center the stack")
    expect(tall.button_row == 32 and tall.slots == 10, "extra rows keep the buttons under the list")
    expect(tall.left == 21 and tall.width == 80, "extra columns center the column")
    local short = geometry(20, 80)
    expect(short.banner and short.slots == 6 and short.list_row == 14, "a short screen keeps the mark")
    expect(short.button_row == 20, "a short screen keeps the buttons on the last row")
    local narrow = geometry(30, 40)
    expect(not narrow.banner and narrow.art_row == 10 and narrow.list_row == 11, "a narrow screen centers the word")
    expect(narrow.button_row == 21 and narrow.slots == 10, "a narrow screen keeps ten slots")
    expect(narrow.left == 1 and narrow.width == 40, "a narrow screen uses the full width")

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
        expect(state.selected == 2, "screen starts on dawn")
        expect(state.deadline == 1010, "timer is 10 seconds")
        expect(Screen.tick(state, 1009) == false, "timer waits")
        expect(Screen.launch_seconds(state, 1000) == 10, "Launch shows 10")
        Screen.explore(state)
        expect(state.mode == "explore" and state.deadline == nil, "Explore pauses the timer")
        expect(Screen.tick(state, 5000) == false, "timer does not fire during Explore")
        local view = Screen.explore_lines(state)
        Screen.scroll(state, 1, #view, 1)
        expect(state.json_scroll == 1, "Explore scrolls")
        Screen.back(state, 3000)
        expect(state.mode == "list" and state.json_scroll == 0, "Back returns to the list")
        expect(state.deadline == 3010, "Back rearms the timer")
        Screen.move(state, 1, 4000)
        expect(state.selected == 3, "move lands on UNSAFE")
        expect(state.deadline == nil, "UNSAFE is not timed")
        expect(Screen.launch(state) == false, "UNSAFE does not launch")
        Screen.select(state, 1, 5000)
        expect(Screen.tick(state, 5009) == false, "older row waits")
        expect(Screen.tick(state, 5010) == true, "timer launches the selected row")
        expect(state.launched == 1, "launched the older row")
        local report = Screen.report(state)
        expect(report:find("launch slot 1\n", 1, true) == 1, "report names the slot")
        expect(report:find("file Base.efi\n", 1, true) ~= nil, "report names the file")
        expect(report:find("offset 8388608\n", 1, true) ~= nil, "report names the offset")
        expect(report:find("\nsha256 " .. BASE_HASH .. "\n", 1, true) ~= nil, "report names the hash")

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
