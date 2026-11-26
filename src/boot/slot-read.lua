-- slot-read.lua
--
-- Ten generation slots occupy the first 40 KiB of the database disk.
-- Slot i starts at byte (i - 1) * 4096. There is no filesystem.
--
-- A slot is one line of JSON, a newline, then a line
-- "sha256" and 64 lowercase hex digits, then zeros to 4096.
-- The hash covers the JSON bytes only, so key order is not part of it.
-- An empty slot is the two bytes {} and a hash that matches them.
--
-- empty:     {} and the hash matches.
-- candidate: JSON parses, OS is QueueOS, and the hash matches.
-- unsafe:    JSON parses and the hash is missing or wrong. Shown, not started.
-- foreign:   the hash matches and OS is not QueueOS.
-- unreadable: the bytes are not JSON.
--
-- A candidate is launchable when one binary has role "base" and the
-- architecture this Boot was built for. The host stand-in uses x86-64.
-- The "file" field is a label for Explore. It is not a path.
--
-- The hash function is injected. Firmware will hash in C. This file
-- does not call sha256sum and does not write a disk.

local function this_dir()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then
        src = src:sub(2)
    end
    return src:match("^(.*)/") or "."
end

local Json = dofile(this_dir() .. "/json-text.lua")

local Read = {}

Read.SLOT_BYTES = 4096
Read.SLOT_COUNT = 10
Read.REGION = 4096 * 10
Read.EMPTY_JSON = "{}"
-- SHA-256 of the two bytes {}. --check compares this with sha256sum.
Read.EMPTY_HASH = "44136fa355b3678a1146ad16f7e8649e94fb4fc21fe77e8310c060f61caaff8a"
Read.ARCH = "x86-64"

local function whole(n)
    if type(n) ~= "number" or n ~= n then
        return nil
    end
    if n < 0 or n > 2 ^ 53 or n ~= math.floor(n) then
        return nil
    end
    return n
end

local function hex64(text)
    if type(text) ~= "string" or #text ~= 64 then
        return nil
    end
    if not text:match("^[0-9a-f]+$") then
        return nil
    end
    return text
end

local function append_lines(lines, text)
    if text == "" then
        lines[#lines + 1] = ""
        return
    end
    local pos = 1
    while pos <= #text do
        local nl = text:find("\n", pos, true)
        if not nl then
            lines[#lines + 1] = text:sub(pos)
            return
        end
        lines[#lines + 1] = text:sub(pos, nl - 1)
        pos = nl + 1
    end
end

local function preview(blob)
    local shown = {}
    local n = math.min(#blob, 64)
    for i = 1, n do
        local b = blob:byte(i)
        if b == 0 then
            break
        elseif b >= 32 and b < 127 then
            shown[#shown + 1] = string.char(b)
        else
            shown[#shown + 1] = "."
        end
    end
    return table.concat(shown)
end

local function all_zero(blob)
    return blob:find("[^%z]") == nil
end

local function base_of(binaries, arch)
    for n = 1, #binaries do
        local bin = binaries[n]
        if bin.role == "base" and bin.arch == arch and bin.offset ~= nil
            and bin.length ~= nil and bin.sha256 ~= nil then
            return bin
        end
    end
    return nil
end

local function read_binaries(value)
    local bins = {}
    if not Json.is_array(value) then
        return bins
    end
    for n = 1, #value.items do
        local item = value.items[n]
        if Json.is_object(item) then
            local offset = whole(Json.number(Json.field(item, "offset")))
            local length = whole(Json.number(Json.field(item, "length")))
            if length ~= nil and length < 1 then
                length = nil
            end
            bins[#bins + 1] = {
                role = Json.string(Json.field(item, "role")),
                arch = Json.string(Json.field(item, "arch")),
                offset = offset,
                length = length,
                sha256 = hex64(Json.string(Json.field(item, "sha256"))),
                file = Json.string(Json.field(item, "file")),
            }
        end
    end
    return bins
end

local function label_for(slot)
    local detail
    if slot.kind == "empty" then
        detail = "empty"
    elseif slot.kind == "unreadable" then
        detail = "unreadable"
    elseif slot.kind == "unsafe" then
        if slot.name ~= "" then
            detail = "UNSAFE  " .. slot.name
        else
            detail = "UNSAFE"
        end
    elseif slot.kind == "foreign" then
        detail = "not QueueOS"
    else
        local bits = {}
        if slot.name ~= "" then
            bits[#bits + 1] = slot.name
        end
        if slot.version ~= "" then
            bits[#bits + 1] = slot.version
        end
        if slot.published ~= "" then
            bits[#bits + 1] = slot.published
        end
        if #bits == 0 then
            detail = "QueueOS"
        else
            detail = table.concat(bits, "  ")
        end
    end
    return string.format("%2d  %s", slot.index or 0, detail)
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

local function view_for(slot, value)
    local lines = {
        string.format("slot %d  %s", slot.index or 0, status_word(slot)),
    }
    if slot.kind == "unreadable" then
        lines[#lines + 1] = "This slot is not JSON."
        if all_zero(slot.bytes) then
            lines[#lines + 1] = "The bytes are zero."
        else
            local text = preview(slot.bytes)
            if text ~= "" then
                lines[#lines + 1] = text
            end
        end
        return lines
    end
    if slot.kind == "unsafe" then
        if slot.hash_stored == nil then
            lines[#lines + 1] = "There is no hash line."
        else
            lines[#lines + 1] = "The hash does not match."
            lines[#lines + 1] = "stored  " .. slot.hash_stored
            lines[#lines + 1] = "actual  " .. (slot.hash_actual or "")
        end
    end
    lines[#lines + 1] = ""
    append_lines(lines, Json.pretty(value))
    return lines
end

-- Builds a 4096-byte slot in memory. The image writer is a later piece.
function Read.pack(json_text, hash_hex)
    if type(json_text) ~= "string" then
        return nil, "slot JSON is not a string"
    end
    if json_text:find("\n", 1, true) then
        return nil, "slot JSON is one line"
    end
    if hex64(hash_hex) == nil then
        return nil, "hash is not 64 lowercase hex digits"
    end
    local text = json_text .. "\nsha256 " .. hash_hex .. "\n"
    if #text > Read.SLOT_BYTES then
        return nil, "slot does not fit in 4096 bytes"
    end
    return text .. string.rep("\0", Read.SLOT_BYTES - #text)
end

local function split_slot(blob)
    local nl = blob:find("\n", 1, true)
    if not nl then
        return nil, nil
    end
    local json_text = blob:sub(1, nl - 1)
    local rest = blob:sub(nl + 1)
    local nl2 = rest:find("\n", 1, true)
    local hash_line
    local tail = ""
    if nl2 then
        hash_line = rest:sub(1, nl2 - 1)
        tail = rest:sub(nl2 + 1)
    else
        hash_line = rest:gsub("%z+$", "")
    end
    return json_text, hash_line, tail
end

local function booted_of(tail)
    if type(tail) ~= "string" then
        return ""
    end
    local stamp = tail:match("^booted (%g+)\n")
    if stamp == nil or stamp:find(" ") then
        return ""
    end
    return stamp
end

function Read.classify(blob, opts)
    opts = opts or {}
    local arch = opts.arch or Read.ARCH
    if type(blob) ~= "string" then
        return nil, "slot is not a string"
    end
    if #blob < Read.SLOT_BYTES then
        blob = blob .. string.rep("\0", Read.SLOT_BYTES - #blob)
    elseif #blob > Read.SLOT_BYTES then
        blob = blob:sub(1, Read.SLOT_BYTES)
    end

    local slot = {
        index = opts.index or 0,
        kind = "unreadable",
        launchable = false,
        name = "",
        version = "",
        published = "",
        countdown = nil,
        booted = "",
        tag = "",
        os = nil,
        json_text = nil,
        hash_stored = nil,
        hash_actual = nil,
        binaries = {},
        base = nil,
        bytes = blob,
        view_lines = {},
    }

    local json_text, hash_line, tail = split_slot(blob)
    slot.booted = booted_of(tail)
    if json_text == nil then
        slot.label = label_for(slot)
        slot.view_lines = view_for(slot, nil)
        return slot
    end
    slot.json_text = json_text
    if type(hash_line) == "string" then
        slot.hash_stored = hash_line:match("^sha256 ([0-9a-f]+)$")
        if slot.hash_stored ~= nil and #slot.hash_stored ~= 64 then
            slot.hash_stored = nil
        end
    end

    local value, perr = Json.parse(json_text)
    if value == nil then
        slot.label = label_for(slot)
        slot.view_lines = view_for(slot, nil)
        slot.parse_error = perr
        return slot
    end
    if not Json.is_object(value) then
        slot.label = label_for(slot)
        slot.view_lines = view_for(slot, nil)
        return slot
    end

    if type(opts.hash) ~= "function" then
        return nil, "no hash function"
    end
    local actual, herr = opts.hash(json_text)
    if actual == nil then
        return nil, herr or "hash failed"
    end
    slot.hash_actual = actual

    slot.os = Json.string(Json.field(value, "OS"))
    slot.name = Json.string(Json.field(value, "name")) or ""
    slot.version = Json.string(Json.field(value, "version")) or ""
    slot.published = Json.string(Json.field(value, "published")) or ""
    slot.countdown = Json.number(Json.field(value, "countdown"))
    slot.tag = Json.string(Json.field(value, "tag")) or ""
    slot.binaries = read_binaries(Json.field(value, "binaries"))

    local matches = slot.hash_stored ~= nil and slot.hash_stored == actual
    if not matches then
        slot.kind = "unsafe"
    elseif json_text == Read.EMPTY_JSON then
        slot.kind = "empty"
    elseif slot.os == "QueueOS" then
        slot.kind = "candidate"
        slot.base = base_of(slot.binaries, arch)
        slot.launchable = slot.base ~= nil
    else
        slot.kind = "foreign"
    end

    slot.label = label_for(slot)
    slot.view_lines = view_for(slot, value)
    return slot
end

function Read.slots_from_bytes(bytes, opts)
    opts = opts or {}
    if type(bytes) ~= "string" then
        return nil, "disk bytes are not a string"
    end
    if #bytes < Read.REGION then
        bytes = bytes .. string.rep("\0", Read.REGION - #bytes)
    end
    local slots = {}
    for i = 1, Read.SLOT_COUNT do
        local start = (i - 1) * Read.SLOT_BYTES + 1
        local blob = bytes:sub(start, start + Read.SLOT_BYTES - 1)
        local one = {}
        for k, v in pairs(opts) do
            one[k] = v
        end
        one.index = i
        local slot, err = Read.classify(blob, one)
        if slot == nil then
            return nil, err
        end
        slots[i] = slot
    end
    return slots
end

function Read.initial(slots)
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

return Read
