-- Consumer-visible Luau language semantics, independent expected values.
type Pair<T> = {first: T, second: T}
local p: Pair<number> = {first = 2, second = 5}
local x: number = if p.first < p.second then p.second else p.first
x += 3
x *= 2
assert(x == 16 and 7 // 2 == 3 and (-7) // 2 == -4)
local s = `value={x}`
assert(s == "value=16")
local total = 0
for i = 1, 8 do
    if i % 2 == 0 then continue end
    total += i
end
assert(total == 16)
local values = {3, 5, 8}
local sum = 0
for _, value in values do sum += value end
assert(sum == 16)
local function many(...)
    return select('#', ...), ...
end
local n, a, b, c = many(1, nil, 3)
assert(n == 3 and a == 1 and b == nil and c == 3)
local function make()
    local count = 0
    return function(v) count += v; return count end
end
local counter = make()
assert(counter(2) == 2 and counter(4) == 6)
local order = {}
local function mark(v) table.insert(order, v); return v end
local t = {}
t[mark('key')] = mark('value')
assert(#order == 2 and t.key == 'value')
emit('language:16:16:6')
