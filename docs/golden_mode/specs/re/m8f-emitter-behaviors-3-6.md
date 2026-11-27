# M8f ordinary emitter behaviors3 through6

Companion to [the common emitter contract](m8f-emitter-render-state.md). All offsets and sizes below are hex; scalar suffix fields are binary32 except the explicitly signed integer angle span. These are ordinary primitive1 behavior selectors, not EP primitive kinds. Apply the common prefix, RNG, spawn/death, fractional-time sampling, appearance and trail ordering. `V(v,k)`, `U()`, `I()` and `S(U)` have exactly the definitions on that page; each V/U occurrence consumes its own draw. `L` is the signed primary lifetime atB8. Preserve the common zero-lifetime return.

## Source receipts

Switch1.2 ordinary NSO functions:355600(helix),3560C0(orbit),356A80(cone),3573D0(angular cone). PS2 comparison functions in `Graphics/Model` are `mdlEffectUpdateHelixEmitter` in `mdlEffect_inferred_0048cdf0.c`, `mdlEffectUpdateOrbitEmitter` in `mdlEffect_inferred_0048d8c0.c`, `EffConeEmitterUpdate` in `mdlEffect_inferred_0048e2f0.c`, and `EffConeAngleEmitterUpdate` in `mdlEffect_inferred_0048ec50.c`. Golden behavior-bank copy/state widths, rather than inferred PS2 native object layouts, determine the sizes below. These NSO paths are ordinary code; no Steam `.arch` code was decoded.

## Behavior3: helix

PrimaryF4; per-primary native state40.

| Primary offset | Meaning |
|---|---|
| C8 | heightRange |
| CC,D0 | initialRadius,radiusVariation |
| D4,D8 | finalRadius,finalRadiusVariation |
| DC,E0 | angularSpeed,angularSpeedVariation |
| E4,E8 | verticalSpeed,verticalSpeedVariation |
| EC | angularAcceleration |
| F0 | gravity |

State: originXYZ+0, orientation quaternion+C, height+1C, radius+20, radiusStep+24, initialPhase+28, angularSpeed+2C, verticalSpeed+30, sizeScale+34, angleOffset+38, angleScale+3C.

Birth, in RNG order:

1. radius=V(CC,D0); radiusStep=(V(D4,D8)-radius)/L.
2. initialPhase=U()*6.2831855; angularSpeed=V(DC,E0); verticalSpeed=V(E4,E8); height=heightRange*U().
3. Local cylindrical position=(radius*cos(initialPhase),height,radius*sin(initialPhase)). In world-space rotate by the primary quaternion and add the primary position; save this birth quaternion/origin in the state. The moving world-space particle thereafter uses this saved basis, not the current moving emitter basis.
4. Draw size/angle variation as in motion behavior2. Set age0.
5. Preroll draws r=I()%L, adds verticalSpeed*r-0.5*gravity*r*r to height and radiusStep*r to radius. Reconstruct the cylindrical position using the **unchanged initialPhase**, then the saved world basis if applicable. This branch does not advance the phase by r*angularSpeed. Evaluate appearance at age r, initialize trail state and advance age to r+1 under the common preroll rule.

At active integer age a:

- phase=initialPhase+(0.5*angularAcceleration*a+angularSpeed)*a;
- height=(height+verticalSpeed)-gravity*a;
- radius+=radiusStep;
- position=(radius*cos(phase),height,radius*sin(phase)), transformed by the saved basis in world-space;
- evaluate common appearance using the previous position, apply stored size/angle variation, propagate trails and increase age by1.

`initialPhase` remains unchanged. Do not replace these operations with an accumulated angular velocity or continuously integrate the preroll branch.

## Behavior4: randomly tilted orbit

PrimaryE8; per-primary native state34.

| Primary offset | Meaning |
|---|---|
| C8,CC | initialRadius,radiusVariation |
| D0,D4 | finalRadius,finalRadiusVariation |
| D8,DC | angularSpeed,angularSpeedVariation |
| E0 | angularAcceleration |
| E4 | gravity |

State: originXYZ+0, tiltX+C, tiltY+10, radius+14, radiusStep+18, angularSpeed+1C, initialPhase+20, height+24, sizeScale+28, angleOffset+2C, angleScale+30.

Birth draw order: radius=V(C8,CC); radiusStep=(V(D0,D4)-radius)/L; angularSpeed=V(D8,DC); initialPhase=U()*1.5707964; tiltX=S(U)*3.1415927; tiltY=S(U)*3.1415927. The initial phase range is one quadrant, **not**2pi. Height starts0. Size/angle draws then follow the common rule.

Form the orbit basis with NSO384260(tiltX),384580(tiltY),384640(). The first helper builds an X rotation; the second builds a Y rotation; the third multiplies X*Y in column-vector notation. Equivalently apply the Y rotation first and then X:

```
q = (radius*cos(phase), 0, radius*sin(phase))
x = cos(tiltY)*q.x + sin(tiltY)*q.z
z = -sin(tiltY)*q.x + cos(tiltY)*q.z
rotated = (x, -sin(tiltX)*z, cos(tiltX)*z)
```

Add saved birth origin in world-space, then add state.height to **world/local Y after the tilt**, not to the circle's Y before rotation. This behavior does not apply the emitter quaternion instead of the two random tilts.

Preroll r=I()%L sets height-=0.5*gravity*r*r and radius+=radiusStep*r, reconstructs the position with unchanged initialPhase and the same tilt basis, adds the origin if world-space, then adds height to Y. Appearance/trail initialization and age advancement follow the common preroll rule.

Active age a: phase=initialPhase+(0.5*angularAcceleration*a+angularSpeed)*a; height-=gravity*a; radius+=radiusStep; reconstruct position in the specified order. Apply appearance/trails and increase age. As in helix, the initial phase is never overwritten.

## Behavior5: cone

PrimaryE8; state1C, identical in layout to motion behavior2. SuffixC8 spread,CC spreadVariation,D0 cone,D4 coneVariation,D8 speed,DC speedVariation,E0 acceleration,E4 gravity.

Birth draws cone=V(D0,D4), then two independent S(U) values. Direction=normalize((S0*cone,1-cone,S1*cone)), with the common zero-length no-division branch. If world-space, rotate this direction by the serialized primary quaternion. Save the direction before the negative-speed placement operation. Then speed=abs(V(D8,DC)), spread=V(C8,CC); configured speed<0 negates only the vector used for initial placement. Initial position=placementDirection*spread, plus primary position if world-space. Common size/angle variation follows.

Active and preroll motion are behavior2's equations, using this cone direction and acceleration/gravity atE0/E4. In particular active speed is max(0,speed+acceleration*age), and preroll is the separate analytic displacement, not a simulation loop. Do not use coneVariation as a symmetric angular deviation or use speed's sign to reverse every later step.

## Behavior6: angular cone

PrimaryEC; state1C as behavior2. The firstE8 bytes match behavior5; E8 is **s32 angleSpanDegrees**.

Birth draws phase=S(U())*float(angleSpanDegrees)*0.5*0.017453292-1.5707964, then cone=V(D0,D4). Direction=normalize((cone*cos(phase),cone-1,cone*sin(phase))). Its Y sign is the opposite of behavior5's `1-cone`. Rotate by primary quaternion in world-space. Continue with behavior5's speed/spread, signed initial placement, common appearance and preroll/active rules. RNG order and draw count differ from behavior5; neither subtype can be implemented by changing only the angular spread of an isotropic motion emitter.

## Boundary

This continuation closes behavior3..6's suffix/state/birth/active/preroll contract. It does not claim that the remaining behavior7..11 callbacks, every trail-curve branch or every primitive geometry/filter subtype are closed. Those remain explicit on the master OPEN list.
