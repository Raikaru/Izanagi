# M8f model, helper and drawable primitives

All offsets in this file are hexadecimal unless explicitly decimal. Wire scalars are little-endian; native pointers and structure sizes below are explanatory, not wire fields. These are registered primitive kinds26(Model),27(Helper),32(Drawable), distinct from ordinary particle behavior numbers. Reuse the M8c model contract pinned at48afcdf5204d388e021dd1f786ab7fef39afb126; do not invent a second GMO/AMD reader. [Dependency census](m8f-model-adjunct-census.md) enumerates2403 adjunct3 model spans and104 inline helper models.

## Common primary prefix

| Offset | Type/bytes | Meaning |
|---|---|---|
| 00..23 | 24 bytes | Color key descriptor consumed by353F40; same packed-color/key semantics as [common emitter](m8f-emitter-render-state.md) |
| 24 | u32 | Maximum-opacity argument to353F40 |
| 28 | u32, selector lowu16 | Renderer layer selector;384C90/384CF0 narrow to u16 and require0..6; retain upper16 bits |
| 2C,30 | f32 each | Fade fractions passed with the color key descriptor |
| 34..5F | 2C bytes | Scalar key descriptor consumed by354200; divide evaluated value by10 before multiplying parent scale |

These keys are deterministic evaluations at trunc(clock), not particle birth-randomization. Preserve unused bytes rather than applying emitter variation behavior to model keys. Parent packed color multiplies each evaluated channel by parentChannel/255 with truncation. Parent scale is independent of the curve value.

## Kind26: Model

Primary size68: common prefix, u32duration at60, u8blendRequest at64, padding65..67. Adjunct kind3 supplies the actual bounded model bytes; a valid M8c model is required. Native instance sizeA8: position00, quaternion10, scalar scale20, per-draw advance latch24, parent color28, copied primary2C, model handle98, own effect clockA0. None of these native offsets may replace explicit wire parsing.

Create361D70 loads an independent model through361BD0. Update362010 does nothing when duration!=0 and duration<clock; otherwise sets advanceLatch=1 then clock+=0.5*step. Draw362070 requires a nonnull model and duration==0 or clock<=duration. It evaluates color/scalar keys, applies quaternion orientation, scale and position, binds the shared effect render-work pointer through38AD40, and submits389660. Duration decimal1225(0x4C9) additionally sets model flag408 bit20; this is an exact constant, not a generic looping sentinel.

Before drawing, clear model408 mask18. If blendRequest!=0, maxOpacity==0 or maxOpacity<255 selects bit8, otherwise mask18. A nonzero model field1008 clears mask18 regardless. Set model playback rate to advanceLatch, submit, then clear advanceLatch. Thus repeated draw without update does not advance model motion. The model manager's rate consumer388DDC uses step*rate/60*2 before3904D0; do not confuse rate with a seek-frame field.

Reset362000 only seeks the underlying model to frame0 through38A4F0. It does **not** zero own effect clockA0. Copy361F50 recreates the model from its resource identity pair404/406 via387D50, initializes motion when present, clears408 bit1, and registers361BC0. Mutable model handles are never shared across effect instances. Destroy361E60 releases the owned model before freeing the instance.

## Kind27: Helper

The first68 bytes resemble Model, but60 is signed duration and64 is **f32 playbackRate**, not a blend flag. Bytes68..6F are preserved padding; inline model bytes are exactly primary[70:declaredPrimarySize]. All33 distinct observed helper primaries have eight zero padding bytes and a valid OMG header at70. There are24 distinct inline models, not33. Never locate them by an unconstrained magic scan.

Subtype0 create367FE0 loads this inline model. Native sizeB0: transforms00..23,parentColor24,anchorCount28,clock2C,copied68-byte prefix30,child pointer array98,allocationA0,modelA8. Anchor count is38ABB0(model), **not automatically bone count**. When motion exists,38A130 sets looping iff duration==0. An optional adjunct must be kind6(EPL). Instantiate the first child with363B80, then clone it with365980 for each remaining anchor. Allocate one independent mutable EPL per anchor; no single shared live child is transformed repeatedly.

Update368480 requires a model. While duration==0 or clock<duration:

1. Apply parent orientation/position/scale to the model; set model motion rate from primary64 and update388F70.
2. If children exist, evaluate color and scalar keys at trunc(clock), multiply parent color, and for each successful38AA40(model,index,matrix) query: set child position to matrix translation, orientation to the matrix-to-quaternion conversion below, scale to scalarCurve/10*parentScale, color to the evaluated result, then update364F80.
3. After this active block, advance own clock by0.5*step. A nonnull model continues advancing this clock even after duration expires.

Draw3687C0 draws all children only if duration==0 or trunc(clock)<duration. The source branch with anchors but no child EPL is explicit debug anchor visualization, not a license to substitute debug markers for missing production resources. Reset368420 resets each child through365BF0, seeks the model to0, and zeros clock2C. Destroy368200 destroys all children, frees their pointer array, releases the model, then frees work. Copy3682C0 recreates the model by resource identity and clones **source child0** into every destination anchor; do not silently replace this with per-index source cloning or mutable sharing.

### Helper orientation conversion361950

Use the source 4x4 storage convention shared with M8c, m[0],m[5],m[10] diagonal. Let t=m0+m5+m10+1. If t>=1, s=2*sqrt(t) and q=((m6-m9)/s,(m8-m2)/s,(m1-m4)/s,-s/4). Otherwise choose k=(m0<=m5?1:0), replace k by2 only if m10>m[4*k+k], i=(k+1)%3,j=(i+1)%3, and s=2*sqrt(m[4*k+k]-m[4*i+i]-m[4*j+j]+1). If s!=0: qk=s/4, qi=(m[4*k+i]+m[4*i+k])/s, qj=(m[4*k+j]+m[4*j+k])/s, qw=-(m[4*i+j]-m[4*j+i])/s. If s==0, qk=1 and all other components zero. Preserve the negative-W convention; do not insert an unproved scale-removal or normalization pass.

## Kind32: Drawable

Primary sizeA4; prefix through5F is common above.

| Offset | Type | Meaning |
|---|---|---|
| 60,64 | s32 each | Cycle interval start/end |
| 68..6F | 8 preserved bytes | No semantic reads proved here |
| 70 | u8 | Alternate cycle/skip enable |
| 71..73 | padding | Preserve |
| 74,78,7C | f32 each | Logical-surface X/Y and camera-depth offsets |
| 80,84,88 | f32 each | Local rotation X/Y/Z arguments |
| 8C | u32 | Effect duration, zero unbounded |
| 90 | u8 | Inverted model40C bit0 setting: zero sets, nonzero clears |
| 91 | u8 | Blend-enable selector |
| 92..93 | padding | Preserve |
| 94 | u32 | Blend-mode/threshold selector; nonzero also sets408 bit40000 at creation |
| 98..A3 | preserved bytes | No semantic reads proved here |

Native sizeC8: logical X/Y00/04, advance latch08,parentColor0C,copied primary10,modelB8,cycleModeC0. Create362440 uses model bridge flags4 and registers model callback362BF0 with this instance. CycleMode is true exactly when both signed interval values are nonnegative and end>=start.

Update362760 samples actual model animation frame f through38A7B0. Let next=trunc(f+0.5*step). If duration!=0 and duration<u32(trunc(f)+1), return. If cycleMode and next>cycleEnd-1-0.2, seek cycleStart-0.5*step+0.2. If !cycleMode and cycleEnabled!=0 and cycleStart<=next<cycleEnd, seek cycleEnd-0.5*step+0.2. Set advanceLatch=1 on every nonfinished path. Draw362880 accepts duration==0 or actual model frame<=duration, evaluates common keys using that frame, binds render-work, submits, then clears the latch. Reset362750 seeks model0; it has no separate effect clock. Copy362620 recreates the model, reinstalls362BF0 and the primary90/94 flag policy, and recomputes cycleMode.

Draw clears408 bit100. If primary91!=0 and(primary94==0 or primary94<=254), set408 mask18; otherwise clear18. Nonzero model1008 overrides and clears18. These are explicit source state transitions, not a backend-agnostic claim that every model material is translucent.

### Drawable logical projection callback362BF0

The reference logical surface is **960x540**, not the current framebuffer dimensions. Obtain the active camera's near/far values n/f through334850/3348A0, projection factors px/py at camera80/84, and camera matrix through3369D0. The source computes:

- z=(n*(-65535)*f)/(f*(-65535)+(f-n)*31)+100.
- hx=px*z; hy=py*z.
- Uniform local scale=(scalarCurve/10)*(2*hx/960).
- Camera-space position=(2*hx*(0.5-(instanceX+offsetX)/960),2*hy*(0.5-(instanceY+offsetY)/540),z+offsetZ).

The local matrix is initialized by339660 with the primary80 X rotation, then applies nonzero primary84/88 Y/Z rotations using the M8c SDK matrix-composition convention. Apply uniform scale, compose with the camera matrix via338AC0, rotate the camera-space position by its3x3 through33DCC0 and add camera translation. Set model matrix38AA10 and refresh388EB0/38AA30. Do not replace the source depth constants31/65535/+100 with a guessed orthographic projection. Exact engine-camera facade and composition adapter remains O03/O04 until tied to the renderer's established M8c convention; the wire interpretation, logical reference dimensions and source numeric formula are closed here.

## Render layers and integration boundaries

384C90(Model) and384CF0(Drawable) accept low-u16 selector0..6 and use byte lookup63C632: [0,1,4,6,2,3,5]. They read shared render-work slots at38+8*lookup for Model and70+8*lookup for Drawable.384C80 returns the current effect render-work pointer, **not itself a view matrix**;38AD40 stores it at modelFA8 and propagates to active related models.361BC0 forwards to the configured effect callback hook; do not install a no-op as an equivalent production renderer.

The registration table86FAD0 uses128-byte rows: create+00,combined update/draw+08,update+10,draw+18,destroy+20,clone+28,copy+30,reset+38,position+40,orientation+48,scalarScale+50,vectorScale+58,color+60. Combined+08 is not reset. Model phase roots361D70/362350/362010/362070/361E60/361E90/361F50/362000; Helper367FE0/3688D0/368480/3687C0/368200/368270/3682C0/368420; Drawable362440/362AE0/362760/362880/362570/3625A0/362620/362750.

Steam parser/dispatch crosswalk and hashes remain in [effect grammar](m8f-effect-grammar.md). This continuation was checked against ordinary Switch1.2 NSO code, never protector code, and against exact declared Steam descriptor spans. Runtime layer/backend/camera binding, material sub-pipelines, legacy model formats and model-attached AMD EFFECT_* interpretation remain separately explicit OPENs rather than invisible fallbacks.
