# Raw Luau hooks and guest memory

[Advanced index](README.md) · [Luau reference](../lua.md)

## Typed hooks at retail addresses

```luau
-- Choose a researched address from this build's generated catalog.
local info = assert(p4.describe(address))
p4.log(info.signature)
local observer = p4.hook(info.address, info.signature, "after", function(ctx)
    p4.log(string.format("returned from %x", ctx.address))
end)
observer:disable()
observer:enable()
observer:remove()
```

`p4.describe(address)` returns nil for an unsupported address. The generated
`gen/mod_hooks.tsv` records supported signatures and exclusions; do not infer a
function name or ABI from an address spelling or stale decomp alias. Signatures
have a result letter followed by at most 12 parameter letters:

| Letter | Type |
| --- | --- |
| `v` | void result |
| `b`, `B` | signed/unsigned 8-bit |
| `h`, `H` | signed/unsigned 16-bit |
| `i`, `I` | signed/unsigned 32-bit |
| `q`, `Q` | signed/unsigned 64-bit, represented by an eight-byte LE buffer |
| `f`, `d` | float32/float64 |
| `p` | 32-bit guest data address; zero is NULL |

Variadics, by-value aggregates, unknown aliases, function-pointer declarators and
target-dependent scalar widths such as `size_t` and `long` are not guessed.
An incompatible signature fails before registration or native
invocation. Before hooks execute in global package order; after hooks execute in
reverse. A second active replacement is a reported conflict, not last-writer-wins.

```luau
local replacement = p4.hook(address, signature, "replace", function(ctx)
    ctx.args[1] = adjusted_argument
    ctx.next() -- original at most once; updates ctx.result
    ctx.result = adjusted_result
end)
local value = p4.call(address, signature, argument) -- original, bypasses root hooks
```

`ctx.next()` is valid only inside the replacement's current native coroutine and
callback. Retaining a context does not retain a live native stack frame; stale,
cross-coroutine and repeated continuations are refused. Recursive calls made by
the original still observe hooks. `p4.call` does not establish that a selected
operation is pure, legal at this lifecycle point, or free of RNG/cost/turn effects.
Never invoke a damage/accuracy calculator merely to draw a preview.

The generator instruments final mirrored definitions **after** overrides and ABI
corrections. Named calls, aliases and guest function pointers reach that same
entry, so a hook wraps a hand-translated override. Inactive entries perform only
a predictable enabled load/branch, without allocation, lookup, VM entry or argument
marshalling. Active typed bridges are separate. No executable patching, JIT or
platform-specific detour is involved.
An override's includes, defines, comments, types, data and helper functions may
precede its body: splicing leaves its `// FUN_` marker in front of the override's
own body (docs/porting.md), and the generator hooks the definition that marker
names, looking past includes and comments by that name, never by position, so a
helper is never hooked in its place. A conditional directive (`#if`/`#ifdef`/
`#else`/`#endif`) before the body records an `unsupported preamble` row instead of
guessing the active definition; old-style (K&R) definitions remain unhooked.
Bridge invokers honor the same forced CRT aliases as the mirrored unit: a
retail function such as EE `kill` reaches its `func_00421fb8` body, not a host
process API with the same spelling.
The table generator inspects the compiled bridge as a project archive, not a
host library; system-library names remain subject to the existing host guard.
The standalone i386/x64 task comparison links the real bridge and dispatch
objects, retaining only its reachable task hooks with linker section GC.

## Guest memory, function words and declared layouts

```luau
local scratch = p4.mem.alloc(256)
local a = scratch.address
p4.mem.write("u32", a, 123)
assert(p4.mem.read("u32", a) == 123)
local bits = p4.u64(0x89abcdef, 0x01234567)
p4.mem.write("u64", a + 8, bits)
local low, high = p4.split64(p4.mem.read("u64", a + 8))
```

Integer helpers are explicitly little-endian and permit unaligned accesses.
32-bit and narrower values are range-checked Luau numbers; 64-bit values are
exact eight-byte buffers, **not** doubles, signed host integers or decimal strings.
Use Luau's `buffer` operations for binary codecs. `f32/f64`, bounded strings and
byte buffers are also supported. String writes reject embedded NUL and require
capacity for the terminator. All stores validate the complete payload before
changing any bytes.

[Memory policy](../memory.md) describes the source-backed write allowlist, executable
and readonly exclusions, real stack/debug-header protection and function-word
classification. Ordinary writable spans require actual image/heap evidence; a
numeric address inside the old RAM window is not authorization. Function cells
accept only nil or a `p4.fn(address)` reference to a known native dispatch entry,
not arbitrary integers. Declared function cells cannot be partially overwritten
through a scalar/string/byte store.

Scratch allocations are zeroed, 1 byte..1 MiB each and at most 4 MiB per mod.
On translated guest models they use the real guarded guest arena; exhaustion is
an error, never a host-pointer fallback. `scratch:release()` revokes that handle.
A declared view holds its own lifetime pin, whereas a naked numeric address does
not retain memory. Other mods cannot use private scratch through memory helpers.

```luau
local Record = p4.mem.layout<<{flags: number, id: number, skills: P4GuestArray<number>}>>({
    name = "ResearchedRecord",
    size = 0x30,
    fields = {
        flags = {offset = 0, type = "u16"},
        id = {offset = 2, type = "u16"},
        skills = {offset = 0x0c, type = "u16", count = 8},
    },
})
local record = Record:view(guest_address, true) -- read-only
p4.log(tostring(record.skills[1]))
```

In strict Analysis, supply the view type explicitly as above: the layout
specification describes runtime fields but cannot infer the generic view type.

Layouts validate bounded field extents, fixed array counts, nested layouts and
`type="string", capacity=N` declarations. Read-only restrictions propagate into
arrays and nested views. Out-of-range array reads return nil; writes and unknown
fields are refused. Layouts pin authorized memory but do not invent native-object
lifetime or relocate fixed retail tables. Internal pin/read/write closures are
not public memory methods.
