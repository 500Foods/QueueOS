#!/usr/bin/env lua
-- test-queueos.lua
--
-- Interactive catalogue of QueueOS tests. The shell alias tq points here.
--
-- No arguments, on a terminal, opens a full-screen list: each case, the
-- last run that recorded it, and that run's result. `all` calls
-- tests/harness/run.sh on the first batch. The second batch is test-second.
-- This file still does not build a QEMU command of its own.
--
-- The screen uses terminal.lua, the same library Hydrogen's SchemaHelper
-- uses. Lua 5.5 is the interpreter. The module is installed with luarocks
-- for that Lua, and this script asks luarocks for the path so the alias
-- does not have to export LUA_PATH.
--
-- The background is red on every cell. The word QueueOS is silver, with
-- Q and OS in bold white. The version is major.minor.build from
-- cmake/CMakeLists-version.cmake and the git commit count. Mouse tracking
-- is SGR 1006, same as SchemaHelper, and it is turned off on the way out.
--
-- A case is a directory tests/cases/<id>/ that contains case.conf. The
-- directory name is the id, because results/<run>/<id>/ is keyed that way.
-- The last run is the highest run id that has a meta.txt for the case.
-- Run ids are the six-digit form 000-000 from plan 001.
--
-- Keep this file the list. Split it before it approaches 1000 lines.

local USAGE = [[
Usage: test-queueos.lua [all]

  (no arguments)   list tests and their last results
  all              run the first batch
  --help           show this help

On a terminal the list is a full-screen TUI. The background is red.
q quits, r rescans, up and down move, page up and page down move a page.
A click selects a row. The wheel scrolls. The alias tq runs this file.
]]

-- The catalogue lives at tests/test-queueos.lua. The repository root is
-- the parent of that tests directory, including when arg[0] is relative.
local function repo_root()
    local src = arg[0] or ""
    local dir = src:match("^(.*)/test%-queueos%.lua$")
    if not dir or dir == "" then
        return "."
    end
    if dir == "tests" then
        return "."
    end
    return dir:match("^(.*)/tests$") or "."
end

local function sh_quote(text)
    return "'" .. tostring(text):gsub("'", "'\\''") .. "'"
end

local function read_file(path)
    local handle = io.open(path, "r")
    if not handle then
        return nil
    end
    local text = handle:read("*a")
    handle:close()
    return text
end

local function conf_value(text, key)
    if not text then
        return ""
    end
    local pattern = "^" .. key .. "=(.*)$"
    for line in text:gmatch("[^\n]+") do
        local value = line:match(pattern)
        if value then
            return value
        end
    end
    return ""
end

-- GNU find, which this Fedora host has. A missing directory yields no names.
local function dir_names(path)
    local names = {}
    local handle = io.popen(
        "find " .. sh_quote(path) .. " -mindepth 1 -maxdepth 1 -type d -printf '%f\\n' 2>/dev/null",
        "r")
    if not handle then
        return names
    end
    for line in handle:lines() do
        if line ~= "" then
            names[#names + 1] = line
        end
    end
    handle:close()
    return names
end

-- 000-001 sorts before 001-000. Anything else sorts last.
local function run_number(run_id)
    local major, minor = tostring(run_id):match("^(%d%d%d)%-(%d%d%d)$")
    if not major then
        return -1
    end
    return tonumber(major) * 1000 + tonumber(minor)
end

-- Highest run id wins. A later suite that skipped this case leaves the
-- earlier result in place, which is the last time the case actually ran.
local function last_result(root, case_id)
    local best_id = ""
    local best_n = -1
    local best_result = ""
    for _, run_id in ipairs(dir_names(root .. "/results")) do
        local n = run_number(run_id)
        if n > best_n then
            local meta = read_file(root .. "/results/" .. run_id .. "/" .. case_id .. "/meta.txt")
            local result = conf_value(meta, "result")
            if meta and result ~= "" then
                best_n = n
                best_id = run_id
                best_result = result
            end
        end
    end
    if best_id == "" then
        return "none", "none"
    end
    return best_id, best_result
end

local function discover(root)
    local tests = {}
    local cases = root .. "/tests/cases"
    for _, id in ipairs(dir_names(cases)) do
        local text = read_file(cases .. "/" .. id .. "/case.conf")
        if text then
            local last_run, result = last_result(root, id)
            tests[#tests + 1] = {
                id = id,
                title = conf_value(text, "title"),
                last_run = last_run,
                result = result,
            }
        end
    end
    table.sort(tests, function(a, b)
        return a.id < b.id
    end)
    return tests
end

local function usage(stream)
    stream:write(USAGE)
end

local function run_all(root, tests)
    if #tests == 0 then
        io.stdout:write("No tests to run.\n")
        return 0
    end
    local harness = root .. "/tests/harness/run.sh"
    local handle = io.open(harness, "r")
    if not handle then
        io.stderr:write("Cannot run tests yet. The harness is a later step.\n")
        return 1
    end
    handle:close()
    local cmd = "bash " .. sh_quote(harness)
        .. " --root " .. sh_quote(root)
        .. " --build " .. sh_quote(root .. "/build")
        .. " --suite " .. sh_quote(root .. "/tests/suites/first-batch.list")
    if os.execute(cmd) == true then
        return 0
    end
    return 1
end

local function stdout_is_tty()
    -- os.execute inherits this process's stdout, so test -t 1 sees the
    -- real terminal. io.popen would test the pipe instead. Do not redirect
    -- file descriptor 1: that would make the test look at /dev/null.
    local ok = os.execute("test -t 1")
    return ok == true
end

-- major and minor are written in the CMake version file. The build
-- count is git rev-list --count HEAD, the same command CMake runs.
-- A missing git history leaves the third number at 0.
local function queueos_version(root)
    local text = read_file(root .. "/cmake/CMakeLists-version.cmake") or ""
    local major = tonumber(text:match("QUEUEOS_VERSION_MAJOR%s+(%d+)")) or 1
    local minor = tonumber(text:match("QUEUEOS_VERSION_MINOR%s+(%d+)")) or 0
    local build = 0
    local handle = io.popen(
        "git -C " .. sh_quote(root) .. " rev-list --count HEAD 2>/dev/null",
        "r")
    if handle then
        build = tonumber(handle:read("*l")) or 0
        handle:close()
    end
    return string.format("%d.%d.%d", major, minor, build)
end

local function plain_list(tests, version)
    io.stdout:write("QueueOS " .. version .. "\n\n")
    if #tests == 0 then
        io.stdout:write("No tests yet.\n")
        return
    end
    io.stdout:write(string.format("%-16s %-8s %-6s %s\n", "id", "last run", "result", "title"))
    for _, test in ipairs(tests) do
        io.stdout:write(string.format(
            "%-16s %-8s %-6s %s\n",
            test.id, test.last_run, test.result, test.title))
    end
end

-- terminal.lua lives in the user's luarocks tree for Lua 5.5. Ask the
-- tool rather than hard-coding a home directory. The TUI is the only
-- path that needs the module. `all` and a piped list still work without it.
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

local function count_text(tests)
    if #tests == 1 then
        return "1 test"
    end
    return string.format("%d tests", #tests)
end

local function clamp(app)
    if #app.tests == 0 then
        app.selected = 0
        app.scroll = 0
        return
    end
    if app.selected < 1 then
        app.selected = 1
    elseif app.selected > #app.tests then
        app.selected = #app.tests
    end
end

local function move(app, delta)
    if #app.tests == 0 then
        return
    end
    app.selected = app.selected + delta
    clamp(app)
end

local function reveal(app)
    local page = app.page_rows or 1
    if app.selected < 1 then
        app.scroll = 0
        return
    end
    if app.selected <= app.scroll then
        app.scroll = app.selected - 1
    elseif app.selected > app.scroll + page then
        app.scroll = app.selected - page
    end
    local max_scroll = math.max(0, #app.tests - page)
    if app.scroll > max_scroll then
        app.scroll = max_scroll
    end
    if app.scroll < 0 then
        app.scroll = 0
    end
end

-- Pad or trim to a column width using terminal.lua's width helpers, so a
-- wide character in a title cannot push the next column through the border.
local function fit(t, text, width)
    text = tostring(text or "")
    if width < 1 then
        return ""
    end
    local ok, shown = pcall(t.text.width.truncate_ellipsis, width, text, "right")
    if not ok or type(shown) ~= "string" then
        shown = text:sub(1, width)
    end
    local wide_ok, wide = pcall(t.text.width.utf8swidth, shown)
    local w = (wide_ok and type(wide) == "number") and wide or #shown
    if w < width then
        shown = shown .. string.rep(" ", width - w)
    end
    return shown
end

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

-- Every attribute carries the red background. Silver is the 256-color
-- gray 252. Bold white is the Q, the OS, the selection, and the column
-- titles. Green is the one extra colour, so a pass is visible on the red.
local RED = "red"
local ATTR_SILVER = { fg = 252, bg = RED }
local ATTR_WHITE = { fg = "white", bg = RED, brightness = "bright" }
local ATTR_HEAD = { fg = "white", bg = RED, brightness = "bright" }
local ATTR_ROW = { fg = 252, bg = RED }
local ATTR_DIM = { fg = 252, bg = RED }
local ATTR_SEL = { fg = "white", bg = RED, brightness = "bright" }
local ATTR_HOVER = { fg = "white", bg = RED, underline = true }
local ATTR_PASS = { fg = "green", bg = RED, brightness = "bright" }
local ATTR_FAIL = { fg = "white", bg = RED, brightness = "bright" }

local function result_attr(result, selected)
    if selected then
        return ATTR_SEL
    end
    if result == "pass" then
        return ATTR_PASS
    end
    if result == "fail" then
        return ATTR_FAIL
    end
    if result == "none" then
        return ATTR_DIM
    end
    return ATTR_ROW
end

local function write_wordmark(t, row, col)
    local parts = {
        { "Q", ATTR_WHITE },
        { "ueue", ATTR_SILVER },
        { "OS", ATTR_WHITE },
    }
    local x = col
    for i = 1, #parts do
        write_at(t, row, x, parts[i][1], parts[i][2])
        x = x + #parts[i][1]
    end
    return x
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

local function paint_header(t, self, app)
    -- Header paints first, so this is where the click map starts over.
    app.hotspots = {}
    local row = self.inner_row
    local col = self.inner_col
    local width = self.inner_width
    if width < 8 then
        return
    end
    local x = write_wordmark(t, row, col)
    local count = count_text(app.tests)
    local room = width - (x - col) - #count - 1
    if room > 1 then
        write_at(t, row, x, fit(t, " " .. app.version, room), ATTR_SILVER)
    end
    if #count < width then
        write_at(t, row, col + width - #count, count, ATTR_SILVER)
    end
end

local function paint_footer(t, self, app)
    local row = self.inner_row
    local col = self.inner_col
    local width = self.inner_width
    local items = {
        { "q", "Quit", true },
        { "r", "Rescan", true },
        { "up down", "Move", false },
        { "wheel", "Scroll", false },
    }
    local x = col
    for i = 1, #items do
        local key, desc, clickable = items[i][1], items[i][2], items[i][3]
        if x + #key + #desc + 3 >= col + width then
            break
        end
        write_at(t, row, x, key, ATTR_WHITE)
        if clickable then
            app.hotspots[#app.hotspots + 1] = {
                y = row, x0 = x, x1 = x + #key - 1, kind = "key", key = key,
            }
        end
        x = x + #key
        local label = " " .. desc .. "  "
        write_at(t, row, x, label, ATTR_SILVER)
        x = x + #label
    end
end

local function paint_tests(t, self, app)
    local row = self.inner_row
    local col = self.inner_col
    local height = self.inner_height
    local width = self.inner_width
    if height < 1 or width < 8 then
        return
    end

    -- id, last run, result, then whatever is left for the title.
    local id_w, run_w, result_w = 16, 8, 6
    local gaps = 3
    local title_w = width - id_w - run_w - result_w - gaps
    if title_w < 4 then
        title_w = 4
    end
    app.page_rows = math.max(1, height - 1)
    reveal(app)

    local function cell(text, w)
        return fit(t, text, w)
    end

    local header = cell("id", id_w) .. " " .. cell("last run", run_w) .. " "
        .. cell("result", result_w) .. " " .. cell("title", title_w)
    write_at(t, row, col, fit(t, header, width), ATTR_HEAD)

    if #app.tests == 0 then
        write_at(t, row + 1, col, fit(t, "No tests yet. A test is tests/cases/<id>/case.conf.", width), ATTR_DIM)
        return
    end

    local first = app.scroll + 1
    local last = math.min(#app.tests, app.scroll + app.page_rows)
    for index = first, last do
        local test = app.tests[index]
        local selected = index == app.selected
        local hover = index == app.hover
        local attr = selected and ATTR_SEL or (hover and ATTR_HOVER or ATTR_ROW)
        local line = cell(test.id, id_w) .. " " .. cell(test.last_run, run_w) .. " "
            .. cell(test.result, result_w) .. " " .. cell(test.title, title_w)
        local y = row + 1 + (index - first)
        write_at(t, y, col, fit(t, line, width), attr)
        app.hotspots[#app.hotspots + 1] = {
            y = y, x0 = col, x1 = col + width - 1, kind = "row", index = index,
        }
        -- The result word keeps its own colour when the row is not selected.
        if not selected then
            local result_col = col + id_w + 1 + run_w + 1
            write_at(t, y, result_col, cell(test.result, result_w), result_attr(test.result, false))
        end
    end
end

local function run_tui(app)
    local t = load_terminal()
    local Screen = require("terminal.ui.panel.screen")
    local Panel = require("terminal.ui.panel")
    local keys = t.input.keymap.default_keys
    local key_map = t.input.keymap.default_key_map
    local script_dir = (arg[0] or ""):match("^(.*)/test%-queueos%.lua$") or "tests"
    local Mouse = dofile(script_dir .. "/tq-mouse.lua")

    -- One red base on the attribute stack for the whole session. Cell
    -- clears write spaces, and those spaces keep this background.
    t.text.push({ bg = RED, fg = 252 })
    t.cursor.position.set(1, 1)
    t.clear.screen()

    local function rescan()
        app.tests = discover(app.root)
        clamp(app)
    end

    local body = Panel {
        name = "body",
        border = {
            format = t.draw.box_fmt.single,
            attr = { fg = 252, bg = RED },
            title = " tests ",
            title_attr = ATTR_WHITE,
        },
        content = function(self)
            paint_tests(t, self, app)
        end,
    }
    local header = Panel {
        name = "header",
        min_height = 1,
        max_height = 1,
        content = function(self)
            paint_header(t, self, app)
        end,
    }
    local footer = Panel {
        name = "footer",
        min_height = 1,
        max_height = 1,
        content = function(self)
            paint_footer(t, self, app)
        end,
    }
    local screen = Screen {
        header = header,
        body = body,
        footer = footer,
    }

    local function redraw()
        t.cursor.position.set(1, 1)
        t.text.attr({ bg = RED, fg = 252 })
        t.clear.screen()
        screen:calculate_layout()
        screen:render()
    end

    local function on_click(spot)
        if not spot then
            return false
        end
        if spot.kind == "row" then
            app.selected = spot.index
            return true
        end
        if spot.kind == "key" and spot.key == "r" then
            rescan()
            return true
        end
        return false
    end

    t.cursor.visible.set(false)
    redraw()
    Mouse.enable(t)
    local ok, err = xpcall(function()
        while true do
            local seq = t.input.readansi(0.2)
            if seq == nil then
                screen:check_resize(true)
            elseif Mouse.is_mouse(seq) then
                local event = Mouse.parse(seq)
                if event and (event.wheel == 64 or event.wheel == 96) then
                    if not event.release then
                        move(app, -1)
                        redraw()
                    end
                elseif event and (event.wheel == 65 or event.wheel == 97) then
                    if not event.release then
                        move(app, 1)
                        redraw()
                    end
                elseif event and event.motion then
                    local spot = hotspot_at(app, event.x, event.y)
                    local hover = (spot and spot.kind == "row") and spot.index or 0
                    if hover ~= app.hover then
                        app.hover = hover
                        redraw()
                    end
                elseif event and event.release and event.btn == 0 then
                    local spot = hotspot_at(app, event.x, event.y)
                    if spot and spot.kind == "key" and spot.key == "q" then
                        break
                    end
                    if on_click(spot) then
                        redraw()
                    end
                end
            else
                local name = key_map[seq]
                local quit = seq == "q" or name == keys.escape or name == keys.ctrl_c
                if quit then
                    break
                elseif seq == "r" then
                    rescan()
                    redraw()
                elseif name == keys.up or seq == "k" then
                    move(app, -1)
                    redraw()
                elseif name == keys.down or seq == "j" then
                    move(app, 1)
                    redraw()
                elseif name == keys.pageup then
                    move(app, -(app.page_rows or 1))
                    redraw()
                elseif name == keys.pagedown then
                    move(app, app.page_rows or 1)
                    redraw()
                elseif name == keys.home then
                    app.selected = #app.tests > 0 and 1 or 0
                    redraw()
                elseif name == keys["end"] then
                    app.selected = #app.tests
                    redraw()
                end
            end
        end
    end, debug.traceback)
    Mouse.disable(t)
    if not ok then
        error(err, 0)
    end
end

local function main()
    local command = arg[1]
    if command == "--help" or command == "-h" then
        usage(io.stdout)
        return 0
    end
    if command ~= nil and command ~= "all" then
        usage(io.stderr)
        return 1
    end
    if arg[2] ~= nil then
        usage(io.stderr)
        return 1
    end

    local app = {
        root = repo_root(),
        tests = {},
        selected = 1,
        scroll = 0,
        page_rows = 1,
        hover = 0,
        hotspots = {},
        version = "",
    }
    app.version = queueos_version(app.root)
    app.tests = discover(app.root)
    clamp(app)

    if command == "all" then
        return run_all(app.root, app.tests)
    end
    if not stdout_is_tty() then
        plain_list(app.tests, app.version)
        return 0
    end

    -- initwrap returns a function. Calling it initializes the terminal, runs
    -- the screen, and restores the terminal on the way out, including after
    -- an error. disable_sigint lets q and Ctrl-C take the same quit path.
    local t = load_terminal()
    local wrapped = t.initwrap(function()
        run_tui(app)
    end, {
        displaybackup = true,
        filehandle = io.stdout,
        skip_width_detection = true,
        disable_sigint = true,
        autotermrestore = true,
    })
    wrapped()
    return 0
end

os.exit(main() or 0)
