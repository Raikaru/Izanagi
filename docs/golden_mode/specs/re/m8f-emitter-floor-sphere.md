# M8f ordinary emitter behaviors8 and9

Offsets/sizes hex; apply [common particle prefix/RNG/Golden sample scheduling/appearance](m8f-emitter-render-state.md). Ordinary NSO source:358DA0(8),3595D0(9), inspected through their complete birth and active branches. These are behavior selectors, not historical null primitive8 or environment primitive9.

## Allocation versus callback indexing

The independently qualified behavior bank reserves F8 state bytes perprimary for8 and E8 for9. **Keep those source allocation/copy sizes** when implementing its exact allocator contract. The callbacks themselves index compact live state at strides20(8) and24(9), respectively. Allocation capacity and callback stride are distinct facts:358DA0 ends with state+=8 floats;3595D0 ends with state+=9 floats. Do not use the bank allocation byte count as the callback indexing stride, and do not edit the qualified bank to make them equal. Golden sample stride remains58.

## Behavior8: downward cone with floor bounce

PrimaryF8. Suffix f32 fields: C8 spread,CC spreadVariation,D0 cone,D4 coneVariation,D8 floorHeight,DC speed,E0 speedVariation,E4 horizontalAcceleration,E8 gravity,EC bounce,F0 bounceVariation,F4 bounceDecay.

Compact state20: directionX0,verticalVelocity4,directionZ8,speedC,bounceScale10,sizeScale14,angleOffset18,angleScale1C. The initial directionY is immediately stored in the field later used as vertical velocity; **it is not multiplied by speed** in the active vertical recurrence.

Birth RNG order and state:

1. cone=V(D0,D4); draw two S(U), form direction=(S0*cone,cone-1,S1*cone), normalize when length>0. World-space rotates it by the current primary quaternion. Save directionX/Y/Z in state0/4/8.
2. speed=abs(V(DC,E0)); bounceScale=-V(EC,F0); spread=V(C8,CC). If configured speed DC<0, negate only the direction used for initial placement, not the saved direction/velocity.
3. Position=placementDirection*spread; add primary position only in world-space. Draw common size/angle variation. Set age0, evaluate appearance using that same initial position as the previous position, apply stored variation.
4. Unlike motion/helix/orbit, **there is no I()%lifetime analytic preroll branch** here. Preroll affects the initial birth budget only. The birth branch neither increments age nor initializes a trail sample through the ordinary active trail callback.

Active integer age a, after saving the previous position:

```
horizontalStep=max(0,state.speed + horizontalAcceleration*a)
state.verticalVelocity -= gravity
position.x += horizontalStep*state.directionX
position.y += state.verticalVelocity
position.z += horizontalStep*state.directionZ
if position.y < floorHeight:
    k=bounceDecay*state.bounceScale
    state.bounceScale=k
    state.verticalVelocity *= k
    position.y=floorHeight+k*(position.y-floorHeight)
```

The bounce test is strict<, the floor correction is signed, and X/Z are unaffected by bounce. Gravity is subtracted once per active tick; it is **not** gravity*a as in motion2. Apply appearance at a, size/angle variation, trail propagation35BD40, then age=a+1. Preserve the no-respawn/lifetime rules separately from collision.

## Behavior9: variable-radius spherical boundary and noisy reflection

PrimaryE8. Suffix C8 initialRadius,CC initialRadiusVariation,D0 finalRadius,D4 finalRadiusVariation,DC speed,E0 speedVariation,E4 acceleration are f32. D8 is signed32 radiusTransitionTicks, not an f32 divisor despite decompiler aliases.

Compact state24: directionXYZ0,radiusC,radiusStep10,speed14,sizeScale18,angleOffset1C,angleScale20. World-space center is the current primary position quad. Local-space center is the engine's canonical zero-origin quad referenced through GOT881790; its RELATIVE relocation at5BDD80 points to zero-initialized BSS51C8B90. No emitter quaternion is applied to the two random direction vectors in this callback; local-space renderer transformation remains separate.

Birth RNG order:

1. Draw three S(U) components, normalizeXYZ when length>0, store direction.
2. speed=abs(V(DC,E0)); radius=V(C8,CC); final=V(D0,D4); radiusStep=(final-radius)/float(signed radiusTransitionTicks). The original performs this floating division without a zero-divisor replacement; do not invent an epsilon or a different transition length.
3. Ordinary birth age0. Preroll instead draws r=I()%lifetime, sets age=r and radius+=radiusStep*r; if step>0 and radius>final, or step<0 and radius<final, clamp radius to final.
4. Draw a **second independent** three-component S(U) vector and normalizeXYZ. Position=this second vector*radius, plus center only in world-space. This is not the saved travel direction and does not consume a uniform-sphere sampler's draws.
5. Draw size/angle variation, evaluate appearance at the selected age. Preroll initializes the source trail state and increases that age by1; ordinary birth keeps age0.

Active tick, with old position P, stored direction D and integer age a:

1. Save `R=state.radius`. If a<signed radiusTransitionTicks, increase state.radius by radiusStep. **The collision below still uses old R**, not the newly advanced radius.
2. v=max(0,state.speed+acceleration*a); candidate=P+D*v. Let delta=center-candidate and distance=lengthXYZ(delta).
3. If distance<=R, retain candidate. Otherwise normalize delta when its XYZ length>0, obtaining inward normal N. Draw one U and set `k=2*dotXYZ(N,D)*(0.075*U()+0.925)`. New direction Dnew=D-k*N. Store Dnew without an extra normalization.
4. On collision let excess=distance-R. New position is `P + D*(v-excess) + Dnew*excess`, in that operation order. Excess is a distance, not a normalized time fraction; do not substitute a ray/sphere intersection, clamp it to[0,v], or project candidate directly onto the sphere.
5. Evaluate appearance using previous P, apply size/angle variation, propagate trails and set age=a+1.

Only the actual outside-boundary branch consumes the reflection RNG draw. A visually similar ideal elastic reflection with deterministic restitution is not equivalent. The source's quad operations carry the corresponding W values; the XYZ equations above fix rendered motion without reinterpreting W as another spatial dimension.

## Boundary

These equations close ordinary behavior8/9; Bezier7 and the general trail-curve/primitive-render schemas remain explicitly separate source work. Allocation sizes, local-origin ownership, constructor scale callbacks and the full emitter scheduler remain their existing contracts; this page does not authorize replacing them with PS2 native layouts or shrinking the qualified bank.
