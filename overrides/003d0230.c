/* func_003d0230 (560 bytes): for the draw record arg0 (+8 the object o) whose geometry g = o[6] has flag 0x20 in g[2] (otherwise nothing happens), registers the shadow
 * receivers that are near the object's light. The halfword counter at 0x008872EA is incremented, the matrix m = func_003e9700(o[1]) is copied to a local with func_003e0960 and the
 * local context c is built: c[0] = the word at (b + *(b + 0xC) + 0x18) plus 0xC with b = g[0x54 / 4], the copied matrix at c + 0x10, c[0x50] the length of the matrix row +0x20
 * (1.0 when the matrix flag word m[3] & 3 == 3) and c[0x54] its reciprocal (1.0 when that flag is set, the length itself when it is <= 0). func_003c96d0(func_003cffa0, c) runs the
 * callback over it. Then every node n of the list at o + 0x64 (next pointers at +0) is visited, each with the sub list at n[2] + 0x40: for an entry e with a non null
 * record r = e[2] whose halfword at r + 0x3C differs from the counter and with bit 0 of the byte at r + 2 set, the halfword is set to the counter; with the sphere s = func_003bfae0(o) (centre
 * x, y, z, radius) and the position p = func_003e9700(r[1]) + 0x30 the record is passed to func_003cfa80(r, c + 0x10, c[0x50], c[0x54]) when the squared distance between s and p is
 * below (s radius + r[0x14 / 4]) squared and bit 0 of the byte at r + 2 is still set. */

extern u8 *p4c_003bfae0_003d0230(u8 *o) __asm__("func_003bfae0");
extern s32 p4c_003c96d0_003d0230(void *cb, void *c) __asm__("func_003c96d0");
extern void p4c_003cfa80_003d0230(u8 *r, void *m, f32 a, f32 b) __asm__("func_003cfa80");
extern u8 *p4c_003cffa0_003d0230(u8 *a, u8 *b) __asm__("func_003cffa0");
extern void p4c_003e0960_003d0230(void *d, void *s) __asm__("func_003e0960");
extern u8 *p4c_003e9700_003d0230(u32 f) __asm__("func_003e9700");

void func_003d0230(u8 *arg0)
{
    u8 *o = *(u8 **)(arg0 + 8);
    u8 *g = *(u8 **)(o + 0x18);
    u32 c[24];
    u32 i;
    u8 *m;
    u8 *base;
    u8 *n;
    u8 *nend;

    if (!(*(u32 *)(g + 8) & 0x20)) {
        return;
    }
    *(u16 *)0x008872EA = *(u16 *)0x008872EA + 1;
    m = (u8 *)p4c_003e9700_003d0230(*(u32 *)(o + 4));
    for (i = 0; i < 24; i++) {
        c[i] = 0;
    }
    p4c_003e0960_003d0230(&c[4], m);
    if ((*(u32 *)(m + 0xC) & 3) == 3) {
        ((f32 *)c)[20] = 1.0f;
        ((f32 *)c)[21] = 1.0f;
    } else {
        f32 l = ((f32 *)c)[13] * ((f32 *)c)[13];

        l = l + ((f32 *)c)[12] * ((f32 *)c)[12];
        l = l + ((f32 *)c)[14] * ((f32 *)c)[14];
        l = __builtin_sqrtf(l);
        ((f32 *)c)[20] = l;
        if (!(l <= 0.0f)) {
            l = 1.0f / l;
        }
        ((f32 *)c)[21] = l;
    }
    base = *(u8 **)(g + 0x54);
    c[0] = *(u32 *)(base + *(u32 *)(base + 0xC) + 0x18) + 0xC;
    p4c_003c96d0_003d0230((void *)p4c_003cffa0_003d0230, c);
    n = *(u8 **)(o + 0x64);
    nend = o + 0x64;
    while (n != nend) {
        u8 *sub = *(u8 **)(n + 8);
        u8 *e = *(u8 **)(sub + 0x40);
        u8 *eend = sub + 0x40;

        while (e != eend) {
            u8 *r = *(u8 **)(e + 8);

            if (r != 0 && *(u16 *)(r + 0x3C) != *(u16 *)0x008872EA && (*(u8 *)(r + 2) & 1)) {
                u8 *s;
                u8 *p;
                u8 *q;
                f32 d0, d1, d2, rr;

                *(u16 *)(r + 0x3C) = *(u16 *)0x008872EA;
                q = (u8 *)p4c_003e9700_003d0230(*(u32 *)(*(u8 **)(e + 8) + 4));
                p = q + 0x30;
                s = p4c_003bfae0_003d0230(o);
                r = *(u8 **)(e + 8);
                d1 = *(f32 *)(s + 4) - *(f32 *)(p + 4);
                d0 = *(f32 *)s - *(f32 *)p;
                d2 = *(f32 *)(s + 8) - *(f32 *)(p + 8);
                d1 = d1 * d1;
                d1 = d1 + d0 * d0;
                d1 = d1 + d2 * d2;
                rr = *(f32 *)(s + 0xC) + *(f32 *)(r + 0x14);
                rr = rr * rr;
                if (d1 < rr && (*(u8 *)(r + 2) & 1)) {
                    p4c_003cfa80_003d0230(r, &c[4], ((f32 *)c)[20], ((f32 *)c)[21]);
                }
            }
            e = *(u8 **)e;
        }
        n = *(u8 **)n;
    }
}
