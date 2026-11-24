-- tq-mouse.lua
--
-- terminal.lua 0.1.0 has no mouse module. This is the same SGR 1006
-- tracking SchemaHelper uses. Motion (1003) highlights the row under
-- the pointer. Button events and the wheel select and scroll. The
-- caller turns tracking off before the terminal is restored, so the
-- user's shell is not left in mouse mode.

local M = {}

local ESC = "\27"

function M.enable(term)
    term.output.write(ESC .. "[?1003h" .. ESC .. "[?1006h")
    term.output.flush()
end

function M.disable(term)
    term.output.write(ESC .. "[?1003l" .. ESC .. "[?1006l")
    term.output.flush()
end

function M.is_mouse(raw)
    return type(raw) == "string" and raw:sub(1, 3) == ESC .. "[<"
end

-- SGR 1006: CSI < btn ; x ; y M  (press or motion) or m (release).
-- Wheel buttons are 64 and 65. Motion adds 32 to the button.
function M.parse(raw)
    local cb, x, y, tail = raw:match(ESC .. "%[<(%d+);(%d+);(%d+)([Mm])")
    if not cb then
        return nil
    end
    local btn = tonumber(cb)
    return {
        btn = btn,
        x = tonumber(x),
        y = tonumber(y),
        release = (tail == "m"),
        motion = btn >= 32 and btn < 64,
        wheel = (btn == 64 or btn == 65 or btn == 96 or btn == 97) and btn or nil,
    }
end

return M
