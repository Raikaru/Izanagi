# The 64-bit port model

Izanagi's supported players use 64-bit host pointers, while the original game
uses 32-bit guest addresses. Port code must keep these representations distinct:
never assume `sizeof(void *) == 4` or store a host pointer in guest memory.

## Guest addresses and pointers

Guest memory is a mapped block addressed by 32-bit guest values. Guest address
zero represents a null pointer; reserved high values represent pointer sentinels.
Game records and serialized data retain their 32-bit layouts. Host pointers are
used for native access, and conversions occur at the boundary between guest
storage and native code. See `include/p4pc/guest.h` for `P4_G2H` and `P4_H2G`.

`P4_G2H` is an inline range check and an add of the block base. Its slow path (`p4_g2h_slow`: null, the executable image and hardware windows, sentinels)
and `p4_h2g_slow` are cold but sit behind about 46,000 inline sites in the Android build, so they are declared with `P4_GUEST_SLOW_CC`
(`preserve_most` where the compiler has it: arm64 and x86-64 Clang) and the range check carries a likely hint. A call with the default convention forces every
value that is live across it into a callee-saved register or a spill, which a rewritten function with hundreds of guest reads pays in its prologue; with
`preserve_most` the callee keeps every register but the argument and result ones, so the fast path stays in caller-saved registers. Default-convention callers
(the Zig battle simulator declares both functions `extern`) are safe, since the callee preserves a superset; a test stub that defines them must carry the
same attribute.

Function pointers stored in guest memory are guest code addresses, not host
pointers. Use `P4_FN_G2H` and `P4_FN_H2G` to convert them. Host-only objects,
including graphics, audio and allocator objects, must not be stored as raw host
pointers in guest memory. New interfaces that exchange guest addresses should
use `u32` and document that contract.

## Compiling the decompilation

The build generates a mirror of the pinned `decomp/` sources and rewrites its
preprocessed C for 64-bit hosts. The rewriter and per-unit driver are under
`tools/port64/`; the original submodule is not edited. Overrides must work with
both host pointer widths. Follow the guest conversion rules in
[CONTRIBUTING.md](../CONTRIBUTING.md) when adding or changing port code.

## Tests

Host tests registered through the test factory also receive 64-bit variants.
Write fixtures using `tests/guest_test.h` and avoid casting retail addresses
directly to host pointers. See [BUILDING.md](../BUILDING.md) for configuration
and test commands, and [porting](porting.md) for mirror and symbol details.
