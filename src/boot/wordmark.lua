-- wordmark.lua
--
-- The Boot mark. Q and OS are solid blocks. ueue is the lighter shade,
-- drawn white. The border is solid spaces, the same field as the inside.
-- A later firmware Boot uses the same picture; this file only holds the lines.

local Mark = {}

-- Each line is one row of the frame. Display width is one column per
-- character. Q and OS use U+2588. ueue uses U+2592.
Mark.lines = {
    "┌──────────────────────────────────────────────────────────────────────────────┐",
    "│                                                                              │",
    "│   ████████                                            ████████    ████████   │",
    "│  ███    ███                                          ███    ███  ███    ███  │",
    "│  ██      ██                                          ██      ██  ██      ██  │",
    "│  ██      ██                                          ██      ██   ███        │",
    "│  ██      ██  ▒▒    ▒▒   ▒▒▒▒▒▒   ▒▒    ▒▒   ▒▒▒▒▒▒   ██      ██    ██████    │",
    "│  ██      ██  ▒▒    ▒▒  ▒▒    ▒▒  ▒▒    ▒▒  ▒▒    ▒▒  ██      ██        ███   │",
    "│  ██   ██  █  ▒▒    ▒▒  ▒▒▒▒▒▒▒   ▒▒    ▒▒  ▒▒▒▒▒▒▒   ██      ██  ██      ██  │",
    "│  ███   ██    ▒▒    ▒▒  ▒▒        ▒▒    ▒▒  ▒▒        ███    ███  ███    ███  │",
    "│   ████  ██    ▒▒▒▒▒▒    ▒▒▒▒▒▒    ▒▒▒▒▒▒    ▒▒▒▒▒▒    ████████    ████████   │",
    "│                                                                              │",
    "└──────────────────────────────────────────────────────────────────────────────┘",
}

local SOLID = utf8.codepoint("█")
local SHADE = utf8.codepoint("▒")

local function columns(line)
    return utf8.len(line) or 0
end

-- The picture is stored with a box. The screen draws solid spaces there.
local function solid_border(line)
    local out = {}
    for _, cp in utf8.codes(line) do
        if cp == SOLID or cp == SHADE then
            out[#out + 1] = utf8.char(cp)
        else
            out[#out + 1] = " "
        end
    end
    return table.concat(out)
end

for i = 1, #Mark.lines do
    Mark.lines[i] = solid_border(Mark.lines[i])
end

Mark.height = #Mark.lines
Mark.width = columns(Mark.lines[1])

function Mark.runs(line)
    local out = {}
    local kind = nil
    local buf = nil
    local function flush()
        if buf == nil then
            return
        end
        out[#out + 1] = {
            kind = kind,
            text = table.concat(buf),
            cols = #buf,
        }
        buf = nil
    end
    for _, cp in utf8.codes(line) do
        local next_kind = "frame"
        if cp == SOLID then
            next_kind = "solid"
        elseif cp == SHADE then
            next_kind = "shade"
        end
        if next_kind ~= kind then
            flush()
            kind = next_kind
            buf = {}
        end
        buf[#buf + 1] = utf8.char(cp)
    end
    flush()
    return out
end

function Mark.consistent()
    if Mark.width < 1 then
        return false
    end
    for i = 1, #Mark.lines do
        if columns(Mark.lines[i]) ~= Mark.width then
            return false
        end
    end
    return true
end

return Mark
