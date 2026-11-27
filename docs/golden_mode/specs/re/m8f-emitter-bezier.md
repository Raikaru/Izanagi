# M8f ordinary emitter behavior7: Bezier and axial rotation

Ordinary Switch1.2 NSO357D50 supplies the complete birth/preroll/active algorithm. Compare the pinned PS2 `MdlEffectBezierParameters` at `include/mdl_effect_emitter_internal.h:90..106`; Golden allocator/behavior-bank widths remain authoritative. Apply the common RNG, spawn/death, appearance and Golden fractional scheduler. All offsets/sizes hex.

## Parameter and state layout

Primary124; per-primary state4C. Shared prefix00..C7 has its existing meaning.

| Offset | Type | Meaning |
|---|---|---|
| C8 | s16 | firstBlendPercent |
| CA | 2 bytes | Preserved padding |
| CC,D0 | f32 | firstDisplacement,firstVariation |
| D4 | s16 | secondBlendPercent |
| D6 | 2 bytes | Preserved padding |
| D8,DC | f32 | secondDisplacement,secondVariation |
| E0 | s16 | rotationSpanDegrees |
| E2 | 2 bytes | Preserved padding |
| E4,E8,EC | f32 | rotationSpeed,rotationVariation,rotationAcceleration |
| F0,F4,F8 | f32 | travelSpeed,travelVariation,travelAcceleration |
| FC | f32[3] | start |
| 108 | f32[3] | end |
| 114 | 8 bytes | Preserved padding |
| 11C,120 | f32 | endpointOffsetRadius,endpointOffsetHeight |

State: firstControlXYZ0,secondControlXYZC,preserved/unused18..20,endpointOffsetXYZ24,rotationSpeed30,travelSpeed34,birthLineLength38,monotonicProgress3C,sizeScale40,angleOffset44,angleScale48. The state cursor advances13 floats, independently agreeing with the bank's4C bytes.

## Rotation helper and canonical up

384500 delegates to384320 and installs its matrix in the effect math scratch.384320 normalizes the supplied XYZ axis only when length>0, then builds the ordinary right-handed Rodrigues matrix. For a vector V, axis N and angle theta in radians:

`Rot(N,theta,V)=V*cos(theta)+(N cross V)*sin(theta)+N*dot(N,V)*(1-cos(theta))`.

Preserve its zero-length branch: zero N yields cos(theta)*V, **not an injected identity/unit-axis fallback**. Positive X rotation agrees with the other effect helpers. The Bezier basis uses up=(0,1,0); GOT881788's RELATIVE record5BDD68 points to immutable86FAA0, whose quad is(0,1,0,1). W is not another spatial component of the cross products.

## Birth and RNG draw order

Let S=primary.start, and make the following draws in order:

1. phi=S(U())*3.1415927. EndpointOffset=(cos(phi)*offsetRadius,-offsetHeight,sin(phi)*offsetRadius); save it. Let E=primary.end+EndpointOffset.
2. birthLineLength=lengthXYZ(S-E); N=normalizeXYZ(S-E), with the no-division zero branch. Form B=(N cross up) cross N. **Do not normalize B afterward**; its magnitude affects displacement.
3. theta=S(U())*float(signed rotationSpanDegrees)*0.5*0.017453292. Build Rot(N,theta) through the helper above.
4. b1=float(signed firstBlendPercent)/100. FirstControl=Rot(N,theta,B*V(firstDisplacement,firstVariation))+(1-b1)*S+b1*E.
5. b2=float(signed secondBlendPercent)/100. SecondControl=Rot(N,theta,B*V(secondDisplacement,secondVariation))+(1-b2)*S+b2*E. Reuse the same rotation matrix; do not draw a second rotation angle.
6. state.travelSpeed=V(F0,F4), then state.rotationSpeed=V(E4,E8), in that order. progress=0; positionXYZ=S.
7. Draw the common size/angle variations, set age0, then use the preroll branch below if requested. The start/end suffix, not the common position0 quad, directly supplies these curve endpoints.

Control points, endpointOffset and birthLineLength are birth state. The controller does not regenerate them each frame. Ordinary birth evaluates appearance at0 and keeps age0. Birth under preroll initializes trail state and advances the selected age by1 after appearance.

## Curve evaluation and axial transport

At integer age a, compute the two nonnegative analytic quantities separately:

```
travel=max(0,state.travelSpeed*a + 0.5*travelAcceleration*a*a)
rotation=max(0,state.rotationSpeed*a + 0.5*rotationAcceleration*a*a)
candidate=min(travel/1000,1)
state.progress=max(state.progress,candidate)
t=state.progress
u=1-t
E=primary.end+state.endpointOffset
P=primary.start*(u*u*u)
  +state.firstControl*(3*t*u*u)
  +state.secondControl*(3*t*t*u)
  +E*(t*t*t)
```

Keep scalar operation ordering consistent with the source. Progress cannot retreat after negative acceleration. It is travel/1000, not travel/lifetime or travel/birthLineLength, and no arclength reparameterization is performed.

If rotation is nonzero, active updates compute N=normalize(primary.start-primary.end), **without the saved endpointOffset**, then pivot=N*(t*birthLineLength). Replace P by `pivot+Rot(N,rotation,P-pivot)`. Do not add primary.start to this pivot or silently use an interpolated point along the actual offset curve. Active endpoints are the current primary suffix values, while the control points and line length remain birth state.

Preroll draws r=I()%lifetime and evaluates travel/rotation/progress/curve at a=r. Its rotation axis is the birth N=normalize(S-E), including the endpointOffset, rather than the active branch's unoffset current endpoint axis. It does not simulate r active steps. Set age=r, evaluate appearance, initialize the source trail state and advance age to r+1.

Active updates save the previous position, evaluate the equations at current integer age a, run common appearance, apply sizeScale and angleScale/offset, propagate trails35BD40, then set age=a+1. Birth and active branches are deliberately distinct. The controller itself does not replace these suffix endpoints with a generic emitter-position/velocity integrator.

## Scope

This closes behavior7's parameter/state/RNG/control-point/progress/preroll/active equations. Together with the other M8f behavior pages, ordinary behavior1..11 are specified; kind0 remains the bank's explicit null behavior. It does not close primitive-specific geometry, every trail curve, BattleOnly subtype rendering, or runtime visual qualification.
