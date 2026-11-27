# L1 cohort 3A: fishing help-controller identity

This batch admits **`00635b38`:1 PROVEN /24 PS2 B**, runtime **pending-seam**. A now has **23 PROVEN /671 B**, **88 OPEN-retain /8,102 B**, and zero PROVEN-layout/D10a/b/c. Only this owned row, its new proof/document and the A summary changed; shared totals/prose and B rows did not.

The complete card is [l1-cohort-3A-fishing-help-proof.json](l1-cohort-3A-fishing-help-proof.json). It qualifies the full15B NUL-terminated `cmmFishingHELP` Steam value (`.sdata` RVA`91cff0`, raw`91bbf0`, SHA-256`26adb4af7ed987bbb45c553d1b388843d337bdc9e94f50fefee2ea5ec7fe5779`), corroborated at NSO`71005d5d2d`. Retain9 PS2 partition-tail bytes. Runtime input is the sealed Steam install, never the NSO or private cache.

## Structural proof and complete users

PS2 constructor`250120` and Golden`12e0c0` destroy an existing same-name HELP manager, allocate92->216B of owned work, install exactly three entry callbacks and register a priority15 child. Original **MIPS** confirms callback stores at`14`, `2c`, `44`. The old dispatcher`24efa0` independently confirms **24B stride and bound3**, not a guessed size from allocation/3. Golden dispatcher`1327e0` unrolls those three entries at**56B stride**, callback offsets`20`, `58`, `90`. Both test active bit0, call each active entry with manager base and entry pointer, clear bit0 on nonzero completion, and return0. The task name is therefore a corresponding controller identity, not cosmetic text.

All three identified old literal users have explicit `required_seams`:

- `24f080`: lookup and arm an entry, reset counters/state and select show/hide flags2/4.
- `24f790`: look up entry1, write its direction and mark prompt feedback.
- `250120`: replace and construct the complete three-entry manager.

The card records all51 materialized Golden name-reference PCs across constructor, new staged setup, UI callbacks and expanded fishing logic. They are positive users, not a no-reference argument. `12e780` is a new time-gated setup, **not** an asserted exact counterpart for the generic old arm API. Constructor parent lookup also differs: Golden uses primary `evtEvent` getter`13c7d0`; the old `285af0` could fall back to `evtEdit`. Do not invent that fallback in Golden mode.

Keep replacement, lookup, entry arming and prompt writes coherent across all translated/inlined/compiled-native copies. Do not copy216B native work over92B guest work or use56B native indices on24B guest records. Remaining work bytes are opaque, not asserted padding. Other fishing tasks, callback tables, assets and gameplay/timing algorithms are context only, not newly admitted or certified equivalent.

Executed static helper:

`"<git-install>/bin/bash.exe" -c 'export PATH=/usr/bin:/bin:$PATH; <python>/python.exe <scratch>/GoldenRE-notes/l1/l1c3A_fishing_help_proof.py'`

Observed:`QUALIFIED 1 24 ALL_PS2_DIRECT_USERS 3 GOLDEN_MATERIALIZATIONS 51; ORIGINAL_MIPS_3x24/GOLDEN_3x56`. Nine additional Romwright analyse-function/worker pairs returned0; original MIPS and full Golden dispatcher were inspected independently. The row installer preserved all unowned JSONL lines byte-for-byte; no archive-supplement duplicate exists.

No build, verify, objdiff, unittest, BBV, CI, game/emulator execution or `.arch` decoding was run. M6b must qualify the three reader seams and controller/task ownership before runtime closure.
