-- Check both folded calls and calls made directly through pcall (the runtime library).
-- A zero significand must remain signed zero even for the largest integer exponents.
local huge = math.huge
assert(math.ldexp(0, 4096) == 0)
assert(1 / math.ldexp(0, 2147483647) == huge)
assert(1 / math.ldexp(-0, 4096) == -huge)
assert(1 / math.ldexp(-0, -2147483648) == -huge)
assert(math.ldexp(1, 4096) == huge)
assert(math.ldexp(1, -4096) == 0)
assert(math.ldexp(1, -1074) > 0)

for _, exponent in {0, 4096, -4096, 2147483647, -2147483648} do
    local ok, positive = pcall(math.ldexp, 0, exponent)
    assert(ok and positive == 0 and 1 / positive == huge)
    local negativeOk, negative = pcall(math.ldexp, -0, exponent)
    assert(negativeOk and negative == 0 and 1 / negative == -huge)
end

-- Argument expressions and validation must still occur before the zero shortcut.
local order = ''
local function argument(mark, value)
    order ..= mark
    return value
end
local ok, message = pcall(function()
    return math.ldexp(argument('x', 0), argument('e', 'not a number'))
end)
assert(not ok and string.find(message, 'number expected', 1, true) and order == 'xe')
return 'OK'
