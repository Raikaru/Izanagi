/* func_00399db0 (536 bytes): replaces the texture bound to the type-1 slot entry of the object arg0's collision
 * record with arg1 (null: just unbinds). Returns arg0, or 0 when the new binding cannot be made.
 *
 * The record is the pointer at arg0 + the plugin offset in 0x007646D0; its entry is the first of its two 0x40-byte slot
 * entries whose type word (+0x20) is 1 (none: 0 is returned here, retail would index the null page). The old words at +8 and +4 are released
 * through func_003ef3a0 and cleared (+4 clearing +0x10 as well). With a new texture: it is stored at +8 with its
 * reference count (+0x54) bumped; when neither the texture's first word (a pointer) nor the one at arg0[0] points at a
 * zero word at +0xC, i.e. both are non-zero there, the combined name is built in a 32-byte buffer by func_00399470(buf, arg0[0], arg1), the cached
 * object func_003ef6d0() is asked for func_003ef650(it, buf) (kept at +4 with its count bumped), or else
 * func_0039ab10(arg0[0], arg1) makes one (0 returns 0; func_003ef5b0(it) then finishes the lookup). +0x10 is set to
 * 1.0 / (float)(**(entry+4))[3] (the +0xC word), and likewise 1.0 / (float)(**(*arg0))[3] when arg1 is null. */

extern void p4c_00399470_00399db0(u8 *o, u8 *a, u8 *b) __asm__("func_00399470");
extern u32 p4c_0039ab10_00399db0(u8 *a, u8 *b) __asm__("func_0039ab10");
extern void p4c_003ef3a0_00399db0(u32 t) __asm__("func_003ef3a0");
extern void p4c_003ef5b0_00399db0(u32 a, u32 b) __asm__("func_003ef5b0");
extern u32 p4c_003ef650_00399db0(u32 a, u8 *name) __asm__("func_003ef650");
extern u32 p4c_003ef6d0_00399db0(void) __asm__("func_003ef6d0");

u8 *func_00399db0(u8 *arg0, u8 *arg1)
{
    u8 *rec;
    u8 *e;
    u8 *s16;
    u32 s20;
    u32 k;
    u8 buf[32];
    u32 i;
    u8 *t;

    rec = *(u8 **)(arg0 + *(s32 *)0x007646D0);
    e = (u8 *)0;
    for (k = 0; k < 2; k++) {
        if (*(s32 *)(rec + k * 64 + 0x20) == 1) {
            e = rec + k * 64;
            break;
        }
    }
    if (e == (u8 *)0) {
        return (u8 *)0;
    }
    if (*(u32 *)(e + 8) != 0) {
        p4c_003ef3a0_00399db0(*(u32 *)(e + 8));
        *(u32 *)(e + 8) = 0;
    }
    if (*(u32 *)(e + 4) != 0) {
        p4c_003ef3a0_00399db0(*(u32 *)(e + 4));
        *(u32 *)(e + 4) = 0;
        *(u32 *)(e + 0x10) = 0;
    }
    if (arg1 == (u8 *)0) {
        t = **(u8 ***)arg0;
        *(f32 *)(e + 0x10) = 1.0f / (f32)*(s32 *)(t + 0xC);
        return arg0;
    }
    *(u8 **)(e + 8) = arg1;
    *(s32 *)(*(u8 **)(e + 8) + 0x54) += 1;
    s16 = *(u8 **)arg0;
    if (*(s32 *)(*(u8 **)arg1 + 0xC) == 0 || (s16 != (u8 *)0 && *(s32 *)(*(u8 **)s16 + 0xC) == 0)) {
        return arg0;
    }
    for (i = 0; i < 32; i++) {
        buf[i] = 0;
    }
    p4c_00399470_00399db0(buf, s16, arg1);
    s20 = p4c_003ef6d0_00399db0();
    *(u32 *)(e + 4) = 0;
    if (s20 != 0) {
        *(u32 *)(e + 4) = p4c_003ef650_00399db0(s20, buf);
    }
    if (*(u32 *)(e + 4) != 0) {
        *(s32 *)(*(u8 **)(e + 4) + 0x54) += 1;
    } else {
        *(u32 *)(e + 4) = p4c_0039ab10_00399db0(s16, arg1);
        if (*(u32 *)(e + 4) == 0) {
            return (u8 *)0;
        }
        if (s20 != 0) {
            p4c_003ef5b0_00399db0(s20, *(u32 *)(e + 4));
        }
    }
    t = **(u8 ***)(e + 4);
    *(f32 *)(e + 0x10) = 1.0f / (f32)*(s32 *)(t + 0xC);
    return arg0;
}
