# M8c census: complete grammar and measured field tables

Companion to [the model spec](model-formats.md) and [embedded textures](model-formats-textures.md). All counts below are **OBSERVED**, build 10836033, not renderer results. Semantic names remain I/P where marked in the parent spec.

## Frozen inputs and receipts

* M7c source base: `ef7b5b7ed`.
* Inventory SHA-256: `150454e1b60d50ccdd58346b6da4e18e363860bf49bb22a679db379dee3771f8`.
* Full grammar receipt SHA-256: `833706a6730f9526f9d5478f0df643085121ab7f1c8b5a283b635eb5f52e03c5`.
* Full contextual/texture receipt SHA-256: `ecb36894506ffa5be35acec5965b5d3b581914698b92557cfd7124ab37353d0b`.
* Owner-local receipts: `<scratch>/GoldenM8cSpec-evidence/model-grammar-final.json` and `model-context-final.json`.
* Numerical witness: `numerical-witness.json`; produced by `tools/golden/model_witness.py`. It writes hashes/errors, no image payload.

Both full passes read **5,778 / 5,778 selected inventory objects**, with **0 hash/size mismatches, 0 framing errors, and 0 unclassified objects**. Classification is not a claim that unknown rendering semantics are implemented. No payload is in this annex.

## Coverage denominators

| Grammar class | Inventory objects |
| --- | ---: |
| AMD | 4,679 |
| GMO | 55 |
| pak-v1 | 3 |
| pak-v2 | 1,041 |
| **Total** | **5,778** |

The 4,679 AMDs contain 4,679 MODEL_DATA GMOs and 600 NECK_BENDER archives; each neck archive contains one GMO and one 32-byte parameter object. Adding the 55 inventory GMOs gives **5,334 GMO files**, **5,401 model blocks**, and **7,375,227 chunks/commands**. The 6,360 AMD member records are all classified by name/tag below. **494/494 model/pack PAC roots are PAK v2.** Counts retain locale and nested duplicates.

### Asset-family census, including out-of-battle objects

| Inventory family | Objects |
| --- | ---: |
| AMD field | 12 |
| AMD field/grmd | 7 |
| AMD model/field | 37 |
| AMD model/npc | 1,328 |
| AMD model/npc2 | 845 |
| AMD model/persona | 243 |
| AMD model/symbol | 3 |
| AMD model/weapon | 308 |
| AMD packed in  | 9 |
| AMD packed in field/pack | 1,275 |
| AMD packed in model/pack | 494 |
| AMD packed in title | 118 |
| GMO direct/packed | 55 |
| PAC field/btl_effect | 1 |
| PAC field/pack | 1 |
| PAC model/pack | 494 |
| PAC skill | 547 |
| PAC system/tool/model | 1 |

### AMD members

| Name / tag | Occurrences | Payload byte range |
| --- | ---: | --- |
| `EFFECT_ADDMOT.2e4` | 109 | 160..800 |
| `EFFECT_EDGE.2e5` | 22 | 32 |
| `EFFECT_EPL.2e0` | 236 | 544..52640 |
| `EFFECT_EPL.2e2` | 197 | 608..79968 |
| `EFFECT_TRACK.2e3` | 517 | 128..3936 |
| `MODEL_DATA.101` | 4,679 | 288..4745344 |
| `NECK_BENDER.2d0` | 600 | 40544..85472 |

### Framing and variable-record invariants

| Invariant | Observed count / result |
| --- | --- |
| Every chunk offset/header end/total size divisible by four | 7,375,227 true; 0 false |
| Block spans bounded by parent; H<=A<=C<=S | all 5,334 GMO walks complete |
| Curve buffer formula | 1,984,942 true; 0 false |
| One-UV-only vertex formula | 90,509 true; 1,884 false (additional UV sets) |
| Complete 1+extra-UV vertex formula | 92,393 true; 0 false |
| Sequential/u16-index draw payload formula | 493,569 true; 0 false |
| Blend-palette count ×4 +4 | 5,005 true; 0 false |
| Bind-offset count ×64 +4 | 5,005 true; 0 false |
| Blend-subset count ×4 +4 | 56,602 true; 0 false |
| per3Helper 52-byte payload, quaternion norm near 1, eight zero trailing bytes | 12,295 / 12,295 |
| TIM2 base image bytes equal dimensions × depth | 25,742 true; 0 false |

Suffixes are length-delimited opaque regions, not extra chunks. The complete length distributions follow. Nonzero suffix counts: AMD **179**, GMO **4,597**, neck archives **600**. Zero suffix bytes do not prove reserved semantic meaning.

| Region / count field | Objects | Range | Distribution |
| --- | ---: | --- | --- |
| `amd.members` | 4,679 | 1..21 | 16 values; range at left |
| `amd.trailer.bytes` | 4,679 | 0..24 | 0 ×4487; 8 ×124; 16 ×44; 24 ×24 |
| `gmo.bytes` | 5,334 | 288..4745344 | set enumeration capped at 64 |
| `gmo.trailer.bytes` | 5,334 | 0..28 | 0 ×697; 4 ×738; 8 ×645; 12 ×656; 16 ×627; 20 ×654; 24 ×604; 28 ×713 |
| `neck.members` | 600 | 2 | 2 ×600 |
| `neck.child.bytes` | 600 | 32 | 32 ×600 |
| `neck.trailer.bytes` | 600 | 20 | 20 ×600 |
| `gmo.model.motion_count` | 5,401 | 0..35 | 36 values; range at left |
| `gmo.motion.curves` | 46,796 | 0..369 | set enumeration capped at 64 |
| `arrays.vertices` | 92,393 | 0..17522 | set enumeration capped at 64 |
| `arrays.stride` | 92,393 | 6..32 | 6 ×13; 10 ×52; 12 ×694; 14 ×1810; 16 ×12894; 18 ×32537; 20 ×6612; 22 ×36753; 24 ×1021; 28 ×1; 32 ×6 |
| `arrays.weights` | 92,393 | 0..6 | 0 ×35791; 1 ×6679; 2 ×3924; 3 ×3066; 4 ×3907; 5 ×3280; 6 ×35746 |
| `arrays.weight_sum_min` | 56,602 | 127..128 | 127 ×75; 128 ×56527 |
| `arrays.weight_sum_max` | 56,602 | 128 | 128 ×56602 |
| `curves.format` | 1,984,942 | 128..132 | 128 ×1002081; 129 ×71470; 132 ×911391 |
| `curves.dimensions` | 1,984,942 | 1..4 | 1 ×38; 3 ×173347; 4 ×1811557 |
| `curves.keys` | 1,984,942 | 1..1098 | set enumeration capped at 64 |
| `helper.id` | 12,295 | 0..1551 | set enumeration capped at 64 |
| `helper.quaternion_norm2` | 12,295 | 0.9999983609779131..1.0000015399784843 | set enumeration capped at 64 |

## Every observed GMO type

Sizes include the header. H is u16 at chunk +2. Data/argument ranges use the H/A/C definitions in the parent spec. Commands have H=0 and payload [8,S). A row here is an exhaustive grammar classification, including OPEN types.

| Type | Role / uncertainty | Occurrences | H range | Total-size range | Args bytes | Data bytes |
| --- | --- | ---: | --- | --- | --- | --- |
| `0002` | file | 5,334 | 16 | 264..4745308 | 0 | 0 |
| `0003` | model | 5,401 | 24..52 | 124..4745292 | 0 | 0 |
| `0004` | bone | 300,622 | 24..56 | 44..12760 | 0 | 0 |
| `0005` | part | 92,393 | 28..56 | 184..315976 | 0 | 0 |
| `0006` | mesh | 92,394 | 24..56 | 52..16036 | 0 | 0 |
| `0007` | arrays | 92,393 | 28..56 | 60..315448 | 16 | 0..315396 |
| `0008` | material | 38,628 | 28..84 | 44..664 | 0 | 0 |
| `0009` | layer | 41,151 | 44..96 | 68..236 | 0 | 0 |
| `000A` | texture | 25,750 | 20..52 | 40..263336 | 0 | 0 |
| `000B` | motion | 46,796 | 36..56 | 36..210932 | 0 | 0 |
| `000C` | FCurve | 1,984,942 | 28..96 | 52..8844 | 16 | 4..8784 |
| `8012` | filename | 25,750 | 0 | 12..44 | payload | payload |
| `8013` | file image | 25,747 | 0 | 172..263244 | payload | payload |
| `8014` | bounds | 129,136 | 0 | 32 | payload | payload |
| `8015` | quantization | 8,779 | 0 | 28..36 | payload | payload |
| `8021` | declaration (OPEN U14) | 7 | 0 | 24 | payload | payload |
| `8022` | declaration (OPEN U14) | 7 | 0 | 20 | payload | payload |
| `8023` | declaration (OPEN U14) | 7 | 0 | 32 | payload | payload |
| `8041` | parent | 295,220 | 0 | 12 | payload | payload |
| `8044` | blend palette | 5,005 | 0 | 16..712 | payload | payload |
| `8045` | bind offsets | 5,005 | 0 | 76..11212 | payload | payload |
| `8048` | translation | 300,598 | 0 | 20 | payload | payload |
| `8049` | Euler rotation (order OPEN) | 300,575 | 0 | 20 | payload | payload |
| `804B` | quaternion rotation | 44 | 0 | 24 | payload | payload |
| `804C` | scale | 300,617 | 0 | 20 | payload | payload |
| `804E` | DrawPart | 108,626 | 0 | 12 | payload | payload |
| `8061` | material ref | 92,393 | 0 | 12 | payload | payload |
| `8062` | blend subset | 56,602 | 0 | 16..36 | payload | payload |
| `8066` | draw | 493,569 | 0 | 28..2196 | payload | payload |
| `806B` | OPEN U14 | 1 | 0 | 12 | payload | payload |
| `8081` | material state (OPEN U05) | 10,396 | 0 | 16 | payload | payload |
| `8082` | material RGBA candidate | 38,602 | 0 | 24 | payload | payload |
| `8083` | property (OPEN U05) | 150 | 0 | 28 | payload | payload |
| `8084` | property (OPEN U05) | 25 | 0 | 24 | payload | payload |
| `8085` | material vector candidate | 38,591 | 0 | 24 | payload | payload |
| `8086` | state (OPEN U05) | 38,555 | 0 | 12 | payload | payload |
| `8087` | state (OPEN U05) | 38,555 | 0 | 12 | payload | payload |
| `8088` | state (OPEN U05) | 38,555 | 0 | 12 | payload | payload |
| `8091` | texture ref | 38,685 | 0 | 12 | payload | payload |
| `8092` | layer state (OPEN U05) | 38,673 | 0 | 12 | payload | payload |
| `8093` | layer state (OPEN U05) | 38,673 | 0 | 12 | payload | payload |
| `8094` | layer state (OPEN U05) | 41,151 | 0 | 20 | payload | payload |
| `8097` | layer state (OPEN U05) | 1,985 | 0 | 16 | payload | payload |
| `8098` | layer state / animated channel (OPEN U05) | 6 | 0 | 24 | payload | payload |
| `8099` | layer state (OPEN U05) | 1,985 | 0 | 12 | payload | payload |
| `809A` | matrix candidate (OPEN U05) | 1,985 | 0 | 72 | payload | payload |
| `80B1` | frame interval | 45,602 | 0 | 16 | payload | payload |
| `80B2` | frame rate | 45,602 | 0 | 12 | payload | payload |
| `80B3` | animate binding | 1,984,950 | 0 | 24 | payload | payload |
| `80E2` | OPEN U14 | 1 | 0 | 16 | payload | payload |
| `80F1` | custom data | 59,008 | 0 | 24..60 | payload | payload |

### Complete parent-child grammar

| Parent > child | Occurrences |
| --- | ---: |
| `0002>0003` | 5,401 |
| `0002>8021` | 7 |
| `0002>8022` | 7 |
| `0002>8023` | 7 |
| `0002>80f1` | 4,588 |
| `0003>0004` | 300,622 |
| `0003>0005` | 92,393 |
| `0003>0008` | 38,628 |
| `0003>000a` | 25,750 |
| `0003>000b` | 46,796 |
| `0003>8014` | 4,882 |
| `0003>8015` | 8,779 |
| `0004>8014` | 31,897 |
| `0004>8041` | 295,220 |
| `0004>8044` | 5,005 |
| `0004>8045` | 5,005 |
| `0004>8048` | 300,598 |
| `0004>8049` | 300,575 |
| `0004>804b` | 44 |
| `0004>804c` | 300,617 |
| `0004>804e` | 108,626 |
| `0004>80e2` | 1 |
| `0004>80f1` | 14,411 |
| `0005>0006` | 92,394 |
| `0005>0007` | 92,393 |
| `0005>8014` | 92,357 |
| `0006>8061` | 92,393 |
| `0006>8062` | 56,602 |
| `0006>8066` | 493,569 |
| `0006>806b` | 1 |
| `0008>0009` | 41,151 |
| `0008>8081` | 10,396 |
| `0008>8082` | 38,602 |
| `0008>8083` | 150 |
| `0008>8084` | 25 |
| `0008>8085` | 38,591 |
| `0008>8086` | 38,555 |
| `0008>8087` | 38,555 |
| `0008>8088` | 38,555 |
| `0008>80f1` | 40,009 |
| `0009>8091` | 38,685 |
| `0009>8092` | 38,673 |
| `0009>8093` | 38,673 |
| `0009>8094` | 41,151 |
| `0009>8097` | 1,985 |
| `0009>8098` | 6 |
| `0009>8099` | 1,985 |
| `0009>809a` | 1,985 |
| `000a>8012` | 25,750 |
| `000a>8013` | 25,747 |
| `000b>000c` | 1,984,942 |
| `000b>80b1` | 45,602 |
| `000b>80b2` | 45,602 |
| `000b>80b3` | 1,984,950 |
| `file>0002` | 5,334 |

## Argument-field measurements

Offsets are relative to H (the argument start), not chunk zero. These are u32 fields or raw u32 bit views; the parent spec assigns semantic types. All argument words present in this corpus are included.

| Block / argument offset | Observations | Unsigned range | Values / frequencies |
| --- | ---: | --- | --- |
| `gmo.0007.arg00` | 92,393 | 100667552..536875491 | 38 values; range at left |
| `gmo.0007.arg04` | 92,393 | 0..17522 | set enumeration capped at 64 |
| `gmo.0007.arg08` | 92,393 | 1 | 1 ×92393 |
| `gmo.0007.arg0c` | 92,393 | 0..3 | 0 ×90509; 1 ×1531; 2 ×342; 3 ×11 |
| `gmo.000c.arg00` | 1,984,942 | 128..132 | 128 ×1002081; 129 ×71470; 132 ×911391 |
| `gmo.000c.arg04` | 1,984,942 | 1..4 | 1 ×38; 3 ×173347; 4 ×1811557 |
| `gmo.000c.arg08` | 1,984,942 | 1..1098 | set enumeration capped at 64 |
| `gmo.000c.arg0c` | 1,984,942 | 0 | 0 ×1984942 |

## Command-prefix field measurements

Offsets `wordXX` are **payload-relative**. A u32 view of a float is not an integer semantic claim. Each view samples the named offset in **every occurrence with that offset**; counts therefore differ for variable-length lists. For counted palettes/matrices/subsets, later elements follow the separately verified stride/count grammar; these prefix ranges must not be used as coefficient limits on the complete arrays. Opaque fields remain OPEN under the parent spec's U03/U05/U07/U11/U13/U14. Image/string/custom-tag byte payloads are deliberately not reproduced. F32 interpretations in the semantic spec, rather than these bit-pattern extrema, govern decoding.

| Chunk / payload offset | Observations | u32 bit-pattern range | Small value sets / frequencies |
| --- | ---: | --- | --- |
| `gmo.8014.word00` | 129,136 | 0..3353570139 | set enumeration capped at 64 |
| `gmo.8014.word04` | 129,136 | 0..3351760511 | set enumeration capped at 64 |
| `gmo.8014.word08` | 129,136 | 0..3352553767 | set enumeration capped at 64 |
| `gmo.8014.word0c` | 129,136 | 0..3330592688 | set enumeration capped at 64 |
| `gmo.8014.word10` | 129,136 | 0..3335499598 | set enumeration capped at 64 |
| `gmo.8014.word14` | 129,136 | 0..3326538877 | set enumeration capped at 64 |
| `gmo.8015.word00` | 8,779 | 3..384 | 3 ×4390; 384 ×4389 |
| `gmo.8015.word04` | 8,779 | 0..3406548315 | set enumeration capped at 64 |
| `gmo.8015.word08` | 8,779 | 0..3404426344 | set enumeration capped at 64 |
| `gmo.8015.word0c` | 8,779 | 912392192..3379832052 | set enumeration capped at 64 |
| `gmo.8015.word10` | 8,779 | 1024405679..3379894553 | set enumeration capped at 64 |
| `gmo.8015.word14` | 4,389 | 1046630941..3379894553 | set enumeration capped at 64 |
| `gmo.8015.word18` | 4,389 | 1046630941..3379894553 | set enumeration capped at 64 |
| `gmo.8041.word00` | 295,220 | 266240..266476 | set enumeration capped at 64 |
| `gmo.8044.word00` | 5,005 | 1..175 | set enumeration capped at 64 |
| `gmo.8044.word04` | 5,005 | 266240..266441 | set enumeration capped at 64 |
| `gmo.8044.word08` | 4,332 | 266240..266475 | set enumeration capped at 64 |
| `gmo.8044.word0c` | 3,982 | 266240..266476 | set enumeration capped at 64 |
| `gmo.8044.word10` | 3,809 | 266241..266440 | set enumeration capped at 64 |
| `gmo.8044.word14` | 3,770 | 266241..266435 | set enumeration capped at 64 |
| `gmo.8044.word18` | 3,725 | 266241..266446 | set enumeration capped at 64 |
| `gmo.8044.word1c` | 3,667 | 266242..266441 | set enumeration capped at 64 |
| `gmo.8044.word20` | 3,596 | 266241..266451 | set enumeration capped at 64 |
| `gmo.8044.word24` | 3,533 | 266241..266442 | set enumeration capped at 64 |
| `gmo.8044.word28` | 3,457 | 266241..266447 | set enumeration capped at 64 |
| `gmo.8044.word2c` | 3,432 | 266241..266442 | set enumeration capped at 64 |
| `gmo.8045.word00` | 5,005 | 1..175 | set enumeration capped at 64 |
| `gmo.8045.word04` | 5,005 | 0..3214435800 | set enumeration capped at 64 |
| `gmo.8045.word08` | 5,005 | 0..3225235654 | set enumeration capped at 64 |
| `gmo.8045.word0c` | 5,005 | 0..3258324934 | set enumeration capped at 64 |
| `gmo.8045.word10` | 5,005 | 0 | 0 ×5005 |
| `gmo.8045.word14` | 5,005 | 0..3258450776 | set enumeration capped at 64 |
| `gmo.8045.word18` | 5,005 | 0..3230318973 | set enumeration capped at 64 |
| `gmo.8045.word1c` | 5,005 | 0..3239064408 | set enumeration capped at 64 |
| `gmo.8045.word20` | 5,005 | 0 | 0 ×5005 |
| `gmo.8045.word24` | 5,005 | 0..3230164951 | set enumeration capped at 64 |
| `gmo.8045.word28` | 5,005 | 0..3213404286 | set enumeration capped at 64 |
| `gmo.8045.word2c` | 5,005 | 0..3248944214 | set enumeration capped at 64 |
| `gmo.8048.word00` | 300,598 | 0..3339990536 | set enumeration capped at 64 |
| `gmo.8048.word04` | 300,598 | 0..3357310412 | set enumeration capped at 64 |
| `gmo.8048.word08` | 300,598 | 0..3344594260 | set enumeration capped at 64 |
| `gmo.8049.word00` | 300,575 | 0..3226013660 | set enumeration capped at 64 |
| `gmo.8049.word04` | 300,575 | 0..3217625048 | set enumeration capped at 64 |
| `gmo.8049.word08` | 300,575 | 0..3226013660 | set enumeration capped at 64 |
| `gmo.804b.word00` | 44 | 0..3186993912 | 27 values; range at left |
| `gmo.804b.word04` | 44 | 0..3212836864 | 0 ×1; 687865856 ×1; 831577358 ×2; 863418693 ×1; 867941677 ×1; 1017133021 ×1; 1038740674 ×1; 1042733428 ×1; 1065289374 ×1; 1065353216 ×18; 3207922931 ×2; 3212836864 ×14 |
| `gmo.804b.word08` | 44 | 675282944..3211444827 | 26 values; range at left |
| `gmo.804b.word0c` | 44 | 843898404..3031363404 | 33 values; range at left |
| `gmo.804c.word00` | 300,617 | 996924723..3248141750 | set enumeration capped at 64 |
| `gmo.804c.word04` | 300,617 | 998520841..3225219821 | set enumeration capped at 64 |
| `gmo.804c.word08` | 300,617 | 897988541..3236574985 | set enumeration capped at 64 |
| `gmo.804e.word00` | 108,626 | 331776..332281 | set enumeration capped at 64 |
| `gmo.8061.word00` | 92,393 | 532480..532652 | set enumeration capped at 64 |
| `gmo.8062.word00` | 56,602 | 1..6 | 1 ×6679; 2 ×3924; 3 ×3066; 4 ×3907; 5 ×3280; 6 ×35746 |
| `gmo.8062.word04` | 56,602 | 0..173 | set enumeration capped at 64 |
| `gmo.8062.word08` | 49,923 | 0..174 | set enumeration capped at 64 |
| `gmo.8062.word0c` | 45,999 | 0..174 | set enumeration capped at 64 |
| `gmo.8062.word10` | 42,933 | 0..171 | set enumeration capped at 64 |
| `gmo.8062.word14` | 39,026 | 0..173 | set enumeration capped at 64 |
| `gmo.8062.word18` | 35,746 | 0..173 | set enumeration capped at 64 |
| `gmo.8066.word00` | 493,569 | 462848 | 462848 ×493569 |
| `gmo.8066.word04` | 493,569 | 3..260 | 3 ×917; 5 ×4369; 257 ×1; 259 ×70999; 260 ×417283 |
| `gmo.8066.word08` | 493,569 | 3..13440 | set enumeration capped at 64 |
| `gmo.8066.word0c` | 493,569 | 1..3593 | set enumeration capped at 64 |
| `gmo.8066.word10` | 493,569 | 0..114820823 | set enumeration capped at 64 |
| `gmo.8066.word14` | 5,286 | 0..114951897 | set enumeration capped at 64 |
| `gmo.8066.word18` | 917 | 3..327684 | 3 ×855; 65538 ×12; 65539 ×1; 131072 ×6; 196609 ×5; 196610 ×11; 262147 ×4; 327684 ×23 |
| `gmo.8066.word1c` | 841 | 0..458758 | 21 values; range at left |
| `gmo.8066.word20` | 841 | 0..524296 | 27 values; range at left |
| `gmo.8066.word24` | 839 | 1..589832 | 33 values; range at left |
| `gmo.8066.word28` | 764 | 1..720906 | 43 values; range at left |
| `gmo.8066.word2c` | 764 | 0..786444 | 42 values; range at left |
| `gmo.806b.word00` | 1 | 2 | 2 ×1 |
| `gmo.8081.word00` | 10,396 | 0..6 | 0 ×1098; 1 ×1642; 3 ×5805; 4 ×2; 5 ×1362; 6 ×487 |
| `gmo.8081.word04` | 10,396 | 0..1 | 0 ×9909; 1 ×487 |
| `gmo.8082.word00` | 38,602 | 0..1065353216 | set enumeration capped at 64 |
| `gmo.8082.word04` | 38,602 | 0..1065353216 | set enumeration capped at 64 |
| `gmo.8082.word08` | 38,602 | 0..1065353216 | set enumeration capped at 64 |
| `gmo.8082.word0c` | 38,602 | 0..1065353216 | 0 ×1; 1065353216 ×38601 |
| `gmo.8083.word00` | 150 | 1025548533..1065353216 | 1025548533 ×1; 1042062481 ×2; 1052424895 ×1; 1063642594 ×1; 1063675494 ×134; 1065353216 ×11 |
| `gmo.8083.word04` | 150 | 1025548533..1065353216 | 1025548533 ×1; 1042062481 ×2; 1057063292 ×1; 1063642594 ×1; 1063675494 ×134; 1065353216 ×11 |
| `gmo.8083.word08` | 150 | 1025548533..1065353216 | 1025548533 ×1; 1042062481 ×2; 1056372473 ×1; 1063642594 ×1; 1063675494 ×134; 1065353216 ×11 |
| `gmo.8083.word0c` | 150 | 1065353216 | 1065353216 ×150 |
| `gmo.8083.word10` | 150 | 1008981770..1090519039 | 36 values; range at left |
| `gmo.8084.word00` | 25 | 1065353216 | 1065353216 ×25 |
| `gmo.8084.word04` | 25 | 1065353216 | 1065353216 ×25 |
| `gmo.8084.word08` | 25 | 1065353216 | 1065353216 ×25 |
| `gmo.8084.word0c` | 25 | 1065353216 | 1065353216 ×25 |
| `gmo.8085.word00` | 38,591 | 0..1065353216 | set enumeration capped at 64 |
| `gmo.8085.word04` | 38,591 | 0..1065353216 | set enumeration capped at 64 |
| `gmo.8085.word08` | 38,591 | 0..1065353216 | set enumeration capped at 64 |
| `gmo.8085.word0c` | 38,591 | 0..1065353216 | 36 values; range at left |
| `gmo.8086.word00` | 38,555 | 0 | 0 ×38555 |
| `gmo.8087.word00` | 38,555 | 1065353216 | 1065353216 ×38555 |
| `gmo.8088.word00` | 38,555 | 0 | 0 ×38555 |
| `gmo.8091.word00` | 38,685 | 663552..663634 | set enumeration capped at 64 |
| `gmo.8092.word00` | 38,673 | 130..133 | 130 ×36889; 131 ×211; 133 ×1573 |
| `gmo.8093.word00` | 38,673 | 0..1065353216 | 28 values; range at left |
| `gmo.8094.word00` | 41,151 | 0..2 | 0 ×41131; 2 ×20 |
| `gmo.8094.word04` | 41,151 | 1..6 | 1 ×11; 6 ×41140 |
| `gmo.8094.word08` | 41,151 | 0..7 | 0 ×11; 1 ×2907; 7 ×38233 |
| `gmo.8097.word00` | 1,985 | 1 | 1 ×1985 |
| `gmo.8097.word04` | 1,985 | 1 | 1 ×1985 |
| `gmo.8098.word00` | 6 | 2147483648 | 2147483648 ×6 |
| `gmo.8098.word04` | 6 | 0 | 0 ×6 |
| `gmo.8098.word08` | 6 | 1065353216 | 1065353216 ×6 |
| `gmo.8098.word0c` | 6 | 1065353216 | 1065353216 ×6 |
| `gmo.8099.word00` | 1,985 | 1 | 1 ×1985 |
| `gmo.809a.word00` | 1,985 | 1065353216 | 1065353216 ×1985 |
| `gmo.809a.word04` | 1,985 | 0 | 0 ×1985 |
| `gmo.809a.word08` | 1,985 | 0 | 0 ×1985 |
| `gmo.809a.word0c` | 1,985 | 0 | 0 ×1985 |
| `gmo.809a.word10` | 1,985 | 0 | 0 ×1985 |
| `gmo.809a.word14` | 1,985 | 1065353216 | 1065353216 ×1985 |
| `gmo.809a.word18` | 1,985 | 0 | 0 ×1985 |
| `gmo.809a.word1c` | 1,985 | 0 | 0 ×1985 |
| `gmo.809a.word20` | 1,985 | 0 | 0 ×1985 |
| `gmo.809a.word24` | 1,985 | 0 | 0 ×1985 |
| `gmo.809a.word28` | 1,985 | 1065353216 | 1065353216 ×1985 |
| `gmo.809a.word2c` | 1,985 | 0 | 0 ×1985 |
| `gmo.80b1.word00` | 45,602 | 0 | 0 ×45602 |
| `gmo.80b1.word04` | 45,602 | 0..1243021984 | set enumeration capped at 64 |
| `gmo.80b2.word00` | 45,602 | 1106247680 | 1106247680 ×45602 |
| `gmo.80b3.word00` | 1,984,950 | 262146..4294967295 | set enumeration capped at 64 |
| `gmo.80b3.word04` | 1,984,950 | 72..152 | 72 ×172577; 75 ×1795579; 76 ×770; 130 ×23; 133 ×5799; 147 ×38; 152 ×10164 |
| `gmo.80b3.word08` | 1,984,950 | 0..3 | 0 ×1984835; 1 ×78; 2 ×30; 3 ×7 |
| `gmo.80b3.word0c` | 1,984,950 | 786432..786800 | set enumeration capped at 64 |
| `gmo.80e2.word00` | 1 | 0 | 0 ×1 |
| `gmo.80e2.word04` | 1 | 1 | 1 ×1 |

### Custom-data tags

| Tag | Occurrences | Payload byte range |
| --- | ---: | --- |
| `BlindData` | 4,588 | 28 |
| `EdgeLight` | 530 | 16 |
| `ShadowMap` | 899 | 16 |
| `UserProperty` | 2,116 | 16..40 |
| `per3Helper` | 12,295 | 52 |
| `transAlgo` | 38,580 | 16 |

### NECK_BENDER parameter object

Every parameter member is 32 bytes. Its interpretation is **OPEN U10**. These are u32 views, not a claim that four packed u16 angular limits are one integer. No pointer-looking word is executable or dereferenced.

| Offset | Observations | u32 range | Values / frequencies |
| --- | ---: | --- | --- |
| `+0x00` | 600 | 0..28 | 26 values; range at left |
| `+0x04` | 600 | 5242950..5570640 | 5242950 ×599; 5570640 ×1 |
| `+0x08` | 600 | 4915230..4915240 | 4915230 ×1; 4915240 ×599 |
| `+0x0c` | 600 | 4912472 | 4912472 ×600 |
| `+0x10` | 600 | 0 | 0 ×600 |
| `+0x14` | 600 | 0..28 | 26 values; range at left |
| `+0x18` | 600 | 5242950..5570640 | 5242950 ×599; 5570640 ×1 |
| `+0x1c` | 600 | 4915230..4915240 | 4915230 ×1; 4915240 ×599 |

## Scoped-reference evidence and explicit exceptions

| Reference/check | Count |
| --- | ---: |
| `reference.8041.True` | 295,220 |
| `reference.8044.True` | 155,270 |
| `reference.804e.False` | 16,233 |
| `reference.804e.True` | 92,393 |
| `reference.8061.True` | 92,393 |
| `reference.8066.True` | 493,569 |
| `reference.8091.True` | 38,685 |
| `reference.80b3.False` | 3,992 |
| `reference.80b3.True` | 3,965,908 |
| `reference.owner_context.804e.False` | 233 |
| `reference.owner_context.804e.True` | 16,000 |
| `reference.owner_context.80b3.True` | 638 |
| `reference.sentinel.target_command.0048` | 1,278 |
| `reference.sentinel.target_command.004b` | 1,253 |
| `reference.sentinel.target_command.0082` | 1 |
| `reference.sentinel.target_command.0085` | 701 |
| `reference.sentinel.target_command.0098` | 112 |

The local-scope failures are **not** framing failures. There are 16,233 DrawPart refs in supplemental neck GMOs, 638 neck material targets, 3,345 FFFFFFFF target sentinels, and nine other bone targets. Candidate enclosing-owner lookup resolves 16,000 DrawPart and all 638 material targets; **233 part references remain unresolved**. They must not be clamped or silently omitted. Sentinels, external context and the remaining targets are U02/U07/U10. Full source paths/offsets for every occurrence remain in the owner-local contextual receipt.

### Nine non-sentinel/non-neck bone-target exceptions

| Source key | GMO offset | Reference |
| --- | ---: | --- |
| `data.cpk::model/persona/ps046.AMD::MODEL_DATA` | `0x32040` | `0x0004010F` |
| `data.cpk::model/persona/ps051.AMD::MODEL_DATA` | `0x28560` | `0x0004092A` |
| `data.cpk::model/persona/ps051.AMD::MODEL_DATA` | `0x2E878` | `0x0004092A` |
| `data.cpk::model/persona/ps051.AMD::MODEL_DATA` | `0x34F3C` | `0x0004092A` |
| `data.cpk::model/persona/ps08A.AMD::MODEL_DATA` | `0x26768` | `0x0004010A` |
| `data.cpk::model/persona/ps095.AMD::MODEL_DATA` | `0x25194` | `0x00040064` |
| `data.cpk::model/persona/ps095.AMD::MODEL_DATA` | `0x2C95C` | `0x00040064` |
| `data.cpk::model/persona/ps0A5.AMD::MODEL_DATA` | `0x24614` | `0x00040002` |
| `data.cpk::model/persona/ps0A5.AMD::MODEL_DATA` | `0x2ED78` | `0x00040002` |

## Motion-slot evidence

32 is the party MODEL0 record count, not a generic GMO clip capacity. All 4,281 nonempty model motion lists start unsuffixed; 1,120 model blocks have no motions. There are 42,434 suffixed entries: 42,393 agree with suffix=index-1, **41 do not**; another **81** noninitial entries have no numeric suffix. The [complete naming exception list](model-formats-motion-exceptions.md) gives all 122 entries across 49 GMO objects. Names alone are not a total runtime slot map (U08).

| Property | Count/range | Distribution |
| --- | --- | --- |
| `motion.name_suffix` | 42,434: 0..33 | 34 values; range at left |
| `gmo.model.motion_count` | 5,401: 0..35 | 36 values; range at left |
| `gmo.motion.curves` | 46,796: 0..369 | set enumeration capped at 64 |

## Embedded-image and high-resolution association census

Container/pixel rules and the explicit OPEN binding/default-embedded decision are in [the texture sub-spec](model-formats-textures.md). The UTF-8 replacement inventory names were recovered from raw TOC cells using TOC indices and a strict CP932 candidate interpretation; there is no claim that replacement characters are valid runtime keys.

| Measurement | Count |
| --- | ---: |
| `binding.both_different.False` | 25,750 |
| `binding.embedded.any_dds.False` | 6,157 |
| `binding.embedded.any_dds.True` | 19,590 |
| `binding.external.any_dds.False` | 3 |
| `binding.filename_ascii.False` | 20 |
| `binding.filename_ascii.True` | 25,730 |
| `binding.filename_found.False` | 6,160 |
| `binding.filename_found.True` | 19,590 |
| `binding.name_ascii.False` | 20 |
| `binding.name_ascii.True` | 25,730 |
| `binding.name_equals_filename.False` | 83 |
| `binding.name_equals_filename.True` | 25,667 |
| `binding.name_found.False` | 6,234 |
| `binding.name_found.True` | 19,516 |
| `dds.conflicting_stems` | 0 |
| `dds.distinct_stems` | 3,790 |
| `dds.inventory_objects` | 3,790 |
| `dds.matched_distinct_stems` | 3,789 |
| `dds.non_ascii_names` | 9 |
| `image.BMP` | 5 |
| `image.TIM2` | 25,742 |
| `tim2.base_image_size.True` | 25,742 |
| `tim2.file_reserved_zero.True` | 25,742 |
| `tim2.palette_alpha_over128.False` | 115 |
| `tim2.palette_alpha_over128.True` | 25,602 |
| `tim2.palette_partial.False` | 23,856 |
| `tim2.palette_partial.True` | 1,861 |
| `tim2.variant.3.0.0.1.0.2` | 7 |
| `tim2.variant.3.0.0.1.0.3` | 18 |
| `tim2.variant.3.0.0.1.3.4` | 2,066 |
| `tim2.variant.3.0.0.1.3.5` | 23,651 |

| TIM2/BMP field | Observations | Range | Values / frequencies |
| --- | ---: | --- | --- |
| `arrays.extra_uv_sets` | 92,393 | 0..3 | 0 ×90509; 1 ×1531; 2 ×342; 3 ×11 |
| `bmp.bpp` | 5 | 24 | 24 ×5 |
| `bmp.colors` | 5 | 0 | 0 ×5 |
| `bmp.compression` | 5 | 0 | 0 ×5 |
| `bmp.dib_bytes` | 5 | 40 | 40 ×5 |
| `bmp.file_bytes` | 5 | 6198..12342 | 6198 ×1; 12342 ×4 |
| `bmp.height` | 5 | 32..64 | 32 ×1; 64 ×4 |
| `bmp.image_bytes` | 5 | 0 | 0 ×5 |
| `bmp.important` | 5 | 0 | 0 ×5 |
| `bmp.pixel_offset` | 5 | 54 | 54 ×5 |
| `bmp.planes` | 5 | 1 | 1 ×5 |
| `bmp.reserved` | 5 | 0 | 0 ×5 |
| `bmp.trailing_bytes` | 5 | 0 | 0 ×5 |
| `bmp.width` | 5 | 64 | 64 ×5 |
| `bmp.xppm` | 5 | 64 | 64 ×5 |
| `bmp.yppm` | 5 | 32..64 | 32 ×1; 64 ×4 |
| `bone804b.quaternion_norm2` | 44 | 0.9999999554546122..1.0000000548678565 | 26 values; range at left |
| `image.command_padding` | 25,747 | 0..2 | 0 ×25742; 2 ×5 |
| `tim2.clut_bytes` | 25,742 | 0..1024 | 0 ×25; 64 ×2066; 1024 ×23651 |
| `tim2.clut_type` | 25,742 | 0..3 | 0 ×25; 3 ×25717 |
| `tim2.colors` | 25,742 | 0..256 | 0 ×25; 16 ×2066; 256 ×23651 |
| `tim2.gsclut` | 25,742 | 0 | 0 ×25742 |
| `tim2.gsreg` | 25,742 | 0 | 0 ×25742 |
| `tim2.header_bytes` | 25,742 | 48 | 48 ×25742 |
| `tim2.height` | 25,742 | 8..512 | 8 ×23; 16 ×1420; 32 ×3650; 64 ×4618; 128 ×12762; 256 ×2599; 512 ×670 |
| `tim2.image_bytes` | 25,742 | 32..262144 | 19 values; range at left |
| `tim2.image_type` | 25,742 | 2..5 | 2 ×7; 3 ×18; 4 ×2066; 5 ×23651 |
| `tim2.mips` | 25,742 | 1 | 1 ×25742 |
| `tim2.palette_alpha_max` | 25,717 | 85..255 | 39 values; range at left |
| `tim2.palette_alpha_min` | 25,717 | 0..255 | 34 values; range at left |
| `tim2.picture_format` | 25,742 | 0 | 0 ×25742 |
| `tim2.picture_padding` | 25,742 | 0 | 0 ×25742 |
| `tim2.pictures` | 25,742 | 1 | 1 ×25742 |
| `tim2.style` | 25,742 | 0 | 0 ×25742 |
| `tim2.tex0` | 25,742 | 0 | 0 ×25742 |
| `tim2.tex0_cpsm` | 25,742 | 0 | 0 ×25742 |
| `tim2.tex0_csa` | 25,742 | 0 | 0 ×25742 |
| `tim2.tex0_csm` | 25,742 | 0 | 0 ×25742 |
| `tim2.tex0_psm` | 25,742 | 0 | 0 ×25742 |
| `tim2.tex1` | 25,742 | 0 | 0 ×25742 |
| `tim2.total` | 25,742 | 144..263216 | 32 values; range at left |
| `tim2.trailing_bytes` | 25,742 | 0 | 0 ×25742 |
| `tim2.version` | 25,742 | 3 | 3 ×25742 |
| `tim2.width` | 25,742 | 8..512 | 8 ×20; 16 ×42; 32 ×4682; 64 ×11629; 128 ×6241; 256 ×2453; 512 ×675 |

## Remaining unknowns (not silently supported)

* **U01:** Euler order/axes/winding and exact quaternion conventions.
* **U02/U07/U10:** sentinel target semantics, supplemental owner binding, 233 remaining part refs, and nine bone-target exceptions above.
* **U03/U04:** full primitive, quantization-state, colour, normal and bind-matrix semantics; unsupported future scalar formats. All observed packing lengths are classified, not all numerical render behavior proved.
* **U05/U06:** unresolved material/layer enums; TIM2 palette/pixel/alpha qualification; high-resolution source priority and non-ASCII filename semantics. DDS name association is OPEN, not 100% coverage.
* **U07/U08/U15:** target options, loop/end-point/empty-motion rules, complete base/slot map, 122 naming exceptions, and arithmetic/interpolation behavior.
* **U09/U10:** helper transform application, head tag and neck parameters. All helper payloads are structurally classified.
* **U11:** both EFFECT_EPL tags plus TRACK/ADDMOT/EDGE internals and lifetime/attachment coupling (M8f seam).
* **U13:** AMD/GMO/neck suffix semantics and retained zero/reserved fields.
* **U14:** 8021/8022/8023 declarations, 806B, 80E2, generic BlindData, and any command-prefix field not assigned a semantic name in the parent spec. Tool-only objects may be unreachable in legal retail BBV.

Each U id has a proposed owner BBV card in the parent spec. No BBV, native Golden rendering, PS2 runtime regression or remote lane receipt is claimed here.
