# L1 cohort 3A1: complete local-map pose/marker bank

This batch admits **5 PROVEN /124 selected PS2 B** as one logical family: `005f2190`, `005f2194`, `005f2198`, `005f219c`, `005f21a4`. All remain **pending-seam**, requiring **`golden-native-driver:local-map-pose-bank`**. The complete card is [l1-cohort-3A1-pose-bank-proof.json](l1-cohort-3A1-pose-bank-proof.json).

## Whole bank and every user

The old aliases are parts of one **five-record/24B eye+target camera table**,120 logical bytes. Its unselected zero target-Y word is included in the family proof; the108B final partition also contains8B tail. No scalar is admitted in isolation.

Golden replaces this with a full **nine-record/36B eye+target+marker bank**,324B, sealed at Steam `.xpdata` RVA`b0d1e0`/raw`b0b9e0`, corroborated at NSO`7100765648`. Exact vector values, bounds and hashes are in the card. Many values differ from PS2. Complete Steam/NSO source equality is corroboration, **not** the PS2 structural proof.

The already-proved local-map owner independently loads field1/room1 and `field/script/lmap.bf`. Its registered `camera pos` task grows to72B, stores its parent-map pointer, and performs the corresponding eye/target interpolation and final snap. Both old readers are covered:

- `1891d0`: camera path update.
- `189600`: camera move/snap setter, whose sole identified old caller is the local-map main update`186eb0`.

Golden NSO main`1dbcb0` performs marker-position copies, camera creation and the inlined setter; callback`1def40` updates/snap-reads the bank. All three ordinary Steam bank-reader spans are recorded: `2f8705..2f8896` marker vectors, `2f89d5..2f8c0d` camera interpolation, `2f9050..2f9169` move/snap setter. Camera uses row+0/+12; marker uses row+24. Dropping the third vector or taking just a five-row/24B prefix would be incomplete.

## Complete selector producer and bound

The native owner registers **base physical IDs `[0,1,2,3,5]`**, not old ordinal0..4. It can append IDs8,6,7, at most eight active choices. Physical ID4 exists in the full nine-record source bank but is deliberately skipped by registration. Maximum produced ID8 and36B indexing bound the complete addressed bank including this hole.

- ID8 iff `!flag(cb5) && flag(cb4)`.
- ID6 iff `!flag(cb0) && flag(cae)`.
- ID7 iff `!flag(cb1) && flag(caf) && signed_date<14b`.
- Nonzero debug mode registers all eight choices in order`0,1,2,3,5,8,6,7`.
- Initial selection searches the requested physical ID in that produced list. No new missing-ID fallback or clamp is invented.

The selector/count/selected-slot work fields and full ordinary constructor code are paired on both Golden platforms. The main callback's actual entry is **`1dbcb0`** (floating-register prologue); the function index begins inside it at`1dbcdc`. Treating that interior label as the registration target would miss the producer. This is positive constructor/consumer evidence, not a missing-reference bound.

## Semantic installation, not a float patch

One qualified provider must own the complete bank and native owner/selector/camera/marker lifetimes. In Golden mode both old table readers and every translated/inlined/compiled-native column/copy alias must use **logical slot ->parent selector ->physicalID*36**. Keep the old partitions inactive; do not overwrite them with324B, allocate five independent copies, retain old ordinal IDs/24B stride, or fabricate a selector for ID4.

Golden also changes the algorithm: saved-start vectors and `(sin(pi*frame/duration-pi/2)+1)/2` replace the old iterative half-sine advance of current vectors. Completion/frame/debug-camera behavior differs. Source identity does not certify camera timing, renderer compatibility or72B native/guest work compatibility; the provider remains unqualified. Runtime input is the sealed Steam bank, never NSO/private caches.

Executed static helper:

`"<git-install>/bin/bash.exe" -c 'export PATH=/usr/bin:/bin:$PATH; <python>/python.exe <scratch>/GoldenRE-notes/l1/l1c3A1_pose_bank_proof.py'`

Observed:`QUALIFIED5/124B; COMPLETE_GOLDEN9x36=324B; BASE_IDS0,1,2,3,5_PLUS8,6,7; ALL_OLD_READERS2_AND_NATIVE_POSE_MARKER_USERS`. Five additional Romwright pairs returned0 (main index entry, actual main entry, constructor, camera update and destructor). Ordinary Steam registration/readers and native code were inspected independently of worker prototypes.

Canonical/triage all5 and archive-supplement all4 intersecting aliases were updated together. Every foreign raw JSONL line was preserved byte-for-byte; no old3A/A2 proofs or shared totals changed. No build, verify, objdiff, unittest, BBV, CI, game/emulator execution or `.arch` decoding was run.
