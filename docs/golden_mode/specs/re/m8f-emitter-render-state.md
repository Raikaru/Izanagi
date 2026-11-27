# M8f particle parameters, keyframes, clocks and render state

This continuation resolves the common emitter fields, effect RNG, indexed key evaluation, Golden interpolation samples and common EPL transform/render state behind O02/O03. It does not assert that the remaining primitive-specific distributions and postprocess subtypes are complete. All offsets/extents are hexadecimal; counts, ids and numeric constants are decimal. Source addresses omit Switch base7100000000.

## Evidence and type discipline

Ordinary NSO readers:353F40 color/opacity,354200 scalar curve,35BBF0 sample appearance,35B590 allocation,35B720 substep/interpolation,35BD40 trails,384190/3841C0/384210 RNG,354330 wave,354D90 motion,364540/364910/364B20 transforms,365270/3653A0 camera-relative state,365590 draw. The existing twelve-kind behavior proof independently pairs the allocation/update/scale bank with Steam. Steam wire construction and ownership remain paired as specified in the main BED/EPL pages.

**Decompiler float-pointer aliases are not wire types.** In354D90, emissionDuration+20 and lifetime+B8 are loaded as integer words and converted withSCVTF where required. A typed f32-pointer decompilation misleadingly prints expressions such as `(int)parameter[0x2E]`; that is not permission to reinterpret lifetime's integer bits as a float. A hash-checked R_BLOCK motion parameter has duration10,rate2,lifetime6 at20/24/B8 with raw words0A/02/06, respectively. This was independently checked with native Python312 and the actual LDR/SCVTF instructions. Scalar/data reinterpretations in this page follow the instructions and file bytes, not inferred decompiler C types.

The scoped semantic census found version200 primitives in real battle BEDs (including R_BLOCK, ANALYZE and assist/bike assets). **Do not globally discard version200 as editor-only.** Only the specific null-callback kinds in the main table are restricted to the observed old tool frontier. Every observed battle-root ordinary particle descriptor's declared parameter size equals its proven behavior-bank size, including version200 occurrences; the blanket 'historical particle blob is too short' hypothesis is false for that battle subset.

## Ordinary particle/behavior primary parameters

This prefix is used by primitive kind1 and by the behavior sub-blocks of related primitive kinds, not at the start of every EP primary payload. For example PolygonTrack prepends its own1C-byte record; ObjectParticle prepends44 bytes and DistortParticle48 bytes. Apply each primitive's own primary base before interpreting the shared behavior fields.

| Offset | Wire type | Meaning |
|---|---|---|
| 00 | f32[4] | Position/translation quad |
| 10 | f32[4] | Orientation quaternion quad |
| 20 | s32 | Emission duration;0 permits respawn after death |
| 24 | u32 | Emission rate; converted unsigned tofloat for the accumulator |
| 28 | f32 | Emission variation |
| 2C | u8 +3 preserved bytes | Color curve mode |
| 30 | u32 | Color start |
| 34 | u32 | Color end |
| 38 | u32 | First intermediate color |
| 3C | f32 | First intermediate fractional time |
| 40 | u32 | Second intermediate color |
| 44 | f32 | Second intermediate fractional time |
| 48..4F | bytes | Preserved color-block state, not consumed by353F40 |
| 50 | u32 | Maximum opacity; becomes high color byte |
| 54 | u16 +2 bytes | Geometry/render resource selector (not an opacity float) |
| 58 | f32 | Fade-in fractional time |
| 5C | f32 | Fade-out fractional time |
| 60 | u8 +3 bytes | Size curve mode |
| 64,68 | f32 | Size start/end |
| 6C | f32 | Birth size variation |
| 70 | u32 | Preserved size-block word |
| 74,78 | f32 | Size first intermediate value/fraction |
| 7C,80 | f32 | Size second intermediate value/fraction |
| 84..8B | bytes | Preserved state |
| 8C | u8 +3 bytes | Angle curve mode |
| 90,94 | f32 | Angle start/end, radians |
| 98 | f32 | Birth angle variation |
| 9C | u8 | Angle mode:2 projected-motion direction,1 random birth offset/sign, otherwise ordinary variation |
| 9D..9F | bytes | Preserved |
| A0,A4 | f32 | Angle first intermediate value/fraction |
| A8,AC | f32 | Angle second intermediate value/fraction |
| B0..B7 | bytes | Preserved state |
| B8 | s32 | Lifetime in discrete behavior ticks |
| BC | u8 | If nonzero, emitter uses local-space parameters (native emitter flagbit0) |
| BD | u8 | Preroll/prepopulate selector |
| BE..BF | bytes | Preserved |
| C0 | u32 | Trail count |
| C4 | s32 | Trail width |
| C8 onward | kind-specific | Behavior suffix; not another EP descriptor |

### Appearance evaluation

Let age be a signed behavior tick and L the signed lifetime.

Scalar curve354200 has mode byte+0,start+4,end+8,firstMiddle+14,firstFraction+18,secondMiddle+1C,secondFraction+20 **relative to the curve block**. Fractional breakpoint ticks are truncation towardzero of `fraction*float(L)`.

- L==0 returns start directly.
- Mode0: `start+(end-start)*(age/L)`. This scalar path does **not** globally clamp age/L to1.
- Mode1: interpolate start→middle1 before tick1; middle1→end thereafter.
- Mode2: interpolate start→middle1 before tick1; middle1→middle2 before tick2; middle2→end thereafter.
- Other modes use interpolation factor0, preserving the source expression rather than assigning a guessed spline.
- For the later segments, a zero denominator selects factor1. The first segment only executes for age<tick1; retain the actual source branch boundaries rather than introducing a generic clamp that changes valid edge behavior.

Color353F40 uses the analogous mode0/1/2 segment topology, but color middle/fraction offsets areC/10 and14/18, not scalar14/18 and1C/20. Its mode0 fraction is upper-clamped to1. If L==1 the color interpolation factor is0. Each of four channel bytes is converted tofloat/255, linearly combined and converted back by truncation, retaining packed byte order. **Opacity then replaces the high byte**:

- L==0 returns start's low24 bits and maximumOpacity in the high byte.
- `fadeInTick=trunc(fadeInFraction*L)`; before it, opacity factor=`age/fadeInTick`.
- From fadeInTick through `fadeOutTick=trunc(fadeOutFraction*L)` inclusive, factor1; L==1 also takes this full-opacity branch.
- After fadeOutTick, factor=`max(L-age,0)/(L-fadeOutTick)`.
- The source uses the selected branch's integer/float conversions, not a replacement smoothstep. Maximum opacity is an integer, not normalizedfloat.

Sample appearance35BBF0 writes color+34,size+40 andangle+4C. Angle-mode2 projects previous and current positions to view space, computes previous-minus-current XY and usesatan2; both deltas zero produceangle0. Otherwise use the angle scalar curve. Birth size/angle variation is applied **after** the common key evaluation by the specific behavior state.

## Effect RNG: separate from battle gameplay SFMT

384190/3841C0 select the supplied four-u32 state, or the effect-global state when the state pointer isnull. With old words(a,b,c,d), wrapping32-bit arithmetic:

`n = ((a >> 30) | (b << 2)) XOR ((c >> 31) | (d << 1))`

New state is(n,a,b,c). Integer draw returnsn. Float draw uses the same state transition and returns `(n & 0x00FFFFFF) * 2^-24`, in[0,1). No rejection sampling, SFMT substitution, host random library or floating hash noise is equivalent.

Seed384210: `a=seed XOR 0x0AED1A0C`; `b=rotr32(a,8)`; `c=rotr32(b XOR 0xAA5A02FE,8)`; `d=rotr32(c XOR 0x11BE81C7,8)`. Store(a,b,c,d). The caller owns when to seed the effect-global versus explicit state; constructing a viewer frame must not reseed perparticle.

Define `U()` as one effectfloat draw, `I()` as one effectinteger draw, `V(v,k)=v*(U()*k+(1-k))`, and `S(U)=U-0.5+U-0.5` with the **same** draw used twice. These are separate operations:V is not centered±variation, whileS is centered[-1,1). Preserve draw order even when a variation value iszero, wherever the source still calls the RNG.

## Golden emitter state and fractional-time behavior

Native emitter header50 (not wire data):u16 behavior id+0,primaryCount+4,totalCount+8,flags+C,logical time f32+10,previousStep s32+14,scheduledStep s32+18,emissionAccumulator f32+1C,sample pointer+20,state pointer+28,currentParameters pointer+30,originalParameters pointer+38,allocation pointer+40. The named pointer fields are full-width. The allocation includes a50-byte header,totalCount samples,one state record perprimary and two parameter copies. `totalCount=primaryCount*(trailCount*trailWidth+1)`.

Sample58: current position f32[4]+00,previous discrete endpoint+10,next discrete endpoint+20,age f32+30,current packed color+34,previous/next packed colors+38/+3C,current size f32+40,previous/next sizes+44/+48,current angle f32+4C,previous/next angles+50/+54. The age sentinel isfloat-1; terminal no-respawn isfloat-2. Do not overlay the old PS2 sample20 or its integer mode word.

35B720 is not 'update once then ++ticks':

1. First logical time0: mark every total sample age-1, dispatch one behavior update, snapshot current position/color/size/angle as the next discrete endpoints for each primary.
2. At later time, run the behavior callback once per scheduled discrete step minus previousStep. Before each call, shift next endpoints to previous and current; after it, snapshot newly computed current as next.
3. Compute the fractional part of logical time. For each primary with age>0, interpolate previous→next positions,size,angle; interpolate all four packed colorbytes separately with truncation and channel clamp0..255.
4. Store previousStep=scheduledStep. Advance logical time by0.5*liveFrameStep. Determine scheduledStep from time, with the source's0.1 epsilon only when the double-precision fractional part is exactly0. Do not round every fractional time with an epsilon.

These three time domains remain separate: packet elapsed, EPL/formation half-step clock, emitter's discrete simulation steps with interpolated samples. Redrawing a frozen frame must not consume behavior RNG or run another emitter step.

### Shared spawning/death rules (ordinary wave/motion controllers)

A zero lifetime returns without advancing that behavior's state. If duration!=0 and emitterTime>=duration, birth budget0. Otherwise, at emitterTime0 with preroll, initial birth budget=capacity, optionally truncated after multiplying by `U()*(1.1-emissionVariation)` when variation>0. This is notV(rate,variation). Non-preroll budget uses accumulator plus unsigned rate, with the same optional `U()*(1.1-variation)` multiplier; extract integer budget and keep the fractional remainder.

Scan primary samples in index order. Age>L dies (strict>, not>=), clearing that primary's trail ages to-1; new age becomes-1 when duration==0, otherwise-2. Age-2 never respawns. Age-1 consumes one available birth budget; active ages run the subtype update, appearance/trail propagation and then increase integer age by1. Exhausted budget does not skip updates of other active samples. The source's expiry/reset state is independent of EPL's part-disabled flag.

## Fully specified ordinary motion behavior2

Primary lengthE0; state length1C perprimary. SuffixC8 spread,CC spreadVariation,D0 speed,D4 speedVariation,D8 acceleration,DC gravity, allf32. State:directionXYZ+0,speed+C,sizeScale+10,angleOffset+14,angleScale+18.

Birth:

1. Draw threeS(U) components, normalize XYZ if length>0, leave the all-zero vector unchanged.
2. Store direction. Speed=`abs(V(speed,speedVariation))` (one freshdraw). Spread=`V(spread,spreadVariation)` (one freshdraw). If serialized speed<0 negate direction before placing the initial sample along it.
3. Initial position=direction*spread; when emitter flagbit0 isclear add the current parameter position quad. In the local-space path keep position local for render-time transformation.
4. sizeScale=`U()*sizeVariation+(1-sizeVariation)` (freshdraw).
5. Angle-mode2 gives offset0/scale1 without those angle variation draws. Otherwise angleScale=`U()*angleVariation+(1-angleVariation)`; mode1 additionally draws offset=`U()*2pi` and flips angleScale if the nextI() lowbit is1. Other modes useoffset0.
6. Set age0. For preroll, draw `r=I()%lifetime` using the source unsigned remainder, advance initial position by direction*max(0,speed*r+0.5*acceleration*r*r), subtract0.5*gravity*r*r fromY, set age=r, evaluate appearance, and advance age to r+1 after initializing trail state. This is distinct from iterating the ordinary active-step recurrence rtimes.

Active tick at integer agea: stepSpeed=`max(0,speed+acceleration*a)`; `position += direction*stepSpeed`, with `Y -= gravity*a`. Evaluate common appearance at a using the old position for projected angle. Multiply size by storedsizeScale and set angle=`keyAngle*angleScale+angleOffset`. Propagate trails, then age=a+1. The source does not replace gravity*a with a separately accumulated velocity.

R_BLOCK's measured behavior2 parameter length224 decimal and exact suffix fields were checked directly. These equations specify its ordinary particle motion without needing either binary. Other behavior ids must not be approximated by this motion controller.

## Wave behavior1 suffix and state

Primary lengthF0,state3C. SuffixC8 spread,CC forwardSpeed,D0 forwardSpeedVariation,D4 forward deceleration/gravity,D8 initialAmplitude,DC initialAmplitudeVariation,E0 finalAmplitude,E4 finalAmplitudeVariation,E8 phaseStep,EC phaseStepVariation. State directionXYZ0,waveDirectionXYZC,phase18,previousSine1C,amplitude20,amplitudeStep24,phaseStep28,forwardSpeed2C,sizeScale30,angleOffset34,angleScale38.

Birth forward direction is(0,1,0,0), independently confirmed by the binary32 constant atNSO86F930. Draw twoS(U) components to form the randomXZ wave direction and normalize when its length>0. Apply the serialized orientation to both directions when notlocal. Then store forward speedV(CC,D0),initial amplitudeV(D8,DC),amplitudeStep=`(V(E0,E4)-initialAmplitude)/lifetime`,phaseStepV(E8,EC),phase=`U()*2pi`,previousSine=sin(phase),and initial spread=`C8*S(U)` along wave direction. EveryV/U above consumes its own draw in that order. Size/angle birth variation follows motion behavior. Initial spread is centered; amplitude/speed variations are not.

Active step: compute `s=sin(phase)`; add waveDirection*amplitude*(s-previousSine), then forwardDirection*(forwardSpeed-D4*age). Updatephase+=phaseStep,previousSine=s,amplitude+=amplitudeStep. Appearance and trails follow the common order. Preroll's forward displacement uses `speed*r -0.5*D4*r*r`, advances phase/amplitude by r, adds the resulting sine displacement, then initializes the next phase/amplitude state. Preserve the ordinary floating-point operations and their order; this is not a closed-form replacement for the active recurrence.

## Common EPL transforms, inheritance and draw ordering

Part/root state flags at68 are **u32 bits**, notfloat values despite decompiler aliases:

| Mask | Operation established |
|---|---|
| 0x04 | Child anchor baseY forced5 |
| 0x08 | Offset magnitude placed on camera-facing normalized XYZ direction |
| 0x10 | Same radial-camera operation in horizontal plane; preserve serialized child offsetY |
| 0x20 | Face camera with pitch/yaw from camera-to-anchor vector |
| 0x40 | Yaw-only camera facing, composed with the child's original orientation |
| 0x80 | Child offset follows parent effective scale |
| 0x100 | Select vector-scale callback rather than scalar-scale callback |
| 0x80000000 | Root transform propagation already performed by update; draw avoids duplicating it and clears this bit afterward |

Part runtime state88 bit0 controls inheritance of parent scale; bit1 disables update/draw. These are not the independent source-sharing flags98. Store raw bits and preserve other flags without inventing a universal meaning for an unconsumed bit.

Root effective scale=externalScale*rootMultiplier. Child scalar scale=serializedChildScale multiplied by effective parent scale only when its runtime state bit0 ison. Vector scale multiplies the serialized scale-vector quad by that same selected child scalar. Position starts at rotated local root position plus external root offset; child local offset is optionally scaled, rotated by root quaternion, then translated. The forcedY5 applies to the anchor base before adding child offsetY. Root orientation updates recompute the root's rotated original position and compose child orientation with the supplied quaternion.

3653A0's0x08/0x10 radial-offset modes preserve the length of the serialized child offset, orient it opposite camera-to-anchor direction, and only zeroY during normalization for0x10 before restoring original offsetY.365270's0x20/0x40 modes compute camera-facing orientation from that live vector, not from actor rotation. Zero-vector normalization follows the no-division branch; do not inject a random or unit fallback.

The primitive callback exclusions are encoded as masks over **primitive kind**. Preserve them as exact sets/masks, not missing-callback guesses: position excludes mask0x200A00000;orientation excludes0x340A00230;scalar scale excludes0x340A00200;vector scale excludes0x3FCA0A2B6. Packed-color propagation excludes kinds21 and33. Both update and draw reuse these rules.

Update364F80 dispatches eligible parts at `part.start<=rootClock`, sets rootbit31 and advances rootClock by0.5*frameStep. Draw365590 first requires rootClock>0 and dispatches parts with `part.start<=trunc(rootClock-0.5*frameStep)`, excluding disabled parts. It calls the distinct draw slot at primitiveTable+18 (update is+10), then clears rootbit31. Thus draw eligibility refers to the just-simulated previous clock and **does not advance** the root time.

## Ordinary particle render contract

Primitive1 uses active samples(age>=0), not allcapacity. No draw when capacity0 or emitter time0. Ordinary geometry subtypes compact active positions,color,sizeXY and normalized angle into real vertex/instance streams; angle is wrapped into[-pi,pi] by subtracting/adding2pi. The primary selector at54 routes render/geometry setup. A local-space emitter transforms sample positions through its current orientation/translation at render time; a world-space emitter emits already-world positions.

For nested EP adjunct5, age0 resets the child primitive, then active samples set childscale from samplesize, childposition fromsampleposition and run the real child update/draw phases. For nested EPL adjunct6, age0 resets the child's EPL tree, then active samples setscale/position and run the real EPL update/draw. This is not equivalent to rendering one shared static child mesh at every particle. Each live child needs an independent clock/reset state although immutable resource backing may be shared.

Adjunct2 adds the texture-track frame's UV/origin/size/angle/color to sample attributes; adjunct7 uses TLP selection. Exact adjunct grammars, ordinary playback and the real HFLC/MD5/tex2 binding are in [the texture-binding continuation](m8f-texture-bindings.md). Do not use a white sprite when a binding is unavailable or treat external lookup completion as a successful null texture.

## Remaining source closure

Ordinary behaviors are specified: wave1/motion2 here, [helix/orbit/cone/angular-cone3..6](m8f-emitter-behaviors-3-6.md), [Bezier7](m8f-emitter-bezier.md), [floor8/sphere9](m8f-emitter-floor-sphere.md), [swirl10](m8f-emitter-swirl.md) and [sway11](m8f-emitter-sway.md).
Preserve each controller's different birth/preroll/active equations and the8/9 distinction between bank allocation capacity and compact callback state stride. Kind0 remains the explicit null bank entry.
Ordinary trail history/fade/interpolation and low-count edges are specified in [the trail continuation](m8f-particle-trails.md). O03 now concerns primitive-specific geometry/render schemas and exact BattleOnly facade binding. Registration/table/primary schemas and R_BLOCK animated-light semantics are specified separately; runtime visual qualification belongs to implementation.
