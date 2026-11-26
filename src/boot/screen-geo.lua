-- screen-geo.lua
--
-- Where the Boot stack sits. The mark, a gap, the list, another gap,
-- and the buttons are one block. The list is wider than the mark when
-- the console has the columns. It holds a black border, the column
-- titles, one blank row, and the slot window. Spare rows pad above and
-- below the block. A terminal that cannot hold the block uses a
-- one-line header the full width of the screen.
--
-- The gaps are three rows when the console can spare them. The slot
-- window shrinks before a gap does. The middle of the gap under the
-- list holds the countdown, when a gap remains. The buttons are a red
-- border, the label, the key, and the border's bottom. The blank row
-- makes a full list one row taller than the mark.

local Geo = {}

local GAP = 3
local BUTTON_H = 4
-- Top border, titles, the blank row, and the bottom border.
local FRAME = 4
-- Ten columns past the mark, so a sha256 line fits without wrapping.
local LIST_EXTRA = 10

local function chrome(header, gap, button_h)
    return header + gap + FRAME + gap + button_h
end

function Geo.frame_rows(slots)
    return (slots or 1) + FRAME
end

function Geo.place(rows, cols, mark, slot_count)
    if rows < 1 then
        rows = 1
    end
    if cols < 1 then
        cols = 1
    end
    if slot_count == nil or slot_count < 1 then
        slot_count = 1
    end
    local banner = false
    local header = 1
    local width = cols
    local mark_ok = mark
        and mark.consistent
        and mark.consistent()
        and mark.width
        and mark.height
    if mark_ok and cols >= mark.width and rows >= mark.height + 2 then
        banner = true
        header = mark.height
        width = mark.width + LIST_EXTRA
        if width > cols then
            width = cols
        end
    end
    -- A mark that leaves no room for one slot and a label gives way
    -- to the one-line word. The red button border can shrink after that.
    if banner and rows < mark.height + FRAME + 1 + 2 then
        banner = false
        header = 1
        width = cols
    end
    local gap = GAP
    local button_h = BUTTON_H
    local slots = slot_count
    while rows < chrome(header, gap, button_h) + slots and slots > 1 do
        slots = slots - 1
    end
    while rows < chrome(header, gap, button_h) + slots and gap > 0 do
        gap = gap - 1
    end
    while rows < chrome(header, gap, button_h) + slots and button_h > 1 do
        button_h = button_h - 1
    end
    local block = chrome(header, gap, button_h) + slots
    local pad = 0
    if rows >= block then
        pad = math.floor((rows - block) / 2)
    else
        slots = 1
        gap = 0
        button_h = 1
        block = chrome(header, gap, button_h) + slots
        if rows >= block then
            pad = math.floor((rows - block) / 2)
        end
    end
    local left = 1
    if cols > width then
        left = 1 + math.floor((cols - width) / 2)
    end
    local art_left = left
    if banner then
        art_left = 1 + math.floor((cols - mark.width) / 2)
        if art_left < 1 then
            art_left = 1
        end
    end
    local art_row = pad + 1
    if art_row > rows then
        art_row = rows
    end
    local box_row = art_row + header + gap
    local head_row = box_row + 1
    local list_row = head_row + 2
    local button_row = list_row + slots + 1 + gap
    if button_row > rows then
        button_row = rows
    end
    if button_row < 1 then
        button_row = 1
    end
    return {
        banner = banner,
        header = header,
        art_row = art_row,
        art_left = art_left,
        box_row = box_row,
        head_row = head_row,
        list_row = list_row,
        button_row = button_row,
        button_h = button_h,
        gap = gap,
        slots = slots,
        left = left,
        width = width,
    }
end

-- Middle row of the gap under the list. Nil when that gap is gone.
function Geo.countdown_row(geo)
    local gap = geo.gap or 0
    local bottom

    if gap < 1 then
        return nil
    end
    bottom = (geo.list_row or 1) + (geo.slots or 1)
    return bottom + 1 + math.floor((gap - 1) / 2)
end

return Geo
