# Game CRT boundary

[Platform index](../platform.md) · [Numeric determinism](numeric_determinism.md) · [Port compilation](../porting.md)

Game logic must not depend on host CRT differences in ordering, integer widths,
rounding, locale or hidden state. This boundary covers generated decomp units,
overrides, game-facing native code and executable address tables.

## Binding inventory

`include/p4pc/libc_rename.h` defines the generated callers' bindings. Per-build
`gen/crt-<profile>.tsv` and JSON files record compiled callers, including
address-taken references. Entry-object inventories and final link maps provide
additional executable-specific evidence. The checked-in
[caller inventory](crt_callers.tsv) summarizes the reviewed profiles.

## Fail-closed build guard

The `p4crt` target uses `tools/crt_guard.py` to inspect compiled game objects and
relocations. It rejects unknown externals, CRT rename bypasses, missing retail
implementations and unreviewed host adapter/caller pairs. Executable link checks
also cover generated tables and entry objects. Generation-only census reports
are not substitutes for object or link checks.

## Indirect callback ABI boundary

Function pointer declarations must match the actual argument and return types.
An indirect cast can hide a host-width or calling-convention mismatch, so review
the caller and callee rather than assuming a cast changes the ABI.

## Allocator boundary and checked headers

The guest allocator uses the retail four-byte chunk and bin words on every host.
RenderWare's checked allocation wrappers retain their tracking header and
backpointer; the mapped-RAM sbrk and interrupt primitives are OS boundaries.
The normal allocation path does not use host-sized guest metadata.

## Inactive software movie decoder

Production movie playback uses the native SFD demuxer and FFmpeg. The manifest
[`config/inactive_movie_decoder.json`](../../config/inactive_movie_decoder.json)
identifies generated decoder sources and registrations removed from the mirror.
The generator checks its expected inputs rather than editing the decomp submodule.
