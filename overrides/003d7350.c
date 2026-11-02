/* func_003d7350 (304 bytes): copies the animation slots of the object arg1 to the object arg0 (both slot tables at the object + the
 * offset word at gp - 0x48B4, 0x0076483C; the eight slot words start at +8) and returns arg0, or 0 when a copy fails. For each
 * slot with an interpolator s: its animation a = s[0] is sized with func_003d5750(a) and a new interpolator func_003d5790(count,
 * (*a)[1]) is made into the destination slot (0 fails), initialised with func_003d59a0(new, s) and func_003d7cd0(a) is called; an
 * empty source slot clears the destination slot. On a failure the destination slots made so far (those before the failed one) are released with
 * func_003d7c50(first word) and func_003d5830(slot). */

extern s32 p4c_003d5750_003d7350(u8 *a) __asm__("func_003d5750");
extern u32 p4c_003d5790_003d7350(s32 n, u32 s) __asm__("func_003d5790");
extern void p4c_003d5830_003d7350(u32 a) __asm__("func_003d5830");
extern void p4c_003d59a0_003d7350(u32 a, u32 b) __asm__("func_003d59a0");
extern void p4c_003d7c50_003d7350(u32 a) __asm__("func_003d7c50");
extern void p4c_003d7cd0_003d7350(u32 a) __asm__("func_003d7cd0");

u8 *func_003d7350(u8 *arg0, u8 *arg1)
{
    s32 off = *(s32 *)0x0076483C;
    u32 *dst = (u32 *)(arg0 + off);
    u32 *src = (u32 *)(arg1 + off);
    u32 i;
    u32 j;
    u32 *s;
    u32 *a;
    u32 r;

    for (i = 0; i < 8; i++) {
        s = (u32 *)src[2 + i];
        if (s == (u32 *)0) {
            dst[2 + i] = 0;
            continue;
        }
        a = (u32 *)s[0];
        r = p4c_003d5790_003d7350(p4c_003d5750_003d7350((u8 *)a), ((u32 *)a[0])[1]);
        dst[2 + i] = r;
        if (r == 0) {
            if (i == 0) {
                return (u8 *)0;
            }
            for (j = 0; j < i; j++) {
                u32 *p = (u32 *)dst[2 + j];

                if (p != (u32 *)0) {
                    p4c_003d7c50_003d7350(p[0]);
                    p4c_003d5830_003d7350((u32)p);
                }
            }
            return (u8 *)0;
        }
        p4c_003d59a0_003d7350(r, (u32)s);
        p4c_003d7cd0_003d7350((u32)a);
    }
    return arg0;
}
