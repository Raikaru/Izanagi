# M8f primitive31: battle-specific callback ownership

Primitive31 is not an ordinary particle subtype or universal sprite. This page specifies its ordinary SDK lifecycle/clock. [The concrete subtype continuation](m8f-battleonly-subtypes.md) now resolves the actual battle registration/table, primary schemas, transition envelopes and R_BLOCK animated-light algorithm. Remaining facade/backend items are explicit there.

## Sources and measured R_BLOCK dependency

Ordinary functions34F790(allocate/copy primary),34F890(construct),34F980(destroy),34F9E0(clone),34FAF0(reset),34FB50(update),34FBC0(draw),34FC20(combined update/draw). Primary/adjunct getters363660/363690 resolve either an allocated native pointer or an offset relative to the retained descriptor source; zero means absent. Do not apply those native getter layouts to the2C-byte wire descriptor.

R_BLOCK has a directly measured version200/primitive31/subtype6 descriptor. Primary40 decimal bytes, SHA-256`8fb8cad4bfe88437e1ef01f254f2539f889ac4cabeaaa2d3aa908219c8df9c94`; adjunct kind3,1648 decimal bytes, SHA-256`135fb6fc4a272f4d2fd9cfb642366191a1599ff35701c14aea65f1363eaa26c4`, beginning`OMG.00.1PSP`. Thus even this common witness requires a real battle-specific **model adjunct** path. A generic particle implementation, a nominal primitive31 node with no callbacks or merely waiting60 frames is not complete support.

## Callback table and native instance

The native context-active predicate is held at71051C8AE0 and callback-table pointer at71051C8AE8. These are process globals, not file words or legal wire addresses. Table stride38, subtype range0..C checked by the allocator:

| Table offset | Established role |
|---|---|
| 00 | Instance initialization/reset callback(instance) |
| 08 | Child/resource construction callback(primary,adjunctKind,adjunctBuffer,adjunctBytes) |
| 10 | Child/resource destruction callback(childHandle) |
| 18 | Child/resource clone callback(sourceInstance), when supplied |
| 20 | Update callback(instance) |
| 28 | Draw callback(instance) |
| 30 | u32 primary-copy byte count |

The table itself and its active predicate must be supplied by the real battle owner. On-file primitive subtype is **not** a function address. An unavailable required callback implementation is unsupported, not a fabricated valid handle.

34F790 allocates50+table.primaryBytes, copies exactly that many primary bytes into the trailing owned area and records subtype at native instance30. Native instance00 is a position quad with W1;10 is identity quaternion;20 scalar scale1;24 packed color80808080;28 floating clock0;2C byte flag0;38 child handle;48 pointer to the copied primary at50. Native sizes/offsets are provenance only: a portable typed owner need not recreate this ABI. Preserve the subtype-specific neutral packed color; do not globally rename it RGBAwhite or apply one normalization rule to every effect family.

34F890 resolves primary and adjunct from the descriptor, allocates/copies the primary, then, while battle context is active, invokes table08 if present, stores its returned full-width child handle at38, and invokes table00 if present. A copied primary with no active child is not evidence that its model/filter has rendered.

## Reset, time and draw separation

Reset34FAF0, only while the battle predicate is active: invoke table00 if present, then set floating clock0 and byte flag0.

Update34FB50, only while active: invoke table20 if present, then add0.5*liveFrameStep to this instance's clock. Draw34FBC0 independently checks active context and invokes table28 if present; it **does not advance** the clock. Combined34FC20 performs those phases in that order, with a separate active-context predicate check before each. Do not call combined update/draw in addition to the ordinary separate phases, or advance this clock when merely redrawing a frozen frame.

## Clone and destruction

34F9E0 always allocates a distinct instance and primary copy. If table18 exists, clone the source instance's child through that callback, install the new child handle, then initialize through table00. Otherwise allocate/copy the primary, require active context before invoking table08 with(primary,0,0,0), then initialize through table00. Immutable backing may be shared only through actual child/resource ownership; byte-copying the live child handle does not implement clone semantics.

34F980, while active and table10 exists, releases the child handle first; it then frees the enclosing instance allocation. Portable owner contract: clear all battle-specific children while the battle callback owner is still valid, before retiring its context/table and source/model generations. Do not release callback code/context first and then suppress child destruction because the active predicate becamefalse. This ordering is part of the common49-handle teardown contract, not a separate R_BLOCK cache.

## Remaining source work

The registered table and primary schemas are resolved in [the subtype continuation](m8f-battleonly-subtypes.md). R_BLOCK's subtype6 GMO drives a helper/light matrix, not a mandatory visible model mesh. Remaining closure is the exact unit action/rest-motion and camera facade semantics plus renderer binding of unit/scene colors and primitive-specific backends. This does not authorize lifting `M8f-res:battle-common-14A8-1490`; runtime qualification belongs to implementation.
