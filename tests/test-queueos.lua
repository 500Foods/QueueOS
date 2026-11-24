#!/usr/bin/env lua
-- test-queueos.lua
--
-- Interactive catalogue of QueueOS tests. The shell alias tq points here.
--
-- No arguments, on a terminal, opens a full-screen list: each case, the
-- last run that recorded it, and that run's result. `all` is reserved for
-- running every case. This file does not launch QEMU. The harness in
-- tests/harness/run.py is a later step, and this catalogue will not invent
-- a command line for it.
--
-- The screen uses terminal.lua, the same library Hydrogen's SchemaHelper
-- uses. Lua 5.5 is the interpreter. The module is installed with luarocks
-- for that Lua, and this script asks luarocks for the path so the alias
-- does not have to export LUA_PATH.
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
  all              run every test
  --help           show this help

On a terminal the list is a full-screen TUI. q quits, r rescans,
up and down move, page up and page down move a page.
The alias tq runs this file.
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

local function run_all(tests)
    if #tests == 0 then
        io.stdout:write("No tests to run.\n")
        return 0
    end
    io.stderr:write("Cannot run tests yet. The harness is a later step.\n")
    return 1
end

local function stdout_is_tty()
    -- os.execute inherits this process's stdout, so test -t 1 sees the
    -- real terminal. io.popen would test the pipe instead. Do not redirect
    -- file descriptor 1: that would make the test look at /dev/null.
    local ok = os.execute("test -t 1")
    return ok == true
end

local function plain_list(tests)
    io.stdout:write("QueueOS tests\n\n")
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

local ATTR_HEAD = { fg = "yellow", brightness = "bright" }
local ATTR_ROW = { fg = "white" }
local ATTR_DIM = { fg = "white", brightness = "dim" }
local ATTR_SEL = { fg = "white", bg = "red", brightness = "bright" }
local ATTR_PASS = { fg = "green", brightness = "bright" }
local ATTR_FAIL = { fg = "red", brightness = "bright" }

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
        local attr = selected and ATTR_SEL or ATTR_ROW
        local line = cell(test.id, id_w) .. " " .. cell(test.last_run, run_w) .. " "
            .. cell(test.result, result_w) .. " " .. cell(test.title, title_w)
        local y = row + 1 + (index - first)
        write_at(t, y, col, fit(t, line, width), attr)
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
    local Bar = require("terminal.ui.panel.bar")
    local KeyBar = require("terminal.ui.panel.key_bar")
    local keys = t.input.keymap.default_keys
    local key_map = t.input.keymap.default_key_map

    local header = Bar {
        name = "header",
        auto_render = false,
        left = { text = "QueueOS", attr = { fg = "yellow", brightness = "bright" } },
        center = { text = count_text(app.tests), attr = { fg = "cyan", brightness = "bright" } },
        right = { text = "terminal.lua " .. tostring(t._VERSION), attr = { fg = "magenta" } },
    }
    local body = Panel {
        name = "body",
        border = {
            format = t.draw.box_fmt.single,
            attr = { fg = "red", brightness = "bright" },
            title = " QueueOS tests ",
            title_attr = { fg = "yellow", brightness = "bright" },
        },
        content = function(self)
            paint_tests(t, self, app)
        end,
    }
    local footer = KeyBar {
        name = "footer",
        auto_render = false,
        key_attr = { fg = "green", brightness = "bright" },
        desc_attr = { fg = "white" },
        items = {
            { key = "q", desc = "Quit" },
            { key = "r", desc = "Rescan" },
            { key = "up down", desc = "Move" },
            { key = "PgUp PgDn", desc = "Page" },
        },
    }
    local screen = Screen {
        header = header,
        body = body,
        footer = footer,
    }

    local function redraw()
        header:set_center(count_text(app.tests))
        screen:calculate_layout()
        screen:render()
    end

    t.cursor.visible.set(false)
    redraw()
    while true do
        local seq = t.input.readansi(0.2)
        if seq == nil then
            screen:check_resize(true)
        else
            local name = key_map[seq]
            local quit = seq == "q" or name == keys.escape or name == keys.ctrl_c
            if quit then
                break
            elseif seq == "r" then
                app.tests = discover(app.root)
                clamp(app)
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
    }
    app.tests = discover(app.root)
    clamp(app)

    if command == "all" then
        return run_all(app.tests)
    end
    if not stdout_is_tty() then
        plain_list(app.tests)
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
