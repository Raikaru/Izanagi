# Guest memory and the trust boundary

Mods are trusted, opt-in code. Install only mods whose authors and contents you
trust. Library sandboxing and the checks below are defence in depth, **not a
security guarantee against hostile mods**: native game functions and native
asset parsers still consume mod-controlled game data. The native Mods screen
must show this warning before a player's first enablement. Native plugins have
a separate, explicitly unsafe opt-in; ordinary script/file installation never
grants permission to load a native library.

## Native admission policy

`include/p4pc/mod_memory.h` defines the shared native admission service. Read/
write/host-data span declarations are native-owner capabilities, not scripting
entry points. Script struct views may add **classification-only** function-word
leases: these narrow writes and never grant read/write access. Before any store,
the binding validates the complete proposed byte payload.
Refusal performs no partial write.
Stores and scratch initialization use the canonical EE byte-copy/fill wrappers,
so the one native byte-provenance observer sees successful writes. The last
scratch-buffer/view pin calls `p4pc_bytes_invalidate` before unmapping its guest
stack allocation; invalidation describes the still-valid span and performs no
store, hash or allocation. A denied write never notifies a successful mutation.

Writable guest ranges are actual allocated, writable ELF data/BSS sections,
the initialized retail guest heap arena, native-declared writable table spans,
and scratch allocations proven to belong to the calling mod. ELF evidence
comes from section flags and bounds contained in a segment the loader actually
loaded. Executable sections and readonly sections are denied even if another
writable declaration overlaps them. Missing/malformed section evidence never
widens the policy to all PS2 RAM: normal image boot remains unchanged, while
unowned retail-memory writes are refused.

The heap bounds query reads the host's initialized `heap_base`/`heap_end`, not
the mutable guest `sbrk` break. Kernel queries protect both a thread's declared
guest stack and its actual enlarged native coroutine stack, including dormant
reservations and contexts awaiting destruction. The debug heap's native-pointer
headers, tail guards and quarantined payloads are protected too. These queries
do not initialize or schedule the kernel, enable heap checking, allocate, poll
hardware or mutate guest memory. Stack/host-data exclusions apply to reads as
well as writes, so protected pages are not inspected through a read helper.

A native owner must declare host-data or readonly spans before publishing an
allocation to scripts. Such an overlap takes precedence over every writable
span, including the enclosing heap. In particular, table/text snapshots do not
become writable merely because their allocation resides in the guest heap.
The owner revokes its declaration before releasing or reusing the allocation.
Declarations retain no guest or host allocation on behalf of their owners.

## Function words

Retail image function-pointer cells come from the generated patch table;
native owners also declare function-word arrays explicitly. Any overlapping
write must replace each affected four-byte cell completely with NULL or a
known dispatch entry. A byte/string store cannot bypass that check. Validation
uses the actual stored representation: a registered retail entry/handle on
64-bit builds and a known native table target on i386. Retail-entry encoding
maps through the native table, never an unchecked integer-to-host-call cast.
Unknown entries and partial function-word writes are refused before the store.
Known function references carry the actual ABI's dispatch word. Encoding and
decoding require the native table's real retail entry; no unchecked integer is
cast into a host call. Script-created function-field leases are revoked with the
last pinned view, while the underlying native allocation keeps its own leases.
Script calls independently require a known generated typed function bridge.

Generic scalar memory does not infer a C schema for every integer word. Native
layouts must identify dynamically created function-pointer fields with the
function-word declaration capability; writing an arbitrary integer into a
previously undeclared integer field does not prove it is a function pointer.
Use declared struct views rather than pointer arithmetic when a layout is
known. This limitation is another reason these checks are not hostile-code
isolation or whole-program control-flow integrity.

## Evidence fixtures

- `mod_memory_policy` checks admission/denial precedence, overflow, readonly and
  native-owned declarations, complete known function words, partial writes and
  unavailable ELF evidence using explicit backend evidence fixtures.
- `heap_guest_span` links the production EE allocator and checks initialization,
  arena-bound stability across `sbrk`, and real debug-header/guard/quarantine
  exclusion. It does not alter the existing allocator/RNG parity fixtures.
- `kernel_threads` checks actual dormant declared-stack exclusion and unique
  native-context incarnations across thread restarts, without advancing the
  presentation counter.
- `image_guest_ranges` loads synthetic MIPS ELF bytes through the production
  loader, including actual code/data/BSS bounds, executable+writable flags and
  unavailable/malformed section evidence. It contains no retail bytes.

These are test definitions, not a claim that a platform run or the compiled-in
no-mod bit-identity proof has passed. Retain those execution receipts separately.
