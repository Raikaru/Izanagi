-- Matches the project's p4_luau sandbox, not the upstream CLI's global set.
assert(os == nil and io == nil and debug == nil and package == nil and require == nil)
assert(load == nil and loadstring == nil and loadfile == nil and dofile == nil)
assert(collectgarbage == nil and integer == nil and vector == nil and class == nil)
assert(type(pcall) == 'function' and type(getfenv) == 'function' and type(setfenv) == 'function')
assert(type(coroutine.create) == 'function' and type(buffer.create) == 'function')
assert(not pcall(function() table.insert = false end))
emit('sandbox:restricted:readonly')
