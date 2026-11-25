-- json-text.lua
--
-- Reads and pretty-prints one generation's JSON on the host.
-- The firmware Boot will hand this job to Jansson. The slot hash is of
-- the stored bytes, so a pretty view must not be written back.

local Json = {}

-- A table would collide with objects. This sentinel is the only null.
Json.null = setmetatable({}, { __tostring = function() return "null" end })

local MAX_DEPTH = 32

local function skip(text, i)
    while i <= #text do
        local c = text:sub(i, i)
        if c ~= " " and c ~= "\t" and c ~= "\n" and c ~= "\r" then
            break
        end
        i = i + 1
    end
    return i
end

local function at(text, i, pattern)
    return text:sub(i):match(pattern)
end

local function utf8_encode(cp)
    if cp < 0 then
        return nil
    elseif cp < 0x80 then
        return string.char(cp)
    elseif cp < 0x800 then
        return string.char(
            0xC0 + math.floor(cp / 0x40),
            0x80 + (cp % 0x40))
    elseif cp < 0x10000 then
        return string.char(
            0xE0 + math.floor(cp / 0x1000),
            0x80 + (math.floor(cp / 0x40) % 0x40),
            0x80 + (cp % 0x40))
    elseif cp < 0x110000 then
        return string.char(
            0xF0 + math.floor(cp / 0x40000),
            0x80 + (math.floor(cp / 0x1000) % 0x40),
            0x80 + (math.floor(cp / 0x40) % 0x40),
            0x80 + (cp % 0x40))
    end
    return nil
end

local function parse_string(text, i)
    local out = {}
    i = i + 1
    while i <= #text do
        local c = text:sub(i, i)
        local b = c:byte()
        if c == '"' then
            return table.concat(out), i + 1
        elseif c == "\\" then
            local e = text:sub(i + 1, i + 1)
            local simple = {
                ['"'] = '"',
                ["\\"] = "\\",
                ["/"] = "/",
                b = "\b",
                f = "\f",
                n = "\n",
                r = "\r",
                t = "\t",
            }
            if simple[e] then
                out[#out + 1] = simple[e]
                i = i + 2
            elseif e == "u" then
                local hex = text:sub(i + 2, i + 5)
                if not hex:match("^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$") then
                    return nil, "bad unicode escape"
                end
                local cp = tonumber(hex, 16)
                i = i + 6
                if cp >= 0xD800 and cp <= 0xDBFF then
                    if text:sub(i, i + 1) ~= "\\u" then
                        return nil, "lone surrogate"
                    end
                    local low_hex = text:sub(i + 2, i + 5)
                    if not low_hex:match("^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$") then
                        return nil, "bad unicode escape"
                    end
                    local low = tonumber(low_hex, 16)
                    if low < 0xDC00 or low > 0xDFFF then
                        return nil, "lone surrogate"
                    end
                    cp = 0x10000 + (cp - 0xD800) * 0x400 + (low - 0xDC00)
                    i = i + 6
                elseif cp >= 0xDC00 and cp <= 0xDFFF then
                    return nil, "lone surrogate"
                end
                local encoded = utf8_encode(cp)
                if not encoded then
                    return nil, "bad codepoint"
                end
                out[#out + 1] = encoded
            else
                return nil, "bad escape"
            end
        elseif b < 0x20 then
            return nil, "raw control in string"
        else
            out[#out + 1] = c
            i = i + 1
        end
    end
    return nil, "unterminated string"
end

local function parse_number(text, i)
    local start = i
    if text:sub(i, i) == "-" then
        i = i + 1
    end
    local int = at(text, i, "^[0-9]+")
    if not int then
        return nil, "bad number"
    end
    if int:sub(1, 1) == "0" and #int > 1 then
        return nil, "bad number"
    end
    i = i + #int
    if text:sub(i, i) == "." then
        local frac = at(text, i + 1, "^[0-9]+")
        if not frac then
            return nil, "bad number"
        end
        i = i + 1 + #frac
    end
    local exp_mark = text:sub(i, i)
    if exp_mark == "e" or exp_mark == "E" then
        local j = i + 1
        local sign = text:sub(j, j)
        if sign == "+" or sign == "-" then
            j = j + 1
        end
        local exp = at(text, j, "^[0-9]+")
        if not exp then
            return nil, "bad number"
        end
        i = j + #exp
    end
    local lex = text:sub(start, i - 1)
    local value = tonumber(lex)
    if value == nil then
        return nil, "bad number"
    end
    return { kind = "number", text = lex, value = value }, i
end

local parse_value

local function parse_array(text, i, depth)
    i = i + 1
    local items = {}
    i = skip(text, i)
    if text:sub(i, i) == "]" then
        return { kind = "array", items = items }, i + 1
    end
    while true do
        local value, next_i = parse_value(text, i, depth + 1)
        if type(next_i) ~= "number" then
            return nil, next_i or "expected a value"
        end
        if next_i <= i then
            return nil, "parser did not advance"
        end
        items[#items + 1] = value
        i = skip(text, next_i)
        local c = text:sub(i, i)
        if c == "]" then
            return { kind = "array", items = items }, i + 1
        end
        if c ~= "," then
            return nil, "expected a comma"
        end
        i = i + 1
    end
end

local function parse_object(text, i, depth)
    i = i + 1
    local pairs = {}
    i = skip(text, i)
    if text:sub(i, i) == "}" then
        return { kind = "object", pairs = pairs }, i + 1
    end
    while true do
        i = skip(text, i)
        if text:sub(i, i) ~= '"' then
            return nil, "expected a key"
        end
        local key, key_next = parse_string(text, i)
        if key == nil then
            return nil, key_next
        end
        i = skip(text, key_next)
        if text:sub(i, i) ~= ":" then
            return nil, "expected a colon"
        end
        local value, next_i = parse_value(text, i + 1, depth + 1)
        if type(next_i) ~= "number" then
            return nil, next_i or "expected a value"
        end
        pairs[#pairs + 1] = { key, value }
        i = skip(text, next_i)
        local c = text:sub(i, i)
        if c == "}" then
            return { kind = "object", pairs = pairs }, i + 1
        end
        if c ~= "," then
            return nil, "expected a comma"
        end
        i = i + 1
    end
end

parse_value = function(text, i, depth)
    if depth > MAX_DEPTH then
        return nil, "JSON is nested too deep"
    end
    i = skip(text, i)
    local c = text:sub(i, i)
    if c == "{" then
        return parse_object(text, i, depth)
    elseif c == "[" then
        return parse_array(text, i, depth)
    elseif c == '"' then
        return parse_string(text, i)
    elseif c == "-" or (c ~= "" and c:byte() >= 48 and c:byte() <= 57) then
        return parse_number(text, i)
    elseif text:sub(i, i + 3) == "true" then
        return true, i + 4
    elseif text:sub(i, i + 4) == "false" then
        return false, i + 5
    elseif text:sub(i, i + 3) == "null" then
        return Json.null, i + 4
    end
    return nil, "expected a value"
end

function Json.parse(text)
    if type(text) ~= "string" then
        return nil, "JSON text is not a string"
    end
    local value, next_i = parse_value(text, 1, 0)
    if type(next_i) ~= "number" then
        return nil, next_i or "expected a value"
    end
    next_i = skip(text, next_i)
    if next_i <= #text then
        return nil, "trailing junk"
    end
    return value
end

function Json.is_object(value)
    return type(value) == "table" and value.kind == "object"
end

function Json.is_array(value)
    return type(value) == "table" and value.kind == "array"
end

-- Last key wins, which is what a reader does when a key is repeated.
function Json.field(object, key)
    if not Json.is_object(object) then
        return nil
    end
    local found = nil
    for n = 1, #object.pairs do
        if object.pairs[n][1] == key then
            found = object.pairs[n][2]
        end
    end
    return found
end

function Json.string(value)
    if type(value) == "string" then
        return value
    end
    return nil
end

function Json.number(value)
    if type(value) == "table" and value.kind == "number" then
        return value.value
    end
    return nil
end

local function escape_string(s)
    local out = { '"' }
    for i = 1, #s do
        local c = s:sub(i, i)
        local b = c:byte()
        if c == '"' then
            out[#out + 1] = '\\"'
        elseif c == "\\" then
            out[#out + 1] = "\\\\"
        elseif c == "\n" then
            out[#out + 1] = "\\n"
        elseif c == "\r" then
            out[#out + 1] = "\\r"
        elseif c == "\t" then
            out[#out + 1] = "\\t"
        elseif b < 0x20 then
            out[#out + 1] = string.format("\\u%04x", b)
        else
            out[#out + 1] = c
        end
    end
    out[#out + 1] = '"'
    return table.concat(out)
end

local function pretty(value, indent, depth)
    if value == Json.null then
        return "null"
    end
    if type(value) == "boolean" then
        if value then
            return "true"
        end
        return "false"
    end
    if type(value) == "string" then
        return escape_string(value)
    end
    if type(value) == "table" and value.kind == "number" then
        return value.text
    end
    if Json.is_array(value) then
        if #value.items == 0 then
            return "[]"
        end
        local lines = { "[" }
        for n = 1, #value.items do
            local comma = ""
            if n < #value.items then
                comma = ","
            end
            local rendered = pretty(value.items[n], indent, depth + 1) .. comma
            lines[#lines + 1] = indent:rep(depth + 1) .. rendered
        end
        lines[#lines + 1] = indent:rep(depth) .. "]"
        return table.concat(lines, "\n")
    end
    if Json.is_object(value) then
        if #value.pairs == 0 then
            return "{}"
        end
        local lines = { "{" }
        for n = 1, #value.pairs do
            local comma = ""
            if n < #value.pairs then
                comma = ","
            end
            local pair = value.pairs[n]
            local rendered = escape_string(pair[1]) .. ": "
                .. pretty(pair[2], indent, depth + 1) .. comma
            lines[#lines + 1] = indent:rep(depth + 1) .. rendered
        end
        lines[#lines + 1] = indent:rep(depth) .. "}"
        return table.concat(lines, "\n")
    end
    return "null"
end

function Json.pretty(value)
    return pretty(value, "  ", 0)
end

return Json
