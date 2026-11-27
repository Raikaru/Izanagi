# L1 cohort 3A: task registration/identity sources

## Owned scope and decisions

Main split the previous 188 OPEN rows at the module/loader boundary recorded in `<scratch>/GoldenRE-L1-c3-SPLIT.md`. Set A owns **111 rows / 8,773 PS2 B**; set B and subsequent Main transfer amendments are managed separately in that split receipt. Previously published cohort-3 proofs remain read-only shared context, not additional A ownership.

The task batch establishes **7 PROVEN / 192 B**. The [archive follow-on](l1-cohort-3A-archive-routing.md) adds **2 / 64 B**, the [dungeon preload family](l1-cohort-3A-dungeon-preload.md) adds **4 / 127 B**, the [typed property schema](l1-cohort-3A-field-object-metadata.md) adds **5 / 144 B**, the [complete event identity/source families](l1-cohort-3A-event-identities.md) add **4 / 120 B**, and the [complete fishing help-controller identity](l1-cohort-3A-fishing-help.md) adds **1 / 24 B**, giving A **23 PROVEN / 671 B** and **88 OPEN-retain / 8,102 B**. PROVEN-layout and D10a/b/c remain zero. All twenty-three rows are **pending-seam**, not runtime closure. No set-B row or combined/global summary was changed.

The complete source receipt is [l1-cohort-3A-task-registration-proof.json](l1-cohort-3A-task-registration-proof.json).

| PS2 row | Partition | Complete Golden name | Live / retained tail |
|---|---:|---|---:|
| `0063f398` | 24 B | `MsgAssist` | 10 / 14 B |
| `005e46e8` | 24 B | `initDataRead` | 13 / 11 B |
| `005eff48` | 24 B | `field root` | 11 / 13 B |
| `005f1590` | 32 B | `enemy pop scheduler` | 20 / 12 B |
| `005f1610` | 32 B | `collision update` | 17 / 15 B |
| `005f1630` | 32 B | `collision controler` | 20 / 12 B |
| `005f1b28` | 24 B | `party ai` | 9 / 15 B |

These are registration/lookup identities, **not diagnostics, coefficients or mutable task state**. Identical text is not the proof: the corresponding constructor work and callback protocols establish each role.

## Structural correspondence

- **Assist:** PS2 manager work is 28-byte prefix plus eleven 12-byte slots; Golden is 40-byte prefix plus eleven 16-byte slots. Both have a second pending-list allocation, entries identity, flags bit 0, the same two invalid window handles and enabled bytes, and a priority-15 queue processor. The callback activates queued messages and drains/releases all eleven slots. Ordinary Steam constructor `32f510` and startup inline copy independently corroborate both list allocations, fields, sentinels, task callback and priority.
- **Boot chain:** 12-byte PS2 work becomes 16 bytes with a widened child handle. Both use priority 256 and the state-0/1/2 indexed child-task protocol, waiting for child state 3 before advancing. Golden expands its stage list and audio initialization; neither those stage counts nor bootstrap content are declared equivalent. Named lookup/stop users are recorded.
- **Field root:** both save the priority-15 root task globally and split event-driven entry from direct field entry. The event branch stores the reader and event coordinates in state 0; direct entry stores field/room/entry/spawn/direction and load modes, requests the primary field file and uses state 1. Camera placement follows. Golden's larger work and additional field cases are explicit deltas.
- **Encounter scheduler:** the paired field-band gate, 16-byte work, priority-15 setup/free callbacks and stored spawn limit identify the constructor, rather than a generic task template or module label.
- **Collision family:** both complete readers decode resource-class bits 10..19, resolve classes 1/3, and associate models against four party and fifteen enemy slots. The priority-15 controller owns a priority-16 updater child carrying the resource key. Golden adds a conditional sphere child and substantially enlarges the controller; these differences are recorded, not hidden by the shared names. Both selected rows are resolved as one family with the same sole PS2 source reader.
- **Follower:** both constructors store follower/leader identities, formation -1 and the camera near-plane value after creating the priority-15 chaser with update/free callbacks. Pointer-width/work-layout differences are explicit.

## Binding and limits

Every admission records the complete NUL-terminated Steam data span, its hash and bounds, corroborating NSO address, corresponding PS2 reader, constructor/code witnesses and `required_seams`. Runtime inputs are the allowlisted **Steam install only**. NSO code/data and private research files are static witnesses, never player dependencies.

Install only the exact name prefix and retain the partition tail. Every translated/inlined/compiled-native copy and any registration/lookup/stop operation must consume the same qualified identity. Constructor work sizes/offsets are proof evidence, **not** instructions to copy or widen PS2 structs. This batch does not certify Golden collision physics, AI, camera behavior, startup audio or complete task scheduling equivalence.

Private helper `<scratch>/GoldenRE-notes/l1/l1c3A_task_proof.py` reported `QUALIFIED 7 192`, with exact live spans and set-A membership assertions. Eleven additional Romwright analyse-function/worker pairs returned zero. Ordinary ARM64 ABI and Steam `.shared` constructor instructions were examined independently of worker-inferred prototypes; generic task-API references alone were not treated as literal identity proof.

No build, verify, objdiff, unit suite, BBV, CI, game/emulator execution or `.arch` decoding was performed. M6b must qualify the runtime seams separately. All unowned/B JSONL lines were preserved byte-for-byte by the row installer.
