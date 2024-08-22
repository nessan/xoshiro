--[[
Quarto extension to allow us to replace e.g. {{< var site >}} with the simpler {site}.
SPDX-FileCopyrightText:  2024 Nessan Fitzmaurice <nessan.fitzmaurice@me.com>
SPDX-License-Identifier: MIT
--]]

---------------------------------------------------------------------------------------------------
-- Utility methods ...
---------------------------------------------------------------------------------------------------

local stringify = pandoc.utils.stringify

-- Checks whether a string is empty.
local function is_empty(str)
    return str == nil or str == ''
end

-- From the Quarto source ...
-- Gets the line number where a function was called from.
local function caller_info(offset)
    offset = offset or 3
    local caller = debug.getinfo(offset, "lS")
    return caller.source:sub(2,-1) .. ":" .. tostring(caller.currentline)
end

-- From the Quarto source ...
-- Writes a warning message decorated with the line number it was called from.
local function warning(message, offset)
    io.stderr:write("WARNING (" .. caller_info(offset) .. ") " .. message .. "\n")
end

-- Escapes all characters in a string so that it can be used literally in a regex.
-- The character `%´ works as an escape for the various "magic" characters in a lua regex.
-- For example, in a regex '.' matches any character but '%.' matches a literal dot.
-- The escape `%´ can also be used for all ordinary characters so '%x' is just 'x'.
-- To ensure a string is interpreted as a literal pattern can just add those escapes everywhere.
-- See: http://www.lua.org/pil/20.2.html
local function literal(str)
    return str:gsub("([^%w])", "%%%1")
end

-- Splits a string on a separator.
-- The default separator is a space " "
local function split(str, sep, allow_empty)
    sep = sep or " "
    local fields = {}
    local pattern
    if allow_empty == true then
        pattern = string.format("([^%s]*)", literal(sep))
    else
        pattern = string.format("([^%s]+)", literal(sep))
    end
    local _ignored = string.gsub(str, pattern, function(c) fields[#fields + 1] = c end)
    return fields
end

-- Encode a string to hex.
-- See: https://stackoverflow.com/questions/65476909/lua-string-to-hex-and-hex-to-string-formulas
local function hexencode(str)
    return (str:gsub(".", function(char) return string.format("%02X", char:byte()) end))
end

-- Decode a string from hex.
-- See: https://stackoverflow.com/questions/65476909/lua-string-to-hex-and-hex-to-string-formulas
local function hexdecode(hex)
    return (hex:gsub("%X%X", function(digits) return string.char(tonumber(digits, 16)) end))
end

---------------------------------------------------------------------------------------------------
-- Functions that set parameters which effect how this filter works.
---------------------------------------------------------------------------------------------------
-- Quarto stores the values it reads from the  priject's '_variables.yml' file under this metadata key.
-- NOTE: This value could change as we're using an undocumented feature of Quarto.
local quarto_key = "_quarto-vars"

-- We keep our own handle to the Quarto variables table -- it gets set in the `init_filter(meta)` function.
local vars = {}

-- Variables are recognized in markup as a name surrounded by curly braces e.g. `{name}`.
local var_prefix = '{'
local var_suffix = '}'

-- Set a pattern we can use to capture a variable name.
local function set_var_pattern()
    -- The var_prefix/var_suffix may have "magic" characters that we need to escape so we escape all their characters!
    local key_pattern =  literal(var_prefix) .. '(.-)' .. literal(var_suffix)

    -- The full pattern we use allows for text before and after the key itself.
    -- For example, "({name})" or "{name}..." or "text-{name}" etc.
    return '(.*)' .. key_pattern .. '(.*)'
end

-- Set the pattern used to identify keys for replacement in markup along with surrounding text.
local var_pattern = set_var_pattern()

-- Initialize the filter by grabbing the variables table from Quarto.
local function init_filter(meta)

    -- Grab the variables table from Quarto
    vars = meta[quarto_key]
    -- quarto.log.output(vars)

    -- Issue a warning if the variable table is empty.
    if not vars then
        warning("Failed to find any variable definitions under the key: '" .. quarto_key .. "'")
    end
end

---------------------------------------------------------------------------------------------------
-- Functions used to turn variable names into variable values.
---------------------------------------------------------------------------------------------------

-- Returns the replacement string for a `name` from our vars table or nil if `name` is not a var.
local function var(name)
    local keys = split(name, ".")
    local value = nil
    for _, key in ipairs(keys) do
        if value == nil then
            value = vars[key]
        else
            key = tonumber(key) or key
            value = value[key]
        end
        -- No value found -- stop trying
        if value == nil then break end
    end
    return value
end

-- Handle variable references in Pandoc Str objects
local function process_Str(el)
    -- Check for a variable reference and capture any surrounding text/punctuation etc.
    local b, key, e = el.text:match(var_pattern)
    -- Did we get a hit?
    if key then
        -- Is that key really a variable reference?
        local val = var(key)
        if val then
            local retval = pandoc.Inlines(val)
            if not is_empty(b) then retval:insert(1, b) end
            if not is_empty(e) then retval:insert(e) end
            return retval
        end
    end
    return el
end

-- If `url` is a variable defined as `url: "https://google.com"`
-- In markup we might have a reference to `[Google]({url})`
-- Pandoc turns that target into "%7Bhttps://google.com%7D" (i.e. hexes the braces and adds '%'s like HTML)
-- This means we need to pattern match on the hex version of our braced pattern
local hex_prefix = '%' .. hexencode(var_prefix)
local hex_suffix = '%' .. hexencode(var_suffix)
local html_var_pattern = literal(hex_prefix) .. '(.-)' .. literal(hex_suffix)

-- Handle variable references in Pandoc Link objects
local function process_Link(el)
    -- See if we get a hit on the hex encoded var pattern.
    local name = el.target:match(html_var_pattern)
    if name then
        -- Have something that might be a var so check whether it is.
        val = var(name)
        if val then el.target = stringify(val) end
    end
    return el
end
---------------------------------------------------------------------------------------------------
-- Invoke the various filter components in the appropriate order.
-- 1. Configure the filter iteslf.
-- 2. Replace any variable with its value.
---------------------------------------------------------------------------------------------------
return  {
    traverse = 'topdown',
    Meta = init_filter,
    Str = process_Str,
    Link = process_Link
}
