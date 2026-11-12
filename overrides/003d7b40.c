/* func_003d7b40 (272 bytes): creates a named entry. Its sixth argument is the float in f12 (the animation's duration), which retail never touches before the calls to func_003d51c0, so it passes through. arg4 selects the source table: 0 gives s = func_003d51c0(*(u32 *)0x0070B650,
 * arg2, 0) and 1 gives func_003d51c0(*(u32 *)0x0070B680, arg2, 0); any other mode, or s = 0, returns 0. A block of 0x44+ bytes
 * (pool *(u32 *)0x00764838, tag 0x30135, slot 0x008873F8; failing returns 0) receives the name (strncpy of arg0, 0x20
 * bytes with byte 0x1F cleared), arg1 words copied from arg3 at +0x20, and +0x40 = 1; s[5] points at it. Returns s. */

extern u32 p4c_003d51c0_003d7b40(u32 t, u32 k, s32 z, f32 duration) __asm__("func_003d51c0");
extern void *p4c_0043f810_003d7b40(void *d, void *s, u32 n) __asm__("func_0043f810");
extern u8 *p4c_00442de8_003d7b40(u8 *d, u8 *s, u32 n) __asm__("func_00442de8");

typedef void *(*P4PoolAllocFn_003d7b40)(u32 pool, u32 tag);

u32 *func_003d7b40(u8 *arg0, s32 arg1, u32 arg2, u8 *arg3, s32 arg4, f32 duration)
{
    u32 *s = (u32 *)0;
    u8 *blob;

    if (arg4 == 1) {
        s = (u32 *)p4c_003d51c0_003d7b40(*(u32 *)0x0070B680, arg2, 0, duration);
    } else if (arg4 == 0) {
        s = (u32 *)p4c_003d51c0_003d7b40(*(u32 *)0x0070B650, arg2, 0, duration);
    }
    if (s == (u32 *)0) {
        return (u32 *)0;
    }
    blob = (u8 *)(*(P4PoolAllocFn_003d7b40 *)0x008873F8)(*(u32 *)0x00764838, 0x30135);
    if (blob == (u8 *)0) {
        return (u32 *)0;
    }
    p4c_00442de8_003d7b40(blob, arg0, 0x20);
    blob[0x1F] = 0;
    p4c_0043f810_003d7b40(blob + 0x20, arg3, (u32)arg1 << 2);
    *(u32 *)(blob + 0x40) = 1;
    s[5] = (u32)blob;
    return s;
}
