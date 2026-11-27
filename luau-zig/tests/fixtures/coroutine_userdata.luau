local proxy = newproxy(true)
local mt = getmetatable(proxy)
local stored = 7
mt.__index = function(_, key) if key == 'value' then return stored end end
mt.__newindex = function(_, key, value) assert(key == 'value'); stored = value end
assert(type(proxy) == 'userdata' and proxy.value == 7)
local co = coroutine.create(function(v)
    assert(proxy.value == 7)
    proxy.value = v
    local sent = coroutine.yield(proxy.value, 'pause')
    proxy.value += sent
    return proxy.value
end)
local ok, value, text = coroutine.resume(co, 11)
assert(ok and value == 11 and text == 'pause' and coroutine.status(co) == 'suspended')
ok, value = coroutine.resume(co, 5)
assert(ok and value == 16 and proxy.value == 16 and coroutine.status(co) == 'dead')
assert(not coroutine.resume(co))
local good, err = pcall(function() error({tag = 42}) end)
assert(not good and type(err) == 'table' and err.tag == 42)
emit('userdata-coroutine:16')
