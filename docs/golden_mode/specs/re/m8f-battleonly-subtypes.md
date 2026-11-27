# M8f BattleOnly subtypes and R_BLOCK animated lighting

This extends [the SDK lifecycle](m8f-battleonly-lifecycle.md) with the actual battle callback registration, primary schemas, common transition envelope and subtype6's complete animated-light controller. Offsets are hex. Numbers in explicitly decimal measurement tables are decimal. Source addresses are ordinary Switch1.2 NSO, not Steam protector code.

## Registration, fixed table and version domains

4E7E0 loads table71BEA0 and predicate4E800, then tail-jumps to34FD50. The latter stores the predicate/table pair at51C8AE0/51C8AE8.4E800 resolves GOT880528 to885638 and returns whether the full-width current-battle pointer is nonnull. This is a real battle-generation dependency, not a condition inferred from an asset filename.

Table bytes71BEA0..71C178:728 decimal bytes, SHA-256`0dcdff04e5c909e3168be68011192b6248e5735412ffe8934e683eca4e9aa030`. Resolve the ordinary RELATIVE relocations before reading pointer fields. All init and draw entries are genuinely null; all row padding words arezero. The graphics/model/environment work is issued by the **update** callbacks; null draw callbacks are not proof that the effect is absent.

| Subtype | Create | Destroy | Clone | Update | Primary allocation bytes(decimal) |
|---:|---|---|---|---|---:|
| 0 | null | null | null | null | 0 |
| 1 | 4E820 | 4E8C0 | 4E8F0 | 4E950 | 48 |
| 2 | 4ED40 | 4ED70 | null | 4ED80 | 16 |
| 3 | null | 4F0C0 | null | 4F0D0 | 16 |
| 4 | 4F470 | 4F4A0 | null | 4F4B0 | 16 |
| 5 | 4F870 | 4F8A0 | null | 4F8B0 | 16 |
| 6 | 4FB40 | 4FBE0 | 4FC10 | 4FC70 | 40 |
| 7 | null | null | null | 50110 | 32 |
| 8 | 50260 | 50300 | 50330 | 50390 | 48 |
| 9 | 50860 | 50920 | 50950 | 509F0 | 88 |
| 10 | null | 50C00 | null | 50C20 | 108 |
| 11 | null | 50C90 | null | 50CA0 | 40 |
| 12 | null | 50EA0 | null | 50EB0 | 48 |

A dedicated recursive census measured the exact version/subtype/declared-primary/adjunct tuples over distinct root identities. Versions200 and210 use identical observed primary sizes for each shared subtype:1=48,2=16,3=16,4=16,5=12,6=40 decimal. Version210 additionally has7=32,8=48,9=88,10=108,11=40,12=48. Subtypes1/6 can have adjunct3 or4;8/9 use3; all other measured rows use4.

**Subtype5 is a real size exception:** every measured declared primary is12 bytes, although the SDK table reserves/copies16.4F8B0 reads only its three declared words. A typed decoder must parse the12 semantic bytes and must not overrun the declared span simply to reproduce the SDK's unused extra four-byte copy. No current schema field at+C is established for5. This is not a reason to reject those installed assets or invent another version.

## Shared unit-selection prefix

50EC0 consumes four bytes. The context source/target pointers are885600/885608; three additional explicitly selected units are885610/885618/885620. Units have genus byte+A2. Evaluate in the following order, preserving early returns:

1. Byte3 nonzero selects every unit.
2. Byte2 bits0/1/2 select an exact match to the corresponding nonnull additional pointer; any matching bit returns true.
3. If source exists and unit.genus==source.genus: byte0==0 returns false immediately; byte0==1 returns unit==source immediately; byte0==2 returns true immediately; other values fall through.
4. If target exists and unit.genus==target.genus: byte1==1 returns unit==target; byte1==2 returns true.
5. Otherwise false.

In particular source and target genus may coincide; do not turn this into an unordered OR of two filters. Four battle unit lists are walked in source order, heads at native battle+1C8+i*10, next link native unit+D00. These offsets identify the Golden owner; do not write into a PS2 struct using them.

## Shared duration/fade envelope

For the subtypes using it, D is u32 duration, I/O are u16 fade-in/fade-out, and t is the instance's floating clock. Each has a per-instance final-applied byte initially0. On the first t>=D, set that byte and use local t=D for that call. On later calls with D!=0 and t>D, return. This ensures one final boundary evaluation even after a frame jump. D0 does not mean a synthetic blank or universal immediate completion.

When D!=0, t<=I is fade-in, with w=t/I or1 if I0. Otherwise t>=u32(D-O) is fade-out, with w=(D-t)/O or0 if O0. Otherwise hold. D-O is the original unsigned subtraction; do not clamp it or silently swap overlapping fade ranges. With D0 use the hold/initial branch. Fade-in takes precedence over fade-out.

The source captures the current affected value at t0 for fade-in, then writes lerp(snapshot,target,w). Fade-out controllers shown below capture the current value **again on each fade-out update**, then write lerp(baseline,currentSnapshot,w). Do not replace this by a once-only fade-out snapshot: interleaved effects and fractional steps differ. Hold writes the target only at t0 unless a subtype explicitly has another operation. Packed colors unpack in byte order0/8/16/24 and normalize by255; unit packed-byte writes use their stated rounding, not one global PS2 half-alpha rule.

## Primary schemas and target operations

Preserve copied tails not read by the proven callback. Do not assign semantic fields merely because the allocation is large.

### 1: selected-unit spatial color control

Primary30: selection bytes0..3; preserved4..B; D+C; I+10; O+12; packed target+14; f32 modelPlaybackRate+18; f32 modelScale+1C; preserved20..2F.

Optional adjunct3 model wrapper owns one full-width GMO/model handle. Update its orientation from instance quaternion, scale=instanceScale*modelScale, position=instanceXYZ and channel0 playback rate; advance the real model. Query helper0's XYZ; if absent use instanceXYZ. For every selected unit write that origin to native unit+54/+58/+5C, independently of transition phase. The current four-float spatial-color control is unit+70..7F; snapshot is+60..6F. Apply the common envelope to the target, restoring the engine's default color through GOT880648 on fade-out. This is distinct from subtype3's packed material tint; do not collapse the two controls into one multiply.

### 2: battle scene color pair

Primary10: D+0,I+4,O+6,packed targets+8/+C. Wrapper owns two four-float snapshots. Current pair is native battle+280/+290 (606C0); baseline pair is battle+2B0/+2C0 (606E0). Apply the common envelope componentwise to the two **absolute** target colors. D0 writes both targets at t0. This updates the battle's scene-color owner, not a private per-effect renderer constant.

### 3: selected-unit packed tint

Primary10: D+0,I+4,O+6,packed target+8,selection bytes+C..F. For selected units, set flags98 bit2. Current packed tint is unit+3C; snapshot+38. Apply the envelope to each byte, converting normalized results with trunc(value*255+0.5) before storing. Fade-out baseline is packedFFFFFFFF (immutable86FAC0 via GOT880650). Hold assigns target only at t0; the flag remains refreshed. Do not reinterpret this packed tint as subtype1's floating spatial color.

### 4: scene-renderer color-pair multipliers

Primary10 as2. Current outputs are the actual renderer group12 color quads returned by29E0D0/29E100 (group object+160/+170). Baseline is native battle+250/+260 (606A0). Fade-in target is **baseline multiplied componentwise by the unpacked target colors**, blended from the captured live outputs. Fade-out recaptures live outputs and blends toward the baseline. D0 initially writes baseline*target. Thus4 is not equivalent to2's absolute pair replacement.

### 5: global scene color factor

Declared primaryC: D+0,I+4,O+6,packed target+8; SDK reserved size10. Current factor is native battle+2E0 (60700). Apply the envelope to RGB; W is forced1. Fade-out RGB baseline is white. The real29DDF0 application visits renderer groups12 and10, updates model/material color control, toggles group flag2 according to positive RGB/W, and updates actual renderer color bytes with upper clamp255. Do not update only an unused effect-local vector or infer the target's packed alpha is a fourth interpolated factor here.

### 6: animated shared-light pair — R_BLOCK

Primary28: D+0,I+4,O+6,packed absolute colors+8/+C,f32 modelPlaybackRate+10,f32 modelScale+14,preserved18..27. The instance owns a28-byte child wrapper: full-width model handle followed by two four-float color snapshots. Adjunct3 and a valid model header load the real model; adjunct4 legitimately leaves the model handle absent while still applying colors.

Update optional model orientation, position and scale from the instance and primary. Set channel0 playback rate, then advance the model and query helper0's **matrix**. This is a helper animation driving the shared light transform, **not a request to render that helper GMO as a visible mesh**. Apply the common color envelope to the shared effect lighting state at51C8DE0 (GOT880658): color0+40,color1+50,dirty+60. Fade-out baseline is battle+250/+260. On successful helper lookup copy its matrix to the lighting state's00..3F. The source does not multiply these parameter colors by the instance packed color.

385290/3852A0 commit dirty lighting through the actual renderer color endpoints39E5F0/39E610 and matrix transform3368E0, then clear dirty. This frame-stage consumer is required: merely caching a matrix/color in a BattleOnly node does not make R_BLOCK lighting visible. Preserve effect/update/render-stage ordering; two instances share this real scene-light target, while their snapshots/model clocks remain independent.

Preserve the dirty-bit operation boundary: successful helper matrix copy by itself does not explicitly set dirty in4FC70. Color writes set it, and the normal battle/environment owner can also set it. Do not add an unconditional dirty write or omit the source frame-stage consumer; retain their observed ordering.

R_BLOCK's40-byte primary at BED-relative1D2C is:

`3c00000000000500ffffff80fffafae6ffff7f3fffff7f3f00000000000000000000000000000000`.

Decoded: D60 decimal,I0,O5,colors80FFFFFF/E6FAFAFF,rate/scale both binary32 bits3F7FFFFF (~0.99999994),remaining16 byteszero. Native4FD20..4FD70 loads rate/scale with LDR S, not integer-to-float conversion. Primary SHA`8fb8cad4bfe88437e1ef01f254f2539f889ac4cabeaaa2d3aa908219c8df9c94`;1648-byte OMG helper adjunct SHA`135fb6fc4a272f4d2fd9cfb642366191a1599ff35701c14aea65f1363eaa26c4`.

The model bridge361BD0 reserves a free id in the real type6 model namespace, constructs from the actual buffer/size, initializes existing motion state, clears model flag408 bit1 and registers361BC0.361CE0 makes a real independent native model clone. Channel430 is a **playback-rate control**, not a fixed animation-frame index: native388DDC..388E00 advances with frameStep*rate/60*2 through3904D0. Reuse M8c model/helper/motion ownership, never a new blank model or synthetic helper matrix.

### 7: selected-unit motion command

Primary20: selection0..3,u16 motionId+4,u16 option+6,f32 playbackRate+8,u16 blendTicks+C,u8 preserveLayerState+E,preserved+F,u32 postSetReset+10,preserved14..1F. Selected units must additionally pass the original current-action predicate. Every eligible update sets unit+C28 bit3. At t0, if preserveLayerState0 clear layer4, then issue the real unit motion command(rate,motionId,blendTicks,option), optionally its post-set reset, and enable layer4. Later updates only refresh the holding flag. This is actor animation control, not a sprite emission record.

### 8: helper-driven selected-unit pose

Primary30: selection0..3,D+4,f32 modelPlaybackRate+8,f32 modelScale+C,u32 restoreMotionAtEnd+10,preserved14..2F. Wrapper owns an optional adjunct3 model. Advance it with instance transform and rate; query helper0 matrix. Apply its translated XYZ and scale-normalized orientation to selected eligible units across all four lists. The uniform scale removed for quaternion extraction is instanceScale*modelScale; preserve the original matrix product/order. If t>=D and restoreMotionAtEnd!=0, the source instead restores the selected eligible units' base pose/rest motion in the first two unit lists. Do not apply one guessed transform only to a visual clone while leaving the actual Golden presentation-unit pose untouched.

### 9: helper-pair camera pose

Primary58: preserved00..2B,D+2C,f32 modelPlaybackRate+30,f32 modelScale+34,u16 cameraSetupMode+38,preserved3A..57. Constructor loads the actual adjunct3 model, computes pairCount=helperCount>>1, and, if nonzero, chooses firstHelper=2*(I()%pairCount). That one effect-RNG draw belongs to construction, not every frame; retain the chosen id in the child wrapper. Clone preserves the selected pair with an independent model instance.

At t0 issue camera setup kind1, option0 when cameraSetupMode!=0, otherwise option1 and its source reset35930(0,0,0,0,100). While the real camera mode getter returns1 and model exists, advance the model using instance transform/rate, query selected helper and helper+1, construct camera pose from their eye/target translations and the first helper's up basis, then publish it to the Golden camera owner. Missing helper IDs take the source diagnostic path, not identity matrices. Exact camera facade/coordinate binding is an explicit integration item below.

### 10: FOV interpolation

Primary6C: D+0,f32 startDegrees+4,endDegrees+8,preservedC..6B. After the final-boundary latch, while D!=0 and t<=D set native battle camera field+EC to `(start+(t/D)*(end-start))*0.017453292`. D0 does not write it. Destroy50C00 restores binary32 bits3F32B8C2. Native50C5C uses LDP S for the two angle fields. Do not cast those float words to integer degrees or conflate this with primitive33's independent FOV schema.

### 11: camera setup plus optional unit restoration

Primary28: u32 mode+0,u8 restoreUnits+4,preserved5..27. At t0 perform setup kind1/option1 and35930 reset as above, then mode0→setup2C(0),1→2D(0),2→2E(selected target's unit id),3→6(target id),4→36(target id); other modes add no second setup. If restoreUnits!=0, restore eligible units' base pose/rest motion in the first two lists using their recorded rate/motion/option fields. Later updates return. Its original destroy50C90 is empty.

### 12: original empty subtype

Primary30 is copied/preserved, but the installed callbacks50EA0 and50EB0 are genuinely empty. This is a source-proven no-action subtype with SDK instance lifetime/clock, not permission to replace other unsupported subtypes by no-ops. Keep it distinct from an unknown callback or a failed constructor.

## Remaining integration/semantic items

The registered table, primary layouts, source envelopes, selected-unit filter and R_BLOCK helper/light algorithm are established. Remaining closure is narrower than 'all BattleOnly unknown': exact native action-eligibility/rest-motion facade equivalence for7/8/11; exact camera facade/coordinate helper semantics for9/11; and renderer binding of unit spatial-color versus packed tint and scene-light outputs. Their numeric callbacks/memory producers are identified above; no fake facade success or hidden no-op is authorized. Image/model/helper/motion decoding remains the existing real M8a/M8c contract. Runtime witnesses belong to implementation, not this static lane.
