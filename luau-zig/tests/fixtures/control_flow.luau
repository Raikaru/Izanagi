local effects = 0
local function bump() effects += 1; return true end
assert((false and bump()) == false and (true or bump()) == true and effects == 0)
local a, b = 1, 2
a, b = b, a
assert(a == 2 and b == 1)
local closure
local function scope()
    local x = 9
    closure = function(v) x += v; return x end
end
scope()
assert(closure(2) == 11 and closure(-1) == 10)
local out = {}
for i = 5, 1, -1 do
    if i == 3 then continue end
    table.insert(out, i)
end
local n = 0
repeat n += 1 until n == 4
assert(table.concat(out, ',') == '5,4,2,1' and n == 4)
local object = setmetatable({value = 5}, {
    __call = function(self, x) return self.value + x end,
    __tostring = function(self) return 'object:' .. self.value end,
})
assert(object(7) == 12 and tostring(object) == 'object:5')
local function tuple() return 'a', nil, 'c' end
local t = {tuple()}
assert(t[1] == 'a' and t[2] == nil and t[3] == 'c')
emit('control:10:5,4,2,1:12')
