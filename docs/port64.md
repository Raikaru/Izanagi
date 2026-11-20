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
