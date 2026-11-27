# L1 cohort 3A1: active player-name output format

This batch admits **`00762ca0`:1 PROVEN /8 PS2 B**, runtime **pending-seam**. The complete card is [l1-cohort-3A1-name-format-proof.json](l1-cohort-3A1-name-format-proof.json). Only this owned row in canonical/archive/triage, its new proof/document and own summary were changed; every unowned raw JSONL line was preserved byte-for-byte.

## Not a diagnostic

`%s %s` constructs the active two-part player display name. PS2 `10d560` clears18/18/36B buffers, copies the two18B stored name fields and formats the output. PS2 `10e710` performs the same cache rebuild after restoring save sections; its GP alias resolves to `762ca0`. The nearby `extract data ID` trace is a different object and remains OPEN: proving the name-cache tail does not prove the entire Golden save-section parser or logging removal.

Independent ordinary Steam `19e5d0` and NSO `111850` preserve the same name-buffer/field protocol, adding locale-dependent argument ordering and encoding updates. Steam `19e8f0` and NSO `111ab0` also format the two profile fields at+c/+1e into a caller buffer. These are structural operand/buffer/continuation anchors, not a generic-format spelling vote. Both old literal users have explicit `required_seams`; post-restore and native/inlined cache copies remain active.

## Exact value and selected span

The complete6B NUL-terminated source is Steam `.sdata` RVA`91ca14`, raw`91b614`, corroborated at NSO`71005f6938`. Span/image SHA-256 and code bounds/hashes are in the card. Retain2PS2 tail bytes.

Raw first-match candidates are wrong: Steam`8fbf02` and NSO`5e73b0` occur inside longer strings. Actual paired LEA and ADRP+ADD instructions select the standalone spans above. Do not use byte similarity to choose a source occurrence, suppress this format as debug output, keep the old unconditional argument reversal, or treat misleading worker-import labels as reliable formatter prototypes.

The qualified provider must preserve **target-Steam** locale/encoding/name-cache semantics; NSO is static corroboration, never a runtime dependency. This string proof does not certify save payload layouts, Unicode conversion or the whole name-entry UI. No new truncation/default/validation is invented.

## Exercised evidence

Executed:

`"<git-install>/bin/bash.exe" -c 'export PATH=/usr/bin:/bin:$PATH; <python>/python.exe <scratch>/GoldenRE-notes/l1/l1c3A1_name_format_proof.py'`

Observed:`QUALIFIED1/8B; BOTH_OLD_NAME_USERS2; PAIRED_STEAM_NSO_FORMATTERS4; ACTIVE_NOT_DIAGNOSTIC`. Both ordinary Steam bodies and corresponding NSO code were examined; three additional Romwright pairs returned0 during this family/candidate investigation. The unrelated field-texture-loader candidate was not admitted as a mipmap counterpart merely because it shared preload filenames.

No build, verify, objdiff, unittest, BBV, CI, game/emulator execution or `.arch` decoding was performed. M6b must qualify both old reader/native-copy and name-provider paths before runtime closure.
