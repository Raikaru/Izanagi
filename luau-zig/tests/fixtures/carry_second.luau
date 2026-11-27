local a, b, c = ...
assert(select('#', ...) == 3 and a == 9 and b == nil and c == 4)
shared_counter += a + c
assert(shared_counter == 23)
emit('carry:second:23')
