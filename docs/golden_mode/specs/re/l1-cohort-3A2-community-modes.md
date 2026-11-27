# L1 cohort 3A2: community modes and portrait tasks

**5 PROVEN / 3,712 PS2 bytes.** The three 1,216-byte motion-bank rows (`0064b420`, `0064b8e0`, `0064bda0`) carry **PROVEN-code components** inside ledger PROVEN, following the cohort-3B controller convention. The two 32-byte task-name rows are `0064ccd0` and `0064cd40`.

Exact original words, Steam `.shared` instruction witnesses, qualified Steam data spans, independent NSO controller bodies and the complete authored schema are in `l1-cohort-3A2-community-modes-proof.json`.

## Positive controller replacement

PS2 `359400` selects the three owned banks for modes 0, 4 and 1 respectively, then consumes exactly **43 × 28 = 1,204 bytes**. The last 12 bytes in each census allocation are not live table rows. Alpha is a byte at row+16, not u32; x/y/scales are floats and timing fields are i32 at +20/+24. No tail, old alpha, old scales, footer or visibility constant is retained as a Golden value.

Steam `15acb0` and NSO `de480` preserve the unchanged-mode early return and changed-mode dispatch, but use **10 scalar / 3 integer channels**, with **34 ordered operations across modes 0–4**. This is a code-built successor, not a 43-row layout remap. The proof records the full schema and the helper guard/store semantics: scalar flag transitions, conditional versus unconditional integer retarget, timing 0/3/5/10, footer 18/19, the `af0` three-frame retarget and mode-3 `cb0=0`. Changed unknown modes perform no channel operations but still update the current footer; they do not run the PS2 assertion or bank fallback.

Old initializer `356250` independently pairs with Steam `15a710` and NSO `de070` through community roster, grid, native window, race-card and member-bust-up tasks. Golden initializes its new state and enters mode 0; it does not seed 43 current-motion records from the old mode-1 bank. Steam grid initialization has 88 24-byte records, eight columns and eleven rows; the independent NSO initializer unrolls this into 44 pairs. Community-kind capacity is Golden's 23, not the old 21.

## Task-name spans

The native factories positively register `cmpCommuRaceCardDraw` and `cmpCommuMemberBustUpDraw` at priority 199 with the corresponding update/destroy lifecycles. They allocate native work (Steam `0x170`, NSO `0x178`, versus PS2 `0x144`) and initialize/configure the same portrait/label roles. Steam `15c6f0` and NSO `de3d0` independently corroborate standalone factories. Name equality is not the mapping argument. Golden's label coordinates `(528,9)` / `(228,9)` replace PS2 `(353,94)` / `(653,94)`.

Required seam: **`golden-native-driver:community-modes`**. All five rows remain **pending-seam**. Migrate initialization, all five modes, portrait tasks and every translated/native/inlined reader atomically through qualified native UI/render providers. No guest layout overwrite, nearest-row matching, active PS2 cosmetic fallback, runtime NSO/code-byte reads or claim of renderer/runtime acceptance.
