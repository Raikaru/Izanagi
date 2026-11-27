# M8f ordinary emitter behavior11: sway with floor bounce

Offsets/sizes hex. Apply [the common prefix, RNG, Golden fractional sampling and appearance contract](m8f-emitter-render-state.md). This is primitive1 **behavior11**, not historical primitive11's null tool callback.

Source: ordinary Switch1.2 NSO35ABD0, including the active recurrence at its final branch; immutable vector86F940 is independently decoded as binary32(0,1,0,0). Birth-axis instructions35AED0..35AF80 distinguish the local/world branch. PS2 comparison: `include/sway_particle_internal.h` and `src/Graphics/Model/mdlEffect_inferred_00492100.c` in the pinned allowed decomp. PS2's integer sample/native layout does not replace Golden's58-byte sample or fractional scheduler.

## Primary and state

Primary110; per-primary state40. Shared prefix00..C7 has the established meaning.

| Offset | Type | Suffix meaning |
|---|---|---|
| C8 | f32 | spread |
| CC,D0 | f32 | speed,speedVariation |
| D4 | f32 | deceleration |
| D8,DC | f32 | initialAmplitude,initialAmplitudeVariation |
| E0,E4 | f32 | finalAmplitude,finalAmplitudeVariation |
| E8,EC | f32 | phaseStep,phaseVariation |
| F0 | f32 | floorHeight |
| F4,F8 | f32 | bounce,bounceVariation |
| FC | f32 | bounceDecay |
| 100..10F | bytes | Preserved copied tail; no semantic read established in this controller |

State: riseDirectionXYZ0,swayDirectionXYZC,phase18,previousSine1C,amplitude20,amplitudeStep24,phaseStep28,speed2C,bounceScale30,sizeScale34,angleOffset38,angleScale3C.

## Birth and RNG order

1. Rise direction=(0,1,0); rotate it by the serialized primary quaternion when world-space. Draw two independent S(U) components for an XZ sway vector, normalize it when length>0, and rotate it by that quaternion when world-space. Zero-length takes the no-division branch, not an injected unit direction.
2. speed=V(CC,D0); bounceScale=-V(F4,F8); amplitude=V(D8,DC); amplitudeStep=(V(E0,E4)-amplitude)/L; phaseStep=V(E8,EC); phase=U()*6.2831855; previousSine=sin(phase). Each V/U consumes its own draw in this order.
3. Initial position=swayDirection*(C8*S(U)), using one new U for that centered spread. Add serialized position only in world-space.
4. Draw size/angle variation in the common order and set age0. Evaluate common appearance at0, then apply stored variation.
5. This controller's preroll branch **does not draw I()%L or perform the motion/wave analytic lifetime displacement**. It initializes that primary's trail state and increases age0→1 after the age0 appearance. The preroll birth budget still follows the common capacity/variation rule. Copying another behavior's preroll is incorrect.

## Active tick at integer age a

Store the previous position for projected-angle evaluation, then perform, in order:

```
s = sin(state.phase)
delta = state.amplitude * (s - state.previousSine)
position += state.swayDirection * delta
state.speed -= primary.deceleration
position += state.riseDirection * state.speed
state.phase += state.phaseStep
state.previousSine = s
state.amplitude += state.amplitudeStep
if position.y < primary.floorHeight:
    k = primary.bounceDecay * state.bounceScale
    state.speed = k * state.speed
    state.bounceScale = k
    position.y = primary.floorHeight + k * (position.y - primary.floorHeight)
```

The floor comparison is strict<. Do not clamp the particle onto the floor, replace the signed bounce coefficient with an absolute restitution, or use `speed-deceleration*age` instead of this stateful speed update. FloorY is applied in the sample's current coordinate space; local-space rendering remains a separate transform.

Evaluate appearance at a, multiply size by sizeScale, set angle=keyAngle*angleScale+angleOffset, propagate trails with35BD40, and set age=a+1. Lifetime/death and disabled terminal sentinel follow the common Golden rules.

## Closure boundary

This specifies behavior11's suffix/state/RNG/birth/active/preroll equations. It does not close the remaining behavior7..10 source algorithms, trail-curve edge cases or every geometry/backend consumer. No runtime/render qualification was performed.
