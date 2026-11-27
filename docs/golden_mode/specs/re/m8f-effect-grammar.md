# M8f effect grammar census and static evidence

Final measured receipt: `<scratch>/GoldenRE-M8f-evidence/census.json`, SHA-256 `d92005cc0da2b3d1cf773eccc631347289853f58b29f024e405a689a08271747`. Reader `measure.py` SHA-256 `de9154625e2bf7fc533a3600ce34472112b3e4defe77b99690f2814eb7a37613` in the same private evidence directory. Neither contains game asset payloads; the receipt contains field metadata,16/32-byte signature prefixes and payload digests. No extracted binary is committed.

## Denominators

4400 selected logical objects,881 distinct extracted identities; zero extraction size/SHA mismatches, zero observed root/array/descriptor/payload range assertion failures. Every selected identity was extracted at least once. No hidden-byte or visual-support coverage is implied.

Per distinct outer identity, without deduplicating repeated embedded references: BED400458 identities;797 live slots and28515 FE physical slots;1070 immediate EPL roots and9845 immediate parts; after recursive adjunct5/6 traversal, **1231 EPL roots,10968 parts,10199 EP descriptor references**. Thus161 additional EPL roots and15 nested EP descriptors were reached. The same physical descriptor can be referenced repeatedly; these are reference occurrences, not unique physical allocation counts.

All10199 descriptors have file instance/primary-allocation/adjunct-allocation words zero. All932 part-clone references address earlier parts; all2050 adjunct-sharing references are in range and address earlier parts. Part sourceFlags histogram:0=7986,1=673,2=2050,3=259. Flag3 follows the clone branch. Serialized root linked-list pointers are zero throughout. These positive invariants are corpus observations, not permission to trust a future malformed file.

| CPK | EPL root references | Parts | EP descriptor references |
|---|---:|---:|---:|
| data.cpk | 1925 | 15750 | 15009 |
| data_c.cpk | 351 | 2135 | 1997 |
| data_ck.cpk | 291 | 1426 | 1361 |
| data_de.cpk | 291 | 1426 | 1361 |
| data_e.cpk | 409 | 2351 | 2202 |
| data_es.cpk | 291 | 1426 | 1361 |
| data_fr.cpk | 291 | 1426 | 1361 |
| data_it.cpk | 291 | 1426 | 1361 |
| data_k.cpk | 409 | 2351 | 2202 |

The table above expands locale/nested-pack duplicate paths; it is a different denominator from1231/10968/10199. The full named-object hashes and every immediate containing archive hash are in [census](m8f-census.md).

## EP kind and version framing

| Kind | Version | Descriptor references | Subtype/id values | Primary byte lengths | Adjunct kinds |
|---:|---:|---:|---|---|---|
| 1 | 200 | 161 | 1, 2, 3, 4, 5, 6, 8, 9, 10 | 224, 232, 236, 240, 244, 248 | 1, 2, 4, 6, 7 |
| 1 | 210 | 2453 | 1, 2, 3, 4, 5, 6, 8, 9, 10, 11 | 224, 232, 236, 240, 244, 248, 272 | 1, 2, 4, 5, 6, 7 |
| 2 | 200 | 19 | 1, 2, 3, 4, 5, 6, 8, 9, 10 | 252, 260, 264, 268, 272, 276 | 4 |
| 2 | 210 | 90 | 2, 3, 4, 5, 6, 8, 9, 10 | 252, 260, 264, 272, 276 | 4 |
| 4 | 200 | 30 | 1, 2, 3, 5, 6 | 232, 236 | 4 |
| 4 | 210 | 228 | 1, 2, 3, 4, 7 | 232, 236, 308 | 4 |
| 5 | 200 | 51 | 1, 2 | 216, 228 | 4 |
| 5 | 210 | 308 | 1, 2 | 216, 228 | 4 |
| 7 | 200 | 57 | 1 | 152 | 1, 4, 7 |
| 7 | 210 | 283 | 1 | 152 | 1, 2, 4 |
| 8 | 200 | 1 | 0 | 64 | 4 |
| 9 | 200 | 3 | 0 | 64 | 4 |
| 9 | 210 | 14 | 0 | 64 | 4 |
| 10 | 200 | 67 | 1, 2, 3, 4, 5, 6, 7, 8 | 128, 136, 140, 152, 156, 164, 168 | 1, 4 |
| 10 | 210 | 1197 | 1, 2, 3, 4, 5, 6, 7, 8, 9 | 128, 136, 140, 152, 156, 164, 168, 264 | 1, 4 |
| 11 | 200 | 1 | 0 | 8 | 4 |
| 12 | 200 | 102 | 1, 2, 3 | 104, 108, 208 | 4 |
| 12 | 210 | 1252 | 1, 2, 3 | 104, 108, 208 | 4 |
| 13 | 200 | 16 | 1, 3, 4 | 136, 172 | 4 |
| 13 | 210 | 80 | 1, 3, 4 | 136, 172 | 4 |
| 14 | 200 | 5 | 1, 2, 3, 4, 5 | 256, 264, 272, 276 | 4 |
| 15 | 200 | 27 | 1, 2, 3, 4, 5, 6, 8, 9, 10 | 252, 260, 264, 268, 272, 276 | 1, 4, 5, 6 |
| 15 | 210 | 16 | 3, 4 | 260, 272 | 1, 5, 6 |
| 16 | 200 | 14 | 1, 2, 3, 4 | 216, 244, 248 | 1, 4 |
| 16 | 210 | 292 | 1, 2, 3 | 216, 244, 248 | 1, 4 |
| 17 | 200 | 4 | 1, 2, 3, 4 | 152, 268, 300, 304 | 4 |
| 19 | 200 | 1 | 1 | 192 | 3 |
| 20 | 200 | 14 | 1, 2, 3, 6, 7, 8, 9, 11, 12, 14, 15, 16, 17 | 4, 12, 16, 20, 28, 36, 40, 76, 112, 152, 156, 340 | 1, 3, 4, 6 |
| 21 | 200 | 24 | 0 | 16 | 4 |
| 21 | 210 | 225 | 0 | 16 | 4 |
| 23 | 210 | 249 | 1 | 152 | 1, 7 |
| 26 | 220 | 261 | 0 | 104 | 3 |
| 27 | 220 | 50 | 0 | 1056, 1368, 1376, 1616, 1624, 1720, 1728, 1928, 2072, 2128, 2192, 2216, 2260, 2320, 2512, 2536, 2600, 2640, 2888, 7976, 23280 | 6 |
| 28 | 200 | 46 | 1, 2, 3, 4, 5, 6, 8, 9, 10 | 292, 300, 304, 308, 312, 316 | 3 |
| 28 | 210 | 106 | 1, 2, 3, 4, 5, 8, 11 | 292, 300, 308, 312, 316, 340 | 3 |
| 29 | 200 | 9 | 1, 2, 3, 4, 5, 6, 8, 9, 10 | 296, 304, 308, 312, 316, 320 | 1 |
| 29 | 210 | 1 | 8 | 320 | 1 |
| 30 | 210 | 56 | 1, 2 | 148, 160 | 4 |
| 31 | 200 | 128 | 1, 2, 3, 4, 5, 6 | 12, 16, 40, 48 | 3, 4 |
| 31 | 210 | 1939 | 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 | 12, 16, 32, 40, 48, 88, 108 | 3, 4 |
| 32 | 210 | 12 | 0 | 164 | 3 |
| 32 | 220 | 306 | 0 | 164 | 3 |
| 33 | 220 | 1 | 1 | 48 | 4 |

Length sets disprove any single fixed primary structure per primitive kind. Historical200 tool entries remain counted even when their Switch callback is null. Predicate/size closure is not semantic equivalence of every parameter tail.

## Attachment constants

NSO710062BB80 height-factor binary32 table: `[-1,0,1,1.5,0,0]`. Height-mode4 has its own explicit branch; the sixth word is recorded to avoid implying all six are independently selected by observed data.

NSO710062BB98 angular table binary32 values: `[0,45,90,135,180,-135,-90,-45,0,0,0]`. Position-mode8 uses its center branch;9/10 use view-relative branches, not ordinary angle lookup.

## Primitive dispatch provenance

NSO runtime table710086FAD0,34 entries of80 bytes, is zero in the unrelocated mapped data. Constructors were recovered using RELA type403 entries from mapped offsets533048..5C9948 (24-byte records), not by treating those zero words as null callbacks. The resolved constructor table and source assertion-string identities were cross-checked with ordinary function bodies.

| Kind | Constructor offset |
|---:|---|
| 0 | `365E40` |
| 1 | `36BB70` |
| 2 | `37E6F0` |
| 3 | `365E40` |
| 4 | `352DA0` |
| 5 | `3537F0` |
| 6 | `365E40` |
| 7 | `3794C0` |
| 8 | `365E40` |
| 9 | `35D060` |
| 10 | `377570` |
| 11 | `365E40` |
| 12 | `379190` |
| 13 | `37E400` |
| 14 | `365E40` |
| 15 | `37EF60` |
| 16 | `382E80` |
| 17 | `365E40` |
| 18 | `365E40` |
| 19 | `365E40` |
| 20 | `365E40` |
| 21 | `35DC40` |
| 22 | `365E40` |
| 23 | `366090` |
| 24 | `365E40` |
| 25 | `365E40` |
| 26 | `361D70` |
| 27 | `367FE0` |
| 28 | `36A420` |
| 29 | `366AE0` |
| 30 | `3691C0` |
| 31 | `34F890` |
| 32 | `362440` |
| 33 | `367DE0` |

## Source identities and bounded code receipts

Steam P4G.exe (inventory): `0f327c7669e6cac72aaf355181b1e51edd12fee4d71709fc1802dbf0da70546e`.

Switch1.2 main.nso SHA-256: `854c7a1e33de1619abf009fa7f32b8c012960d685c8bf1bf99a92a6a4d0bda1c`. Existing authorized static extraction: `<scratch>/romwright-p4g-switch/`; no new extraction/game execution was needed. Key material is not included.

| Architecture | Raw span, end exclusive (base omitted) | SHA-256 |
|---|---|---|
| NSO | `32AB8..32D24` | `1e50aa51bf87a6ab1c2b3f7323b41f21a095c1b4d8858391379410d3e1247156` |
| NSO | `33760..33CD0` | `0f14eeb941b76020d5e26afc81d7ee31da1072b54908bb341a8a3c6242d4270c` |
| NSO | `363660..3636C0` | `933d707167f80ea9de51453220d7a084d626fac189226f72d00dd0b7ca9a550a` |
| NSO | `3636C0..363830` | `548b39db8409d594a04b95ae59e61948395aafc190886774fc169ca980c83cc9` |
| NSO | `363B80..364540` | `251501bb39502178d8dc4a259300ddaa10a98223485a93936854a59006318fdb` |
| NSO | `364F80..365270` | `67dfb8d5307236ac5d091c564e53561de3b2fd299f716070c7597e98a40c28c5` |
| Steam | `79F50..7A144` | `456a44160cfdbec8424a6680e9c5f6806780cc070944509b34396de3db34ca20` |
| Steam | `507870..507A63` | `bdf7175bda76c2a2dc147377f21770b9c01c48ad44d9bbe04befd0ad7570c3b7` |

All Steam instruction reads checked the PE section table and required `.shared`. Protected jump destinations were not followed. NSO pseudocode came from the authorized fixed Romwright worker; pivotal wire widths, loop strides, count loads, offset adds and callback dispatch were separately cross-checked against Capstone ordinary instructions. Decompiler warnings are not treated as field proof.

Private static readers/receipts are under `<scratch>/GoldenRE-M8f-evidence/`: measure.py, decompile.py, census.json,7100*.txt,7100*-analysis.json and steam-*.txt. No code output or proprietary implementation is committed.

## Known limits

- Full recursive inventory traversal recognizes existing PAK/CPK/AFS framing; arbitrary unnamed bytes inside an unrecognized container are not a proof of absence of other formats. All named effect families and all recursively framed EPL/EP adjunct5/6 objects are included.
- M8c AMD/GMO helper effect suffixes remain a separate opaque adjunct frontier unless reached through these EP descriptors. Exact linkage to a battle EPL is OPEN O06.
- Descriptors are bounded against their containing outer asset (or the bounded nested adjunct span). A BED slot does not supply an independent EPL byte length; proving non-overlap of all independent suffix allocations is not claimed.
- No live-render, runtime sound, original-game frame or GPU output was produced. O01–O08 remain explicit acceptance gates, not hidden supported rows.
