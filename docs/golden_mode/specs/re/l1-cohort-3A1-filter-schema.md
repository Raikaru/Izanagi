# L1 cohort 3A1: complete type3 filter-extra constructor

Main explicitly admitted **`0063c660`:1 PROVEN /48 PS2 B** as a **PROVEN-code full constructor schema**, with mandatory **`golden-native-driver:evt-filter-params`**. It remains **pending-seam**. The old48B template partition stays intact for PS2 mode and **inactive in Golden mode**: no descriptor or record is physically written over it.

The complete card is [l1-cohort-3A1-filter-schema-proof.json](l1-cohort-3A1-filter-schema-proof.json). The sole identified old template reader is `28dc30`, including all translated/inlined/compiled-native aliases. This is the complete44B logical constructor, not merely the three visually familiar nonzero words.

## All eleven cells

Offsets below are hexadecimal; source record offsets are also hexadecimal.

| Output offset | Golden value/source | Type |
|---|---|---|
| `00` | record`2b` | u8 zero-extended to u32 |
| `04` | record`14` | s16 sign-extended to s32 |
| `08` | record`13` converted to f32, then divided by100.0f | f32 |
| `0c` | `65848484` | exact raw32 bits |
| `10` | `00000000` | exact raw32 bits |
| `14` | `3ba3d70a` | exact f32 bits |
| `18` | `3e051eb8` | exact f32 bits |
| `1c` | `00000000` | exact raw32 bits |
| `20` | `00000000` | exact raw32 bits |
| `24` | `00000000` | exact raw32 bits |
| `28` | record`16` | s16 sign-extended to s32 |

All7 fixed cells and4 record-derived cells are independently paired in ordinary Steam `208650` and NSO `147870`. Steam `.sdata` vector RVA`966980` contains the exact four words `3ba3d70a,3e051eb8,0,0`; the color and remaining zeros are code-authored. Source code/data bounds, hashes and individual instruction mappings are in the card.

Both platforms construct the same field offsets. NSO `1478b4` proves x21=sp+10, so its x21-relative stores map to the same44B buffer; treating them as sp-relative would falsely suggest a reordered layout. Steam's shuffles likewise need lane-aware interpretation, not a superficial stack-offset comparison.

The old template's four zero defaults are overwritten before use. Golden constructs these cells directly from typed record operands; **no Golden constant defaults or44B data span are invented**. `schema_sha256` hashes canonical schema JSON, not a nonexistent contiguous data object. There is no prefix-copy installation.

## Required binding

The mandatory native driver replaces the entire old copy-and-overwrite path atomically. Preserve exact fixed bits, signed16 extension and f32 conversion/division order, copied-filter type3, record`2a` reference association/kind4, and actual task/filter ownership. Other filter types and native common-parameter/work layouts are not declared compatible by this extra-buffer proof. Render behavior, interpolation and scheduling remain separate qualifications. Runtime uses authored schema constants and sealed Steam sources, never NSO/private caches.

Executed static helper:

`"<git-install>/bin/bash.exe" -c 'export PATH=/usr/bin:/bin:$PATH; <python>/python.exe <scratch>/GoldenRE-notes/l1/l1c3A1_filter_schema_proof.py'`

Observed:`QUALIFIED1/48B; FULL44_SCHEMA_FIXED7_INPUT4; PAIRED_STEAM_NSO; REQUIRED golden-native-driver:evt-filter-params`. Existing decompilation receipts were reused; full relevant ordinary instructions were inspected. No build, verify, objdiff, unittest, BBV, CI, game/emulator execution or `.arch` decoding was run. All foreign ledger lines were preserved byte-for-byte; no archive-supplement duplicate exists.
