/* func_00392a50 (416 bytes): walks the triangles of the mesh arg0 and calls the callback arg3 for each one hit by the
 * probe arg1. The mesh has its triangle count at +0x10, an index list at +0x2C (8 bytes per triangle; the first three halfwords
 * index the vertices) and a pointer at +0x5C whose +0x14 is the vertex array (12 bytes per vertex). func_003d9760(arg1,
 * v0, v1, v2, &hit, &dist) tests a triangle (non-zero: hit; it fills hit[0..2] and dist); a hit has dist multiplied by
 * 1.0 / (float at arg1 + 0xC), the first vertex copied to hit[3..5], the triangle number stored at hit[6] and the three
 * vertex pointers at hit[7..9], and arg3(arg2, &hit, arg4, dist) is called (hit is 12 words, dist the word at +0x30).
 * A callback result of 0 stops the walk. Returns the last test or callback result. */

extern s32 p4c_003d9760_00392a50(u8 *p, f32 *a, f32 *b, f32 *c, void *hit, f32 *dist) __asm__("func_003d9760");

typedef struct P4Hit_00392a50 {
    f32 a[3];
    f32 pos[3];
    s32 tri;
    f32 *v[3];
    f32 pad;
    f32 dist;
} P4Hit_00392a50;

/* The callbacks are the decomp's RpCollisionTriangleCallBack shape (intersection, triangle, distance, data): the float is the THIRD argument on the host stack; MIPS passes it in f12 whatever its position, so the host order, not the register order, must match the callee. */
typedef s32 (*P4HitFn_00392a50)(u32 arg2, P4Hit_00392a50 *hit, f32 dist, u32 arg4);

s32 func_00392a50(u8 *arg0, u8 *arg1, u32 arg2, P4HitFn_00392a50 arg3, u32 arg4)
{
    f32 inv = 1.0f / *(f32 *)(arg1 + 0xC);
    u8 *mesh = *(u8 **)(arg0 + 0x5C);
    s32 n = *(s32 *)(arg0 + 0x10);
    u16 *tri = *(u16 **)(arg0 + 0x2C);
    f32 *verts = *(f32 **)(mesh + 0x14);
    P4Hit_00392a50 hit;
    f32 *v0;
    f32 *v1;
    f32 *v2;
    s32 i;
    s32 last = 0;

    if (n <= 0) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        v0 = verts + tri[0] * 3;
        v1 = verts + tri[1] * 3;
        v2 = verts + tri[2] * 3;
        last = p4c_003d9760_00392a50(arg1, v0, v1, v2, &hit, &hit.dist);
        if (last != 0) {
            hit.dist = hit.dist * inv;
            hit.pos[0] = v0[0];
            hit.pos[1] = v0[1];
            hit.pos[2] = v0[2];
            hit.v[0] = v0;
            hit.v[1] = v1;
            hit.v[2] = v2;
            hit.tri = i;
            last = arg3(arg2, &hit, hit.dist, arg4);
            if (last == 0) {
                return 0;
            }
        }
        tri += 4;
    }
    return last;
}
