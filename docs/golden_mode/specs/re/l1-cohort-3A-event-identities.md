# L1 cohort 3A: event identities and BF source filenames

## Admission

This module batch admits **4 PROVEN /120 PS2 B**, all **pending-seam**. A now has **22 PROVEN /647 B**, **89 OPEN-retain /8,126 B**, and zero PROVEN-layout or D10a/b/c. These are A-only counts, not shared/global totals.

The complete machine-readable card is [l1-cohort-3A-event-identities-proof.json](l1-cohort-3A-event-identities-proof.json). These are three operational task identities and one complete script source format, **not diagnostics or arbitrary strings**. Their constructors, queries and request protocols establish the correspondence; identical text alone does not.

| PS2 object | Complete Golden value | Steam `.sdata` RVA | Live / retained PS2 tail | All identified PS2 readers |
|---|---|---|---:|---|
| `0063c2f8` | `evtEvent` | `92c838` | 9 /15 B | `2859e0`, `285af0` |
| `0063c5c0` | `event_data/script/E%03d_%03d%c.bf` | `92ccd8` | 34 /14 B | `289780` |
| `0063c648` | `evtFilter` | `92cd88` | 10 /14 B | `28d390`, `28dc30` |
| `0063c958` | `evtModelMotion` | `92ce30` | 15 /9 B | `293270`, `2932b0`, `2933a0`, `293550`, `293710` |

Every span includes its terminating NUL. The card carries exact raw bounds, span and image SHA-256, build10836033, independent NSO coordinates and ordinary-code envelopes. Runtime source is the sealed **Steam install**, never the NSO or private research cache.

## Complete reader families

### Primary event task and query namespace

PS2 `2859e0` and NSO `13c010` allocate/account/zero their entire primary-task work, register priority15 with paired update/destroy callbacks, associate an event-loader task, set major/minor IDs with third ID0, request load and initialize state0. Work grows96->112B and Golden creates a distinct `evtEditLoad` task. Those are structural deltas, not instructions to copy guest structs.

The second PS2 literal user `285af0` searches `evtEvent`, then calls `2853c0`, whose actual literal is **`evtEdit`**. It does **not** create a task, notwithstanding its old source comment. NSO `13c7d0` is the primary named getter; `13b290` is the editor getter. Higher-level `13cff0`/`13d040` positively search this namespace but add gates and return **booleans**, not task handles. Do not substitute their return domains for the PS2 wrapper. `13c830` (`evtEditLoad`) and `13c800` (loader work or -1) are different APIs, not asserted event-or-editor handle matches.

### Event BF label request

PS2 `289780` and NSO `144260` have the same action/status and frame gate, set event flag`0x80000`, format current major/minor IDs and command suffix+`0x41` into a256B filename buffer, lazily create the associated script loader, then request priority15 with the command mode. Raw NSO `1442bc`/`1442c0` confirm the **signed16** suffix operand at command+`14`; the extra mode comes from+`16`. No new suffix clamp/default or source-record layout identity is inferred. The complete format is admitted, not a guessed basename substitution or BF payload/VM guarantee.

### Both filter constructors

PS2 direct `28d390` and copied/reference-aware `28dc30` correspond to NSO `147700`/`147870`. Both decode event filter records, create the appropriate filter, allocate task work16->24B, store its handle/progression field, and register the same priority15 child/update/free protocol. The copied family handles1/2/3/5. Type3 independently preserves three rare44B-template anchors: `65848484`, `3ba3d70a`, `3e051eb8`, plus the percentage-normalized weight and reference selectors. The template at PS2 `63c660` is **context only**, not a newly promoted row.

The direct selector ABI differs: old selector0 creates type1, while Golden accepts1 for that role; both handle2. The card does not pretend these inputs or packed parameter buffers are identical. Golden shares record parser`147430` and uses64-bit filter handles.

### All five motion-manager users

The family includes named destroy, resource queue flush, speed, timed and explicit-placement constructors. Their NSO counterparts are `148c30`, `148c60`, `149080`, `1491f0`, `149370`. Constructors find/create one priority15 manager, use userdata4->8B and queued records76->88B, and copy a44B parameter packet. Speed/timed paths remove earlier motion3 records for the same16-bit resource ID. Placement accepts four optional settings and marks the record when present; it does not inherit that duplicate-removal rule. Flush always unlinks matching entries, but Golden adds motion-specific finalization, which is not declared behaviorally identical.

The worker omits a crucial task-work argument: raw `1490cc` loads manager userdata into **x6**, `1490d0` stores its owner and `1490ec` sets priority15. The old `63c930`/Golden `763a80` motion dispatch tables are context only; admitting this complete manager identity does not admit those separate tables or certify animation timing/interpolation.

## Binding, limits and exercised evidence

Install only each complete qualified prefix, retaining the exact PS2 partition tail. Every listed reader and translated/inlined/compiled-native copy must consume the same identity/source format. Registration, lookup, destroy, flush and queue/request operations must remain coherent. Preserve actual task/boolean domains, callback ownership, parent/priority, operand widths and context lifetime. Do not add fallback behavior, clamps, pretend zero writes, or copy enlarged native structs into guest work. All ten listed PS2 users have explicit `required_seams`; source proof is not runtime closure.

Executed static helper:

`"<git-install>/bin/bash.exe" -c 'export PATH=/usr/bin:/bin:$PATH; <python>/python.exe <scratch>/GoldenRE-notes/l1/l1c3A_event_proof.py'`

Observed: `QUALIFIED 4 120 ALL_PS2_DIRECT_USERS 10`, with A-membership, Steam/NSO complete strings, PS2 values, template words and pivotal ARM64 ABI assertions. Fifteen additional Romwright analyse-function/worker pairs returned0 for the documented event/query/loader roles; inferred prototypes were checked against ordinary instructions where material.

No build, verify, objdiff, unittest, BBV, CI, game/emulator execution or `.arch` decoding was performed. The archive supplement contains none of these four rows. All unowned canonical/triage lines were preserved byte-for-byte; no shared totals/prose or set-B row changed.
