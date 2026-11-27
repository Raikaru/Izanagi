# L1 cohort 3A2: shared camp message bank

**1 PROVEN / 848 PS2 bytes:** `0064a790`.

This object is not a motion array. It is the complete embedded `MSG1` camp/card/configuration/quest message bank: **834 serialized bytes, eight records, fourteen allocation-tail bytes**. None of its old text or padding is retained as a Golden value.

The positive provider correspondence is:

1. NSO `fbc20` copies the supplied camp-table archive and resolves **`camp_msg.bmd`** into the shared source reached through GOT `880908`.
2. Steam `.shared` `1817c0` independently copies/walks the PAK-v2 member directory, resolves that named member and stores its pointer at RVA **`ec0a50`** (`181845`). The quest-notice setup body fragment `17b1a0` reads exactly that slot at `17b1ca` and calls native message-window acquisition `448b60`.
3. NSO notice setup `f7050` and window acquisition `2c85b0` corroborate the same source; `2d8240` validates/registers the actual compiled message bank. The independently proved quest wrappers preserve VM argument-minus-one, two modes and native completion handling.
4. Actual Steam resources are **`init_free.bin::init/cmpTable.bin::camp_msg.bmd`**, independently extracted and full-object size/hash qualified in **all nine installed base/locale archives**. Every Golden variant has **33 records** and preserves all original eight label/kind/index roles, with 25 additional Golden records. Matching labels are corroboration of this producer/consumer chain, not the proof by themselves.

The base object is 3,776 bytes; English is 3,904 bytes. Other qualified locale objects range from 3,360 to 4,096 bytes. Exact paths, hashes, records, original bytes/header and code witnesses are in `l1-cohort-3A2-camp-message-bank-proof.json`.

Use the **complete selected-locale Golden bank**, native decoder/relocation/registration and actual window/notice/UI providers. Do not fit a prefix into 848 guest bytes, truncate to eight records, copy host pointers, retain old presentation text, invent locale fallback, or reuse PS2 message-window handles. New native record selections remain Golden-aware.

Required seam: **`golden-native-driver:camp-message-bank`**; status **pending-seam**. All four direct PS2 readers and every translated/native/inlined copy must migrate together. Runtime source is the qualified Steam install, not NSO/private research files. This is source proof, not rendering/runtime acceptance.

The initial data-reference census returned no `ec0a50` sites because that slot is outside its data-backed reference population. Actual RIP-relative operands positively show the store and read above; the empty census was not used as absence evidence.
