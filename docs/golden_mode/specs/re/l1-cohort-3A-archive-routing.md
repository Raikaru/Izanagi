# L1 cohort 3A: complete field archive routing

## Decisions

Two additional **PROVEN / 64 PS2 B** rows: `005efc60` and `005efc90`. Later A modules bring current totals to **18 PROVEN / 527 B**, **93 OPEN-retain / 8,246 B**; see the [scope summary](l1-cohort-3A-task-registration.md). PROVEN-layout/D10a/b/c remain zero. All admissions are source-only `pending-seam`. Set B and global summaries are untouched.

The [complete proof](l1-cohort-3A-archive-routing-proof.json) resolves the primary and secondary archive families together. The old 32-byte partitions remain intact for semantic routing; neither row authorizes a suffix-only filename patch.

## Independent reader correspondence

PS2 scene loader `00144f60` and Golden `71002a0d60` have the same resource-manager/low-three-flags ready shortcut and load-state progression: state 0 advances, state 1 calls primary request plus queue initialization using current field/room, then later states wait for part loading and create the scene task if absent. Golden lighting additions are not claimed equivalent.

The correct queue pairing is **PS2 `0014efc0` -> Golden `71001eb600`**, not the request-only wrapper `71001eb280`. Both initializers clear count/state and a 64-byte paired-key queue, use two field bands and 12-byte preset records, set bit 31 in the upper band, reverse-append selected minor IDs, or reuse loaded resources and set state 4. The Golden initializer calls the separate request wrapper in its queued and ordinary branches. Expanded handles and preset/index values are not declared byte-compatible.

## Complete source objects

Three individually hashed Steam `.sdata` strings supply the family:

| Role | RVA | Live bytes |
|---|---|---:|
| Primary `field/pack/fd%03d_%03d.arc` | `930bc8` | 27 |
| Ordinary `field/pack/f%03d_%03d.arc` | `930ba8` | 26 |
| Model `field/pack/f%03d_%03d_rmd.arc` | `930c20` | 30 |

Primary request `71001eb200` guards its pending handle, applies the full forward winter normalizer, formats the fd archive key and requests it. The preceding field/bank proof supplies all fourteen forward/inverse room mappings and actual calendar gates.

Secondary requests preserve separate ordinary slots 0/1/2 and a model slot. Fields 40..69 preload ordinary variants 0,1,2; fields above 59 in this band first subtract 20 from the local field. Other fields choose forward/inverse normalization through the actual date-range helper. Model requests use fifteen positive scene exclusions, an independent pending-handle guard, and the current mutated local coordinates—not a fresh copy of the original arguments.

### Correcting the worker's Boolean expression

Ordinary ARM64 instructions establish these exact excluded `(field,room)` pairs:

`(4,2), (14,2), (11,2), (8,4), (8,6), (15,1), (16,1), (16,6), (17,8), (17,9), (100,1), (100,2), (100,3), (248,2), (248,3)`.

The worker incorrectly nested its field-11/field-8 expression. Instructions `1eb438`/`1eb43c` independently test the field-8 mask and branch, positively proving rooms 4 and 6. The full branch spans/hashes and code-derived constants are recorded; no admission rests on the malformed inferred expression.

### Stateful coordinates and distinct getter domains

The ordinary block can mutate the local field/room before the model block. When the ordinary pending handle already exists, that mutation is skipped. Preserve this order rather than inventing a universal normalizer or retry.

Pure filename builders also differ: `1d76f0` selects the low-byte current variant for fields 40..69 without subtracting 20; `1d77a0` normalizes then subtracts 20/forces room 0 for fields 60..69. Callers must preserve the appropriate original/queued field domain. The proof records both builders separately.

Primary readiness treats a missing handle as ready. Secondary readiness returns ready when ordinary slot 0 is null; otherwise it checks that slot and each non-null additional slot. No unconditional extra wait or fallback was added.

## Runtime contract and exercised evidence

Golden mode must bind the complete named-key/provider route, scene/queue roles, calendar maps, preload variants, pending slots, exclusions, mutation order and readiness behavior at both PS2 readers and every native/translated/inlined copy. Guest ownership and version-aware payload decoding remain explicit seams. Do not copy host handles, widen PS2 structs, preserve old sector paths as Golden sources, or infer payload/physics equality from filenames.

Runtime inputs are sealed Steam strings plus authored **PROVEN-code** selectors. NSO 1.2 is static structural/code evidence only; no player NSO/private-cache dependency exists. Complete format-user inventories and limitations are in the receipt; no-reference results were not used as absence proof.

Private extractor `<scratch>/GoldenRE-notes/l1/l1c3A_archive_proof.py` reported `QUALIFIED 2 64`, three full format spans, fifteen exclusions and fourteen normalization pairs. Ordinary branch instructions override worker output where necessary. Static source assertions and exact A-only row preservation were exercised; no build, verify, objdiff, unit suite, BBV, CI or game/emulator execution was run.
