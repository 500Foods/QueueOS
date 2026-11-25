-- screen-geo.lua
--
-- Where the Boot stack sits. The mark, the slot window, and the button
-- row are one block. Spare rows pad above and below it. Spare columns
-- pad the sides when the framed mark fits. A terminal that cannot hold
-- the frame uses a one-line header the full width of the screen.

local Geo = {}

function Geo.place(rows, cols, mark, slot_count)
    if rows < 1 then
        rows = 1
    end
    if cols < 1 then
        cols = 1
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
        width = mark.width
    end
    local slots = slot_count
    local block = header + slots + 1
    local pad = 0
    if rows >= block then
        pad = math.floor((rows - block) / 2)
    else
        slots = math.max(1, rows - header - 1)
        block = header + slots + 1
        pad = 0
    end
    local left = 1
    if banner and cols > width then
        left = 1 + math.floor((cols - width) / 2)
    end
    local button_row = pad + 1 + header + slots
    if button_row > rows then
        button_row = rows
    end
    return {
        banner = banner,
        header = header,
        art_row = pad + 1,
        list_row = pad + 1 + header,
        button_row = button_row,
        slots = slots,
        left = left,
        width = width,
    }
end

return Geo
