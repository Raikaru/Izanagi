/* func_003bc9c0 (512 bytes): returns the index of an entry of the name table arg0 ({count, entries}, 16 bytes per entry) for a new
 * item, or -1 when memory is short. The first entry whose word +0xC (the element buffer) is 0 is reused; otherwise the table grows by one
 * entry (allocator slot 0x008873E8, tag 0x3011F; the old entries are copied with func_0043f810 and the old table freed through slot
 * 0x008873EC). The entry gets a zeroed element buffer of arg3 * func_003bd110(arg2) bytes at +0xC, the name at +0 (a copy of the string arg1 made with
 * the slot strcpy 0x008873AC, tag 0x30002, strlen slot 0x008873D4; 0 when arg1 is null or the allocation fails), +4 = arg2 and +8 = arg3. */

extern u32 p4c_003bd110_003bc9c0(u32 t) __asm__("func_003bd110");
extern void *p4c_0043f810_003bc9c0(void *d, void *s, u32 n) __asm__("func_0043f810");
extern void p4c_0043f9c8_003bc9c0(void *p, s32 v, s32 n) __asm__("func_0043f9c8");

typedef void *(*P4AllocFn_003bc9c0)(u32 size, u32 tag);
typedef void (*P4FreeFn_003bc9c0)(void *p);
typedef s32 (*P4StrlenFn_003bc9c0)(u8 *s);
typedef void (*P4StrcpyFn_003bc9c0)(u8 *d, u8 *s);

s32 func_003bc9c0(u32 *arg0, u8 *arg1, u32 arg2, u32 arg3)
{
    s32 idx = -1;
    s32 i;
    u32 *e;
    u32 *np;
    s32 n = (s32)arg0[0];

    for (i = 0; i < n; i++) {
        if (((u32 *)arg0[1])[i * 4 + 3] == 0) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        if (arg0[1] == 0) {
            arg0[1] = (u32)(*(P4AllocFn_003bc9c0 *)0x008873E8)((u32)(n + 1) << 4, 0x3011F);
            if (arg0[1] == 0) {
                return -1;
            }
        } else {
            np = (u32 *)(*(P4AllocFn_003bc9c0 *)0x008873E8)((u32)(n + 1) << 4, 0x3011F);
            if (np == (u32 *)0) {
                return -1;
            }
            p4c_0043f810_003bc9c0(np, (void *)arg0[1], arg0[0] << 4);
            (*(P4FreeFn_003bc9c0 *)0x008873EC)((void *)arg0[1]);
            arg0[1] = (u32)np;
        }
        idx = (s32)arg0[0];
        arg0[0] = (u32)(idx + 1);
    }
    e = (u32 *)(arg0[1] + (u32)idx * 16);
    e[3] = (u32)(*(P4AllocFn_003bc9c0 *)0x008873E8)(arg3 * p4c_003bd110_003bc9c0(arg2), 0x3011F);
    if (e[3] == 0) {
        return -1;
    }
    p4c_0043f9c8_003bc9c0((void *)e[3], 0, (s32)(arg3 * p4c_003bd110_003bc9c0(arg2)));
    e[0] = 0;
    if (arg1 != (u8 *)0) {
        e[0] = (u32)(*(P4AllocFn_003bc9c0 *)0x008873E8)((u32)(*(P4StrlenFn_003bc9c0 *)0x008873D4)(arg1) + 1, 0x30002);
        if (e[0] != 0) {
            (*(P4StrcpyFn_003bc9c0 *)0x008873AC)((u8 *)e[0], arg1);
        }
    }
    e[1] = arg2;
    e[2] = arg3;
    return idx;
}
