# L1 cohort 3A: field-object key-to-typed-selector schema

## Decision and mandatory driver

Main explicitly extended **PROVEN-code** to key-to-typed-selector schemas with paired Steam/NSO evidence. Golden has functional property replacements, so these are **PROVEN**, not D10c:

| PS2 key row | Key suffix | Golden selector | Steam accessor | NSO accessor |
|---|---|---|---|---|
| `005efd20` | Type | byte +4; count comes from metadata header | `4fa350` | `710038b5d0` |
| `005efd40` | Resid | placement identity u16 +0 | `4fa300` | `710038b5a0` |
| `005efd60` | Collis | flags u32 +0x60, bits 1 or 21 | `4fa590` | `710038b6f0` |
| `005efd80` | Id | asset identity u16 +6 | `4fa3b0` | `710038b600` |
| `005efe10` | Anim | mode u32 +8 | `4fa410` | `710038b630` |

Five new **PROVEN / 144 PS2 B** objects. Set A totals: **18 PROVEN / 527 B**, **93 OPEN-retain / 8,246 B**. PROVEN-layout and D10a/b/c remain zero. All are source-only **pending-seam**.

Every row requires **`golden-native-driver:field-object-metadata`**. The [proof](l1-cohort-3A-field-object-metadata-proof.json) contains the exact named-seam schema contract, code bounds/hashes, paired accessors and all PS2 consumers. Canonical seam objects carry this exact `id`; the proof's `required_seams` names the same driver. No byte/string-only binding is authorized.

## Complete accessed record schema

Both libraries obtain model metadata through carrier offset `f80`. Metadata has a u16 count at +0 and a record pointer at +8. Records have **112-byte stride** and the five typed fields above. Steam count accessor `4fa2d0` independently matches NSO `710038b580`.

Carrier offsets describe a **decoded provider-owned model**, not a serialized-file offset or guest RW clump. Unaccessed record bytes remain opaque. No host pointers, full decoded records or numeric selector descriptor may be written over the old ASCII key/guest struct. Runtime uses authored schema constants and a qualified Steam-backed provider, never NSO/private-cache input or code-byte reads.

**Resid and Id must not be swapped:** the misleading legacy names select placement identity at +0 versus filename/model asset identity at +6. Both are independently paired by width, offset and code. Collis must use the **u32** flags accessor `4fa590`; nearby `4fa610` returns only a byte and would lose bit 21.

## Active and retired consumers

### Active: `00151710` -> `71001ed2e0`

The old named-key clump traversal becomes typed metadata traversal. Golden skips records with **placement ID zero**, rather than the old asset-ID-zero gate. Asset ID zero must remain representable when placement identity is nonzero.

The Golden entry-flag operations are explicit:
- flags source bit 1 or 21 produces entry bit 1/value 2;
- animation mode exactly 1 produces entry bit 0/value 1;
- source flags bit 0 adds value 4; otherwise bit 1 adds value 8.

Do not OR the raw mode into old flags, retain the old type-0-only animation condition, truncate the shared flags word, or preserve the named lookup as a fallback. New model/effect kinds, asset decoding, handles and renderer behavior remain separate unqualified dependencies; this schema is not a claim of layout/behavior identity.

### Retired: `00151580` via `00177b30` / `71001f8c70` / `71001ee890`

The sole identified old caller is `Cmd_CreateFieldModel`. Its Golden counterpart retains the two script arguments, model-ready check, duplicate resource check (`2800`/`3ff`), field-model creation and returned resource identifier. Its placement-query helper is positively retired.

Actual NSO instructions at `1ee890` are **`mov x0,xzr; ret`**. It returns zero and performs **no flagsOut write**. This is ordinary code evidence, not a module label or no-reference conclusion.

The paired ordinary Steam handler `321130..32120a` removes the placement-query/transform branch after model registration and still compares the unwritten stack slot at `3211be`. **Do not invent flagsOut = 0.** The callee's no-write contract does not assert a deterministic value for arbitrary surrounding uninitialized stack bits; native/translated caller treatment remains pending qualification.

## Real Steam/NSO index difference

Both use 112-byte records but not the same high-index arithmetic:
- Steam: `movzx edi,dl`, then **`movsx rax,dil`**, so the low byte is sign-extended before multiplying 112.
- NSO: `index & 255`, zero-extended before multiplying 112.

They differ for indices 128..255. The runtime contract targets **Steam**. Neither accessor adds an index-versus-count guard. Do not silently use the NSO unsigned behavior, invent a clamp/default, or claim every shipped object table has fewer than 128 records without evidence.

## Exercised evidence

`<scratch>/GoldenRE-notes/l1/l1c3A_metadata_proof.py` reported `QUALIFIED 5 144`, with all five Steam field accessors, exact 112-byte indexing, typed loads and the retired helper bytes `e0031faac0035fd6`. Source schemas have explicit semantic JSON digests, not misleading data-span hashes. Every field has independent ordinary Steam and NSO code bounds/hashes.

The direct-call census did not identify Steam accessor callers; that result was **not** used as absence proof. Ordinary getter bodies establish field layout/ABI, positive NSO bulk code establishes active semantics under Main's ruling, and positive paired model-create bodies establish retirement. `.arch` was never decoded.

Canonical, archive-duplicate and triage rows are coherent; unowned/B lines were preserved byte-for-byte. No build, verify, objdiff, unit suite, BBV, CI or game/emulator execution was performed. Only M6b qualification of the mandatory native driver can close these runtime seams.
