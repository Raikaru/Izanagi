# L1 cohort 3A1: positive NPC device/pre-request trace retirement

This batch decides **2 D10c /24 PS2 B**, runtime **pending-seam**: `005f0790`16B (`CDV:\data\`) and `00762f20`8B (`%s %d`). The complete card is [l1-cohort-3A1-npc-retirement-proof.json](l1-cohort-3A1-npc-retirement-proof.json).

This is **not** a no-reference, missing-string, module-label or cosmetic-retention decision. All four old consumers are positively resolved as complete request families on both Golden platforms:

| Old request | Ordinary Steam | NSO | Positive retained role |
|---|---|---|---|
| `15c750` | `3061f0` | `1e64b0` | scene table day/night path and real request |
| `15ccc0` | `306760` | `1e6940` | secondary BF day/night path and real request |
| `15cf70` | `3069d0..306aa3` | `1e6b80` | fixed table cache/owned copy/count/132B marker traversal |
| `15d100` | `306ab0` | `1e6c60` | indexed `field/script/comu%02d.bf` request |

Existing read-only [field bank proof](l1-cohort-3-field-bank-proof.json) independently qualifies the complete scene/secondary key recipes and fixed-table shape. This new card binds their hash and adds every old reader, the block reader, actual argument/dataflow evidence and full code bounds/hashes. It does not edit those older proofs.

## What was positively retired

**Device-directory local:** original R5900 code forms the128B directory buffer address exactly once for `strcpy442830`. It never loads this local, passes it to filename formatting/existence/I/O, or otherwise consumes/escapes its contents. Real filenames are formed in a distinct128B buffer. The first three functions use directory`sp+90` versus filename`sp+10`; the block reader uses`sp+b0` versus filename`sp+30`. All four explicit dataflow paths are checked. Golden's complete paired flows form only the real filename/cache key; no such device-directory preparation survives.

**Source-location output:** the original printf arguments are `gp-61d0=762f20`, file`5f07c0`, and source lines`68`, `140`, `1a2`, `1df` respectively. The real request follows separately. This is a source-file/line print, **not** a mutable numeric pair or resource identifier. The complete native replacements retain the real operations but not these explicit request-print sites/arguments. Fixed-table allocation source tagging is a different real native operation and is not removed or replaced by fake release logging. Unrelated SDK users of `%s %d` are not counterparts for these four sites.

Generic Capstone incorrectly labels R5900 LQ/SQ as unrelated MSA/DSP operations. The gate therefore checks raw opcode/register/immediate fields for original stack/dataflow evidence, rather than treating that misleading disassembly as ground truth. Steam's function index also splits the fixed-table body at`306a0e` even though execution continues: the witness includes the actual marker loop and return at`306aa2`, not merely the short index envelope.

## Binding and limits

Keep both old literal partitions inactive in Golden mode. Every old reader/native-copy alias must use the actual qualified Golden request/cache/install provider, retiring only the useless device copy and explicit old source print. Do not install empty loaders, constant success, a CDV fallback or substitute logging. Preserve time-slot5 day/night selection, packed-source bypass1, actual copies/readiness/count/ownership and errors. Golden's block path has unsigned index<131, request flag0 and native request slot+18; do not retain the old unbounded/FileExists/flag1 sequence as a guessed compatibility path.

This does not claim NPCs, all Golden logging or all device paths disappeared. It proves two specific old literal-dependent operations have no counterpart in the positive replacement protocols. Resource payloads/VM compatibility and runtime provider qualification remain separate.

Executed:

`"<git-install>/bin/bash.exe" -c 'export PATH=/usr/bin:/bin:$PATH; <python>/python.exe <scratch>/GoldenRE-notes/l1/l1c3A1_npc_retirement_proof.py'`

Observed:`DECIDED_D10c2/24B; ALL4READERS_RAW_R5900_DATAFLOW; PAIRED_STEAM_NSO_REAL_IO_PRESERVED`. Four additional Romwright pairs returned0. Canonical/archive/triage copies were updated together; every foreign raw line was preserved byte-for-byte. No shared/global totals or A2 rows changed. No build, verify, objdiff, unittest, BBV, CI, game/emulator execution or `.arch` decoding was run.
