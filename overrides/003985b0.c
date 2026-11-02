/* func_003985b0 (720 bytes): copies the collision record of arg1 onto arg0 (both objects carry the record at the plugin
 * offset in 0x007646D0); returns arg0, or 0 when arg1 has no record or one cannot be allocated for arg0.
 *
 * A missing record on arg0 is allocated and zeroed (0x90 bytes from the pool allocator slot 0x008873F8, pool
 * *(u32 *)0x00884A80, tag 0x30120). func_00399bf0(arg0, src[0x80]) selects the same mode, then the two 0x40-byte entries
 * are copied by their type word (+0x20): type 1: func_0039a090 / func_0039a0f0 (size and float) read from arg1 go to
 * func_00399fd0 / func_0039a030 on arg0, and the entry words +4 and +8 are copied with their reference counts
 * (+0x54) incremented; type 2: func_0039a340, 0x3a3a0, 0x3a460 (float) and 0x3a400 are read from arg1 and written with
 * func_0039a150 (only if the first was non-zero), 0x39a200, 0x39a260 and 0x39a2e0 (float); type 4: func_0039a630 and
 * func_0039a690(arg1, &a, &b) give a value and a pair written through func_0039a4c0 (value non-zero) and
 * func_0039a590(arg0, a, b); type 5: func_0039a760(arg1, &a, &b) then func_0039a700(arg0, a, b), whose result
 * replaces both objects for the entries that follow (retail chains them through the same registers). Other types copy nothing. */

extern u32 p4c_00399bf0_003985b0(u8 *o, u32 m) __asm__("func_00399bf0");
extern void p4c_00399fd0_003985b0(u8 *o, u32 v) __asm__("func_00399fd0");
extern void p4c_0039a030_003985b0(u8 *o, f32 f) __asm__("func_0039a030");
extern u32 p4c_0039a090_003985b0(u8 *o) __asm__("func_0039a090");
extern f32 p4c_0039a0f0_003985b0(u8 *o) __asm__("func_0039a0f0");
extern void p4c_0039a150_003985b0(u8 *o, u32 v) __asm__("func_0039a150");
extern void p4c_0039a200_003985b0(u8 *o, u32 v) __asm__("func_0039a200");
extern void p4c_0039a260_003985b0(u8 *o, u32 v) __asm__("func_0039a260");
extern void p4c_0039a2e0_003985b0(u8 *o, f32 f) __asm__("func_0039a2e0");
extern u32 p4c_0039a340_003985b0(u8 *o) __asm__("func_0039a340");
extern u32 p4c_0039a3a0_003985b0(u8 *o) __asm__("func_0039a3a0");
extern u32 p4c_0039a400_003985b0(u8 *o) __asm__("func_0039a400");
extern f32 p4c_0039a460_003985b0(u8 *o) __asm__("func_0039a460");
extern void p4c_0039a4c0_003985b0(u8 *o, u32 v) __asm__("func_0039a4c0");
extern s32 p4c_0039a590_003985b0(u8 *o, u32 a, u32 b) __asm__("func_0039a590");
extern u32 p4c_0039a630_003985b0(u8 *o) __asm__("func_0039a630");
extern void p4c_0039a690_003985b0(u8 *o, u32 *a, u32 *b) __asm__("func_0039a690");
extern u32 p4c_0039a700_003985b0(u8 *o, u32 a, u32 b) __asm__("func_0039a700");
extern u32 p4c_0039a760_003985b0(u8 *o, u32 *a, u32 *b) __asm__("func_0039a760");
extern void p4c_0043f9c8_003985b0(u8 *p, s32 v, s32 n) __asm__("func_0043f9c8");

typedef void *(*P4PoolAllocFn_003985b0)(u32 pool, u32 tag);

u8 *func_003985b0(u8 *arg0, u8 *arg1)
{
    u8 *dst = arg0;
    u8 *src;
    u8 *rec;
    u8 *e;
    u8 *d;
    u32 i;
    u32 a;
    u32 b;
    u32 pair[2];
    u32 v;
    u32 v2;
    u32 v3;
    u32 v4;
    f32 f;

    src = *(u8 **)(arg1 + *(s32 *)0x007646D0);
    if (src == (u8 *)0) {
        return (u8 *)0;
    }
    rec = *(u8 **)(dst + *(s32 *)0x007646D0);
    if (rec == (u8 *)0) {
        rec = (u8 *)(*(P4PoolAllocFn_003985b0 *)0x008873F8)(*(u32 *)0x00884A80, 0x30120);
        if (rec != (u8 *)0) {
            p4c_0043f9c8_003985b0(rec, 0, 0x90);
            *(u8 **)(dst + *(s32 *)0x007646D0) = rec;
        }
    }
    if (rec == (u8 *)0) {
        return (u8 *)0;
    }
    p4c_00399bf0_003985b0(dst, *(u32 *)(src + 0x80));
    for (i = 0; i < 2; i++) {
        e = src + (i << 6);
        switch (*(s32 *)(e + 0x20)) {
        case 5:
            v = p4c_0039a760_003985b0(arg1, &a, &b);
            arg1 = (u8 *)v;
            dst = (u8 *)p4c_0039a700_003985b0(dst, a, b);
            break;
        case 4:
            v = p4c_0039a630_003985b0(arg1);
            p4c_0039a690_003985b0(arg1, &pair[0], &pair[1]);
            if (v != 0) {
                p4c_0039a4c0_003985b0(dst, v);
            }
            p4c_0039a590_003985b0(dst, pair[0], pair[1]);
            break;
        case 2:
            v = p4c_0039a340_003985b0(arg1);
            v2 = p4c_0039a3a0_003985b0(arg1);
            f = p4c_0039a460_003985b0(arg1);
            v3 = p4c_0039a400_003985b0(arg1);
            if (v != 0) {
                p4c_0039a150_003985b0(dst, v);
            }
            p4c_0039a200_003985b0(dst, v2);
            p4c_0039a260_003985b0(dst, v3);
            p4c_0039a2e0_003985b0(dst, f);
            break;
        case 1:
            d = rec + (i << 6);
            v = p4c_0039a090_003985b0(arg1);
            f = p4c_0039a0f0_003985b0(arg1);
            p4c_00399fd0_003985b0(dst, v);
            p4c_0039a030_003985b0(dst, f);
            *(u32 *)(d + 4) = *(u32 *)(e + 4);
            *(u32 *)(d + 8) = *(u32 *)(e + 8);
            v4 = *(u32 *)(d + 4);
            if (v4 != 0) {
                *(s32 *)(v4 + 0x54) += 1;
            }
            v4 = *(u32 *)(d + 8);
            if (v4 != 0) {
                *(s32 *)(v4 + 0x54) += 1;
            }
            break;
        default:
            break;
        }
    }
    return arg0;
}
