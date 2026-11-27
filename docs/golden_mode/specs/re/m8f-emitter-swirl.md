# M8f ordinary emitter behavior10: swirl

Source: ordinary Switch1.2 NSO35A180, with the complete birth/preroll and active branches inspected. Apply [common prefix/RNG/appearance/Golden fractional sampling](m8f-emitter-render-state.md). Offsets/sizes hex. PrimaryF8, state24. This is behavior10 of ordinary particle primitive1, not polygon-flash primitive10.

Primary suffix: f32 initialDistance+C8,finalDistance+CC,angularSpeed+D0,angularSpeedVariation+D4,angularAcceleration+D8,gravity+DC,latitudeSpread+E0; u8 alternateDirection+E4. Preserve E5..F7; no read of that copied tail is established in this callback.

State: radius+0,radiusStep+4,angularSpeed+8,initialPhase+C,height+10,heightStep+14,sizeScale+18,angleOffset+1C,angleScale+20.

Birth RNG order:

1. latitude=latitudeSpread*3.1415927*U()+(1-latitudeSpread)*3.1415927*0.5. This is not a uniform sphere sampler.
2. radius=sin(latitude)*initialDistance; radiusStep=(sin(latitude)*finalDistance-radius)/L.
3. angularSpeed=V(D0,D4), negated when alternateDirection!=0 and the **primary sample index** is even. Parity is not birth serial, particle age or a new random bit.
4. initialPhase=U()*6.2831855; height=cos(latitude)*initialDistance; heightStep=(cos(latitude)*finalDistance-height)/L.
5. Position=(radius*cos(initialPhase),height,radius*sin(initialPhase)). In world-space rotate it through the current serialized primary quaternion and add current primary position; local-space keeps it local. Draw common size/angle variation and set age0.

Preroll draws r=I()%L, sets height-=0.5*gravity*r*r and radius+=radiusStep*r, and reconstructs the position with **unchanged initialPhase**. It does not apply angularSpeed*r or heightStep*r in this branch. Use the current primary basis/position when world-space. Evaluate appearance at r, initialize trails and advance age to r+1 under the common rule.

Active tick at integer age a:

```
angularTerm = angularAcceleration * a * 0.5
if alternateDirection != 0 and primaryIndex is even:
    angularTerm = -angularTerm
phase = initialPhase + (state.angularSpeed + angularTerm) * a
state.radius += state.radiusStep
state.height = (state.height - gravity*a) + state.heightStep
position = (state.radius*cos(phase), state.height, state.radius*sin(phase))
```

World-space reconstruction uses the **current emitter quaternion/position for that update**, not helix behavior3's saved birth transform. InitialPhase is not overwritten. Evaluate common appearance at a with the saved previous position, apply sizeScale and angleScale/offset, propagate trails, then age=a+1. Do not replace the alternating signed analytic phase with a single accumulated angular velocity or integrate preroll by repeated active ticks.

This closes behavior10's suffix/state/birth/active/preroll contract. Behavior7..9, general trail-curve edges and primitive/backend-specific rendering remain on the master OPEN list. No runtime render qualification is claimed.
