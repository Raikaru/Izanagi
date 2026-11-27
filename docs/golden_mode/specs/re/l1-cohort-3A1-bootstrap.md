# L1 cohort 3A1: retained bootstrap and registration families

## Scope and counts

Main's second module split is recorded at `<scratch>/GoldenRE-L1-c3-SPLIT2.md`. It initially retained29 OPEN-baseline rows/832B in A1 and assigned the connected59-row component to GoldenRE-L1-c6b. The merged ledger additionally records Main-transferred `0063f3e0`/`00763960` under GoldenRE-L1-c6c, so those2/40B are excluded: the remaining exclusive A1 baseline is27/792B. Earlier23 PROVEN A rows/671B and their old proof/summary files remain read-only history. New owner-specific files avoid concurrent summary edits; global counts belong to GoldenRE-L1-c12.

The bootstrap batch establishes **4 PROVEN /112 PS2 B**. The [active name-format follow-on](l1-cohort-3A1-name-format.md) adds **1 /8 B**, the [complete type3 filter constructor schema](l1-cohort-3A1-filter-schema.md) adds **1 /48 B**, the [complete local-map pose/marker bank](l1-cohort-3A1-pose-bank.md) adds **5 /124 B**, [positive NPC device/source-trace retirement](l1-cohort-3A1-npc-retirement.md) decides **2 D10c /24 B**, and [positive null-effect output retirement](l1-cohort-3A1-effect-trace.md) decides **1 D10c /32 B**, giving exclusive A1 **11 PROVEN /292 B**, **3 D10c /56 B** and **13 OPEN /444 B**. PROVEN-layout/D10a/D10b are zero within this exclusive scope; c6c's separately transferred shop decisions are not counted here. All14 decided rows are **pending-seam**, not runtime closure. The bootstrap card is [l1-cohort-3A1-bootstrap-proof.json](l1-cohort-3A1-bootstrap-proof.json).

| PS2 row | Complete Golden value | Corresponding old / NSO reader |
|---|---|---|
| `005e46a0` | `init/itemtbl.bin` | `1117c0` / `277680` |
| `005efb60` | `script shutdown(kosaka)` | `14b990` / `1f6110` |
| `005efc28` | `traest proc` | `14ec50` / `1bda10` |
| `0063b568` | `event/icon.bin` | `26d840` / `2a6080`, with complete member/attachment family |

Each card contains complete NUL-terminated Steam `.sdata` bounds, hash, image identity, NSO corroboration, code bounds/hashes and all old literal/native-copy seams. Runtime inputs are the sealed Steam install; never the NSO or private caches. Only the exact qualified prefix is installed, with its specified PS2 partition tail retained.

## Structural correspondence, not equal-text votes

- **Item bootstrap:** both staged readers request at state0, wait for readiness, install item data, release the transient request and finish with-1. Golden first checks its cache and may complete in the **same call** after entering state1; do not force an extra frame. Golden `111c10` copies into its owned buffer and rebuilds count-dependent68B-record pointers. This source proof does not authorize copying the Golden payload into a PS2 item layout; a qualified Golden-aware installer remains required.
- **Shutdown identity:** original MIPS proves4B old work versus8B Golden. Both register a null-parent priority15 one-shot helper carrying a script-task handle; update destroys its hierarchy and finishes-1, then destructor frees work. Golden also clears the stored handle. Raw `1f6174` through return preserves the created-task handle in x0, despite the worker's inferred `void` signature. The destructor is an actual8B load-and-tail-branch wrapper at `1f61c0`, not a guessed metadata envelope.
- **Transition identity:** mode0/1 create a child using the full `field/TRAEST.EPL` source, request flag0 and exact83/50 durations. Work grows80->96B and child252->304B. Allocation failures, parent/child ownership, priority15 and mode storage establish identity; no guest-work widening or effect-timing/rendering equivalence is claimed.
- **Icon bank:** synchronous archive preload, cached member lookup and the two-state effect attachment protocol identify the full corresponding bank. The old task and Golden `2a61d0` attach to resource classes1/3 and release the prior effect. The entire26-entry vector resolves `event/_ICON1.EPL` through `event/_ICON26.EPL` to bare Golden `_ICON1.EPL` through `_ICON26.EPL`. The archive-aware member provider and companion old task`26d890` are explicit seams, not a filename-only patch.

### Do not invent a twenty-seventh icon

Both task-reader predicates accept unsigned index26 (old rejects>26, Golden rejects>=27), but **both physical tables have only26 pointers**. Original table`63b4f0` is104B and its next word is the beginning of `mt_sceneIcon.c`. NSO table`792ac0` has exactly26 pointer relocations/208B; the following nonrelocated word is float1.0. This is positive bounded structural evidence, not an absence-of-reference conclusion.

The card deliberately records the mismatch. Do not invent a27th member, normalize its bytes as a pointer, silently add a26-element clamp/default, or claim index26 safe. Input-domain/consumer qualification remains necessary before runtime closure. The separate pointer-table row and effect payload/rendering are not admitted by the archive-string proof.

## Exercised static evidence

Executed:

`"<git-install>/bin/bash.exe" -c 'export PATH=/usr/bin:/bin:$PATH; <python>/python.exe <scratch>/GoldenRE-notes/l1/l1c3A1_bootstrap_proof.py'`

Observed: `QUALIFIED4/112B; ALL_OLD_LITERAL_USERS4; ICON_EXPLICIT_MEMBERS26_NO27TH; SHUTDOWN_RETURN_ABI_OBSERVED`. The first helper attempt encountered a missing destructor entry in the function index; the actual8B ordinary wrapper was decoded and explicitly bounded, then the source gate succeeded. Nine new Romwright analyse-function/worker pairs returned0. Source image hashes, original MIPS, full relevant decompilations and return ABI were checked statically.

The installer preserved all unowned canonical/triage lines byte-for-byte; no archive-supplement duplicate intersects these four rows. No old A/A2 proof, shared/global prose or totals were edited. No build, verify, objdiff, unittest, BBV, CI, game/emulator execution or `.arch` decoding was performed. Main owns repository checks; M6b must qualify all source/native-copy and resource-provider seams separately.
