# M8f P2A, TLP0 and content-hash texture binding

All offsets/sizes are hex unless stated otherwise. Source data are little-endian. This page specifies embedded EP adjunct2(P2A), adjunct7(TLP0) and the global HFLC binding directory. It is not a new image decoder: valid TMX/DDS images use the real Golden image path specified by M8a. The embedded-family hashes, logical counts and parent-archive attribution are in [m8f-adjunct-census.md](m8f-adjunct-census.md).

## Evidence and boundaries

Switch1.2 ordinary NSO paths: P2A362E60(construction),362FE0(stream selection),363300(release),3633A0(frame evaluation),3635C0(seek),363610(texture access); TLP383960(construction),383C30(release),383D30(selection); hash lookup32A080; graphics initialization322FB0; external request398750,398820,3988B0,3988F0. Fixed source identities are those on the master M8f page. PS2 `effP2A.c`/`effTLP.c` are comparison material, not a substitute Golden native ABI.

The full Steam10836033 adjunct pass extracted each distinct root asset through its actual CPK/archive layers and checked its SHA-256 against the inventory. Result:43 distinct P2A payloads and23 TLP0 payloads; no traversal/bounds exceptions. Logical physical-descriptor occurrences are702 P2A and188 TLP0 across the9 effect-bearing CPKs. Parent counts/hash manifest are in the annex. The initial instrumentation launch failed before extraction because the isolated helper namespace lacked `__file__`; it was corrected before the reported pass, which completed in63.21 seconds. These are static data measurements, not executed-game receipts.

## P2A adjunct2

There is no leading P2A magic in this wire representation. Identify it through the EP adjunct kind, not extension or a four-byte text signature.

Header: signed32 source-relative texture-directory offset+0; signed32 stream count+4. Stream directory starts+8 and has count records of14 bytes. Each stream record:

| Offset | Type | Meaning |
|---|---|---|
| 0 | u32 | Source-relative frame/group payload offset |
| 4 | u32 | Source-relative per-frame scalar-array offset, used when control bit0 ison |
| 8 | u32 | Stream-selection flags; maskC0 selects the paired-stream initialization path |
| C | u32 | Frame count, or child count for a group |
| 10 | u32 | Playback controls; bit10000000 marks a group |

Texture directory: u32 count, followed immediately by count u32 **source-relative** texture offsets. These offsets are not relative to the directory. The native owner copies source prefix `[0,textureDirectoryOffset)`, creates full-width texture handles from the original texture offsets, and keeps a native handle array; never relocate the wire offset words into32-bit host pointers.

Ordinary frame records are18 bytes:

| Offset | Type | Meaning/evaluation |
|---|---|---|
| 0,2 | s16 | Width,height |
| 4,6 | s16 | OriginX,originY |
| 8,A,C,E | s16 | Left,top,right,bottom UV coordinates in four-fractional-bit fixed point |
| 10 | s16 | Texture-directory index |
| 12 | s16 | Additional hold countdown |
| 14 | f32 | Per-frame scalar forwarded to the draw-state field+10; consumers determine its rotation/value use |

Resolved output center=(width*0.5-originX,height*0.5-originY), halfExtent=(width*0.5,height*0.5). For UVs, first perform an **arithmetic signed right-shift4**, then convert tofloat and divide X/Y by the resolved texture's width/height. Do not divide the original fixed value by16 in floating point: negative rounding differs. Output includes the full-width texture handle and actual decoded texture dimensions.

Control word+10 bit0 selects a f32 at `sourceBase+record[4]+frameIndex*4`; when off the source writes a negative-NaN sentinel, not scalar0. Bit1 selects output mode2; otherwise bit2 selects mode3; otherwise mode1. Bit10(loop termination mask, hexadecimal10) selects stop-at-last-frame rather than wrap. These are control-word bits, **not** stream-selection flags+8.

Stream selection initializes frame0 and countdown from the first frame's signed hold. Ordinary playback3633A0: while countdown>0 subtract0.5*liveFrameStep and hold the current frame; otherwise increment the floating frame cursor by0.5*liveFrameStep. If it reaches frameCount, wrap0 unless control mask10 ison, in which case clamp frameCount-1 and clear playback-active. Load the newly selected frame's hold. The frame index used for output is trunc(cursor). Do not add the signed hold to the frame count or advance the cursor once per render call. Seek3635C0 resets the relevant frame cursor using the requested time modulo frameCount and installs the source's small positive countdown; it is not a destructive resource reload.

A group record(control10000000) has C child entries of18 bytes at its payload offset. Each child begins s16 streamIndex and s16 startDelay; the remaining14 bytes are preserved. Initialization selects each referenced ordinary stream, loads its first hold and sets its cursor to negative startDelay. A paired-stream selection(flags8&C0) initializes the requested stream and the immediately following stream, with independent native playback states. Bounds-check both indices, including the second stream. The43 measured Steam P2A identities all have one ordinary stream,2..46 frames and selection flags20 or120; no measured group/paired instance. Controls observed:0,1,2,3,10,12,13. Every frame array and every enabled scalar array fits the native copied prefix before the texture directory, independently checked after the pass.

Native P2A texture creation is through384B60 for each directory entry. Do not assume that a directory-relative slice through the next texture offset is the byte count used by the generic EP hash lookup: image parsing determines its own valid payload. P2A metadata, texture resources and playback instances have distinct ownership; release playback/texture references before freeing their resource owner.

## TLP0 adjunct7

Header10: magic bytes`TLP0`,u32 frame-entry count+4,u32 flags+8,u32 preserved+C. Count records of10 bytes follow. Entry+0 is signed32 holdMinusOne, +4 is u32 embedded texture byte count, +8/+C are preserved words. After the complete record directory, texture payloads are concatenated in entry order, each consuming its own declared byte count. No per-entry offset table or alignment rounding is implied.

Native total duration=sum(holdMinusOne+1).383D30: one entry selects index0. Otherwise flags bit0 loops the requested integer index modulo total duration; without it, an index>=total selects the last entry. Scan cumulative durations: select an entry when `index<=accumulated+holdMinusOne`; then accumulate holdMinusOne+1 if not selected. The endpoint comparison is inclusive. Flags bit2 selects output mode2 rather than1. Output texture width/height come from the real texture;33B160's applicable texture flag halves both. Selection does not itself advance the caller's clock.

All23 measured identity headers use flags1A,1B or1F; entry preserved words+8/+C arezero. HoldMinusOne values observed:0,1,2,3,4,5,12,20,27,50,100,496. Twenty-two identities consume the complete adjunct span. One valid payload, `data.cpk::skill/17D.pac::17D.BED` bank0 slot2 part50, declares1984 decimal bytes, of which the header/two entries/textures consume1328, leaving656 preserved bytes. Accept the bounded declared prefix and retain this trailer; do not reject it or guess another active frame. Trailer semantics remain OPEN.

Each texture is passed with its **entry-declared size** to the external content lookup below; if no external request is obtained, the native constructor uses the embedded texture loader. Native TLP resource uses a refcount and per-entry external-request/texture slots. Shared immutable TLP backing does not imply shared playback clocks. Destroy/cancel outstanding request references and release textures before freeing the owner. Do not free a request-backed buffer while its image upload/consumer still borrows it.

## HFLC global registry: four installed variants

The ordinary graphics initialization loads `FILELIST_hash.hflc` through the active packed-file resolver. Do not concatenate locale tables: record indices address the **same selected file's** name array. Use existing Golden locale/mount precedence for that exact lookup; do not hardcode the neutral registry when a localized overlay supplies it.

| CPK | Bytes(decimal) | Entries(decimal) | SHA-256 |
|---|---:|---:|---|
| data.cpk | 5971284 | 10511 | `ac5bac457dc07fbfe3a68fce8887c2f07e435b4d932d80d701821a94e1aa2128` |
| data_c.cpk | 5998548 | 10559 | `37d4b0840323dd05dd2f7b80ceb51540ce3bc2b935a703ca8ad1aaf7223ce578` |
| data_e.cpk | 6248468 | 10999 | `3ea70e0b96200e2b305b1d6d54844fe3d0f35750635dfa1a7d1dba7597b95de6` |
| data_k.cpk | 6154180 | 10833 | `54932a83fda61557380d7d09396f5411fcc32d6509cff32e66219f5c29255788` |

HeaderC: magic`HFLC`,u32 version0,u32 entryCount. Starting+C, read256 sequential buckets, each u32 recordCount followed by that many18-byte records. Each record: raw16-byte MD5 digest+0,preserved u32+10(index padding, observedzero),u32 nameIndex+14. After all buckets, read entryCount name records of220 bytes: char firstName[104],char secondName[104],raw16-byte digest+208,preserved u32+218,u32 originalPayloadBytes+21C. Names are bounded NUL-terminated byte strings; retain indices and wire ordering.

All4 selected files passed complete sequential bounds/end-size checks: sum(bucketCounts)==entryCount and namesEnd==fileSize. Digest first byte agrees with bucket number except for an observed **all24-byte-zero record in bucket250**. It is a real empty slot, not permission to allow arbitrary invalid buckets/indices. Preserve it in wire order; it cannot match a bucket250 query. Index0 is otherwise valid and must not be globally suppressed.

### Exact lookup and the surprising comparison

32A080 hashes the supplied **raw buffer and supplied byte length**, using the ordinary MD5 init/update/final paths3323F0/332F10/333000. It does not hash the requesting BED/EPL path, an image's decoded pixels, a filename stem or a renderer resource id.

Select bucket by the original digest's first byte. Append a NUL terminator to the16 raw digest bytes. If its first byte iszero, compare starting at byte1; otherwise start at0. Apply the same leading-zero adjustment to each candidate record's digest. The native comparison is **strcmp**, not a16-byte memcmp: stop at the next embedded zero. Resolution is the first equal candidate in wire order with a valid nameIndex. Do not silently replace this behavior with a dictionary keyed by the complete16 bytes or choose the last duplicate. The PLT/GOT relocation for NSO530B20 resolves to imported `strcmp` at GOT87F7A0, independently decoded through the NSO MOD0 dynamic/symbol/relocation tables.

On success copy the selected bounded firstName/secondName. The texture constructor searches the first name for `.EPL`, otherwise `.BED`. If found, locate the **first backslash after that extension**, then copy that suffix over the extension position. Prefix `tex2`. This removes the container extension without removing the container stem. Normalize separators/case only according to the existing packed-file resolver; do not perform arbitrary Unicode folding or drop nested directories. The native source uses imported strcpy for the suffix copy; an implementation may perform the equivalent bounded overlap-safe string transformation.

### R_BLOCK binding witness, proved from installed bytes

The first bank1/slot0 ordinary particle's texture payload is1344 decimal bytes, MD5`3b76295d09e95d9b6a03b5e909c3d16c`. Neutral HFLC's first match is nameIndex565: firstName=`\battle\effect\R_BLOCK.BED\000.TMX`, secondName=`000.TMX`. The rule above produces `tex2\battle\effect\R_BLOCK\000.TMX`. Inventory resolves it to `data.cpk::tex2/battle/effect/R_BLOCK/000.tmx`,4160 decimal bytes, SHA-256`d657c3d432ee3034885e9b98e1fede0cfefe9e2f11a50060cf70a2b7c865c049`. The128448-byte BED identity and canonical common-entry48 ownership remain as specified on the master page. The same embedded identity was measured inside battle_pack's btleff copy; do not install that copy as an additional common entry.

This replaces the unproved filename-association hypothesis with a concrete content-hash/registry/tex2 chain. The static survey found471 distinct texture slices, of which429 came from generic adjunct1/TLP rather than the separate P2A-directory slicing probe. Using the **neutral** registry only,356 of those429 had a native-comparison match and348 had a matching target anywhere in the inventory. These are not locale-qualified success rates; localized inputs require their selected registry. Eight matched generic/TLP paths had no inventory target. The native absent-file path may legally use embedded data; a hash-table hit alone is not proof of an installed external image.

## External request and install contract

398750 first checks the active pack and target existence. Only then does it create a request wrapper and issue the read. A nonexistent external target returns no wrapper, enabling the constructor's embedded path.398820 returns-1 for a failed request,0 pending/not present,1 ready; its blocking option services the native read machinery while waiting.3988B0 returns the completed buffer, not a rendered texture.3988F0 releases the request references. Do not interpret request submission, name resolution, a ready-null image or a pending request as successful texture installation.

Portable owner contract: retain the selected registry generation for lookups; retain source bytes while parsing and any image decoder borrows them; retain an external buffer through image creation; use full-width independent image handles. Resource sharing may share immutable decoded images, keyed at least by resolved source/generation and decode settings, but not share emitter/TLP/P2A playback state. Destroy effects and their image/request references before retiring registry/backing generations. Do not copy native pointer words back into wire buffers. Existing M8a decoding/upload and existing Golden packed-file resolution are required dependencies, not white-sprite substitutes. A terminal read/decode error must remain an error/refusal with provenance; this spec adds no retry or unrelated fallback policy.

## Explicit remaining adjunct/binding items

- P2A group/paired playback consumer edge branches are parser-proven but not exercised by any of the43 measured identities; this page does not invent their draw-composition semantics.
- Frame+14's universal semantic name and every geometry consumer's interpretation remain tied to the primitive draw schema, not guessed from the float type.
- TLP17D's656-byte preserved trailer remains uninterpreted.
- A complete per-locale resolver/renderer qualification matrix and asynchronous image lifetime witnesses belong to implementation acceptance. Static binding mechanism and R_BLOCK target are established; no runtime claim is made.
