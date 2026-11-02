-- Trusted layout factory embedded in the executable, never a mod file.
-- Every field access crosses the native pin/bounds/policy boundary. A pin
-- retains scratch storage independently of the original buffer handle.
local pin, read_scalar, write_scalar, read_string, write_string, function_words = ...
local sizeof = p4.mem.sizeof
local layouts = setmetatable({}, {__mode = "k"})
local function integer(n, low, high, message)
    assert(type(n) == "number" and n >= low and n <= high and n == math.floor(n), message)
    return n
end
local function immutable()
    error("layout declarations are immutable")
end
local function description()
    return "p4.guest-view"
end
local function proxy(index, newindex, length)
    local value = newproxy(true)
    local mt = getmetatable(value)
    mt.__index, mt.__newindex, mt.__len = index, newindex, length
    mt.__tostring = description
    mt.__metatable = "protected guest view"
    table.freeze(mt)
    return value
end
return function(spec)
    assert(type(spec) == "table" and type(spec.name) == "string" and #spec.name > 0, "layout needs a name")
    local name = spec.name
    local size = integer(spec.size, 1, 1048576, "layout size must be 1..1048576")
    assert(type(spec.fields) == "table", "layout needs declared fields")
    local fields = {}
    for key, f in pairs(spec.fields) do
        assert(type(key) == "string" and key ~= "address" and key ~= "layout", "invalid layout field name")
        assert(type(f) == "table", "field needs a declaration")
        local offset = integer(f.offset, 0, size - 1, "field offset outside layout")
        local count = f.count == nil and 1 or integer(f.count, 1, 65536, "invalid field count")
        local width
        if layouts[f.type] then width = layouts[f.type].size
        elseif f.type == "string" then width = integer(f.capacity, 1, 1048576, "string field needs capacity")
        else width = sizeof(f.type) end
        assert(width * count <= size - offset, "field extends outside layout")
        fields[key] = {offset = offset, count = count, array = f.count ~= nil, width = width, type = f.type, readonly = not not f.readonly}
    end
    local layout
    local function read(f, owner, offset, readonly)
        if layouts[f.type] then return f.type:view(owner.address + offset, readonly or f.readonly) end
        if f.type == "string" then return read_string(owner, offset, f.width) end
        return read_scalar(owner, offset, f.type)
    end
    local function write(f, owner, offset, value, readonly)
        assert(not (readonly or f.readonly), "guest view is readonly")
        assert(not layouts[f.type], "assign nested fields individually")
        if f.type == "string" then write_string(owner, offset, f.width, value)
        else write_scalar(owner, offset, f.type, value) end
    end
    local function view(_, address, readonly)
        readonly = not not readonly
        local owner = pin(address, size, readonly)
        for _, f in pairs(fields) do
            if f.type == "fn" then function_words(owner, f.offset, f.count) end
        end
        local children
        return proxy(function(_, key)
            if key == "address" then return owner.address end
            if key == "layout" then return layout end
            local f = assert(fields[key], "undeclared guest field")
            if not f.array and not layouts[f.type] then return read(f, owner, f.offset, readonly) end
            if children and children[key] then return children[key] end
            if not children then children = {} end
            local child
            if not f.array then
                child = read(f, owner, f.offset, readonly)
            else
                child = proxy(function(_, index)
                    if index == "count" then return f.count end
                    if type(index) == "number" and index == math.floor(index) and (index < 1 or index > f.count) then return nil end
                    integer(index, 1, f.count, "invalid guest array index")
                    return read(f, owner, f.offset + (index - 1) * f.width, readonly)
                end, function(_, index, item)
                    integer(index, 1, f.count, "invalid guest array index")
                    write(f, owner, f.offset + (index - 1) * f.width, item, readonly)
                end, function() return f.count end)
            end
            children[key] = child
            return child
        end, function(_, key, item)
            local f = assert(fields[key], "undeclared guest field")
            assert(not f.array, "assign guest array elements individually")
            write(f, owner, f.offset, item, readonly)
        end)
    end
    layout = proxy(function(_, key)
        if key == "name" then return name end
        if key == "size" then return size end
        if key == "view" then return view end
        error("unknown layout member")
    end, immutable)
    layouts[layout] = {size = size}
    return layout
end
