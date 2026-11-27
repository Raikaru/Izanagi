# M8f ordinary particle trail propagation

Source: ordinary NSO35BD40, pivotal branch/copy/color instructions35BD70..35BEC8 and spline helper3614D0. This page closes ordinary history shift, fade, interpolation and low-count edges. It does not replace primitive-specific track geometry or its renderer. Offsets/sizes hex unless stated decimal.

## Storage and measured legal domain

Let N=u32 primary+C0, W=u32 primary+C4, T=N*W. T0 returns without touching trail storage. The ordinary allocator gives each primary T additional58-byte sample records after the complete primary sample array. For primary index i, its trail block starts at `samples[primaryCount+i*T]`. These are full Golden samples, not PS2 native20-byte particles or just XYZ vertices.

A new distinct-source-span survey traversed the pinned881 root identities, checked each extracted root's size/SHA and inspected2928 relevant physical descriptors (primitive1 and the established common-prefix offsets for2/15/28/29). Sixty-eight primitive/behavior/N/W combinations; zero extraction/range exceptions and no negative signed N/W. Observed ordinary primitive1 pairs, in decimal: (0,0),(0,1),(0,3),(0,7),(1,1),(1,2),(2,1),(2,2),(2,3),(3,1),(3,2),(3,3),(4,1),(4,2),(5,4). Zero N with nonzero W is legal and still returns via T0.

Five measured descriptors use N1/W2, including F_HAKEN.EPL and028.BED. **They are not malformed.** The actual native branch exits interpolation for N<2; an ambiguous decompiler `else` placement must not be used to invent an out-of-bounds read of trail[W] for these cases.

## History shift and opacity envelope

Let A be trunc(primary.age). At A0, copy the complete primary sample into trail[0], then set every trail sample's age tofloat-1. No other history is active yet.

For A!=0:

1. Temporarily set trail[0].age=A-1.
2. Starting at the oldest end, shift T-W samples forward by W. Equivalently for j=0..T-W-1, copy full sample `trail[T-W-1-j]` to `trail[T-1-j]`. Copy order is descending so sources are not overwritten.
3. For each shifted destination, replace only its packed color's highest byte by trunc(f*primary.highestColorByte), with f initially0 and repeatedly incremented by1/T after each copy. Keep the lower24 bits. Preserve the repeated floating addition; do not recompute a different index-normalized ramp or multiply the previous alpha repeatedly.
4. Copy the complete current primary into trail[0] and set only its age tofloat-1. That hidden head is the interpolation endpoint/history source, not an extra live sprite.

The actual packing uses raw u32 words:35BE4C..35BE64 loads the old packed word, converts the computed ramp to unsigned integer, inserts8 bits at24 and stores the word. Decompiler float-pointer casts are not numeric conversions of packed colors.

If W==1 or N<2, stop after this shift/copy. Thus N1/W2 legally has no interpolated historical key; it does not borrow a sample from the next primary's block. Lifetime/death logic separately invalidates the primary's entire T-slot block, as specified with the controllers.

## Selecting linear or cubic interpolation

After shifting, let C=trail[0] (current hidden head), B=trail[W] (previous key), and, when available, Aold=trail[2*W]. W>1 and N>=2 are required before reading B.

- If N>=3 and current integer age>=2: use the cubic branch only when Aold.age>=0; otherwise return without replacing interpolants.
- Otherwise use the linear branch only when current age>0 and B.age>=0.

There are W-1 interpolation writes, descending destinations W-1,W-2,..,1. Their successive fractions are t=1/(W+1),2/(W+1),..,(W-1)/(W+1), formed by repeated addition. The denominator is **W+1**, not W. For each destination set age to the current primary integer age, interpolate current size and angle from B to C linearly, and write current position/color as below. Other sample fields remain whatever the complete history copy provided; do not allocate a new behavior state for each interpolant.

## Position and appearance interpolation

Linear position is (1-t)*B.position+t*C.position, including the ordinary quad's W interpolation. No shortest-angle wrap is applied to size/angle interpolation: angle=B.angle+(C.angle-B.angle)*t.

Cubic helper3614D0 receives four quads(Aold.position,B.position,C.position,C.position). For XYZ, with t2=t*t,t3=t2*t:

```
M0=(C-Aold)*0.5
M1=(C-B)*0.5
P=B*(2*t3-3*t2+1)
 +M0*(t3-2*t2+t)
 +C*(-2*t3+3*t2)
 +M1*(t3-t2)
```

It is Catmull-Rom/Hermite interpolation from B to C with a repeated final point, not a Bezier using those four points as controls. The native helper's half-vector has W0 and its temporary coefficient matrix is zeroed; cubic output W is0. Do not infer another spatial coordinate from W.

For packed-color interpolation, unpack the four components in order(byte24,byte16,byte8,byte0), normalize by255, and start from B. Increment per interpolant by `(C-B)/(W+1)` for the **first three** components; the fourth component's increment is multiplied by0 in the native vector operation. Thus its byte0 stays at B's value in these interpolants. Repack using trunc(component*255), shifts24/16/8/0, without introducing rounding+0.5 or a different channel convention. This is separate from the highest-byte fade performed during history shift. Both the linear and cubic position branches use the same appearance interpolation.

## Update/draw ownership

35BD40 runs from ordinary controller updates after their current primary appearance evaluation and before age increment. It is not run every draw or every fractional interpolation of the main sample. Golden35B720 interpolates primary endpoint samples according to its existing scheduler; trail history remains the discrete controller output described here. Sample death/reset and behavior-specific preroll initialization are separate contracts; do not normalize all controller birth branches to a common analytic preroll.

Actual track/line/particle geometry, nested EP/EPL per-sample children and image/vertex-buffer ownership still use their real primitive backends. This page establishes the trail sample values and edge branches, not a claim that a polyline substitute renders those backends correctly.
