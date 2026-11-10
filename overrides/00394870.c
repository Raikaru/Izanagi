/* func_00394870 (1016 bytes): runs the sphere arg2 (centre, radius at +0xC) against every triangle of the morphed geometry arg0 and calls arg4 for each hit. arg0 holds the triangle
 * count at +0x10, the triangles at +0x2C (8 bytes each: three halfword vertex indices) and the morph target table at +0x5C (28-byte entries whose word +0x14 is the vertex array).
 * arg1 selects the two morph targets (signed halfwords +4 and +6) and the blend factor (the product of the floats at +0xC and +0x10). With one array the corners are the array
 * entries (12 bytes each); with two they are v0 + (v1 - v0) * blend, built in locals. func_003d9760(sphere, A, B, C, &rec, &dist) tests the triangle; on a hit the record
 * rec = {normal (3 floats), A (3 floats), triangle index, &A, &B, &C} is passed to arg4(arg3, &rec, arg5, dist / radius) and a zero result stops the walk (returns 0). The result is the callback's last nonzero result when the final triangle was a hit it accepted, else 0. */

extern s32 p4c_003d9760_00394870(f32 *s, f32 *a, f32 *b, f32 *c, f32 *n, f32 *d) __asm__("func_003d9760");

/* The callbacks are the decomp's RpCollisionTriangleCallBack shape (intersection, triangle, distance, data): the float is the THIRD argument on the host stack; MIPS passes it in f12 whatever its position, so the host order, not the register order, must match the callee. */
typedef s32 (*P4HitFn_00394870)(u32 ctx, void *rec, f32 dist, u32 ctx2);

s32 func_00394870(u8 *arg0, u8 *arg1, f32 *arg2, u32 arg3, P4HitFn_00394870 arg4, u32 arg5)
{
    f32 invr = 1.0f / arg2[3];
    u8 *arrays[2];
    f32 blend;
    s32 n = *(s32 *)(arg0 + 0x10);
    u16 *tri = *(u16 **)(arg0 + 0x2C);
    f32 va[3];
    f32 vb[3];
    f32 vc[3];
    f32 *pa = va;
    f32 *pb = vb;
    f32 *pc = vc;
    u32 rec[12];
    f32 dist = 0.0f;
    s32 i;
    u32 k;
    u32 idx[3];
    f32 *dst[3];
    u8 *p0;
    u8 *p1;
    f32 *v0;
    f32 *v1;
    f32 d;
    f32 q;
    u32 w;
    s32 ret = 0;
    s32 r;

    arrays[0] = *(u8 **)(arg0 + 0x5C) + *(s16 *)(arg1 + 4) * 28;
    arrays[0] = *(u8 **)(arrays[0] + 0x14);
    arrays[1] = *(u8 **)(arg0 + 0x5C) + *(s16 *)(arg1 + 6) * 28;
    arrays[1] = *(u8 **)(arrays[1] + 0x14);
    blend = *(f32 *)(arg1 + 0xC) * *(f32 *)(arg1 + 0x10);
    for (i = 0; i < 12; i++) {
        rec[i] = 0;
    }
    for (i = 0; i < n; i++, tri += 4) {
        idx[0] = tri[0];
        idx[1] = tri[1];
        idx[2] = tri[2];
        if (arrays[0] == arrays[1]) {
            pa = (f32 *)(arrays[0] + idx[0] * 12);
            pb = (f32 *)(arrays[0] + idx[1] * 12);
            pc = (f32 *)(arrays[0] + idx[2] * 12);
        } else {
            dst[0] = va;
            dst[1] = vb;
            dst[2] = vc;
            pa = va;
            pb = vb;
            pc = vc;
            for (k = 0; k < 3; k++) {
                v0 = (f32 *)(arrays[0] + idx[k] * 12);
                v1 = (f32 *)(arrays[1] + idx[k] * 12);
                for (w = 0; w < 3; w++) {
                    d = v1[w] - v0[w];
                    q = d * blend;
                    dst[k][w] = q + v0[w];
                }
            }
        }
        ret = 0;
        if (p4c_003d9760_00394870(arg2, pa, pb, pc, (f32 *)rec, &dist) != 0) {
            rec[3] = *(u32 *)&pa[0];
            rec[4] = *(u32 *)&pa[1];
            rec[5] = *(u32 *)&pa[2];
            rec[6] = (u32)i;
            rec[7] = (u32)pa;
            rec[8] = (u32)pb;
            rec[9] = (u32)pc;
            dist = dist * invr;
            r = arg4(arg3, rec, dist, arg5);
            if (r == 0) {
                return 0;
            }
            ret = r;
        }
    }
    return ret;
}
