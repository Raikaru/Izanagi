/* func_003bcbe0 (152 bytes): destroys a list of 16-byte entries. arg0 + arg1 is the list header (+0
 * count, +4 entries). When entries exist func_003bbea0 runs on each and the block is freed through the
 * slot 0x008873EC. The header is cleared and arg0 returned. */

extern void p4c_003bbea0_003bcbe0(u8 *p) __asm__("func_003bbea0");

typedef void (*P4FreeFn_003bcbe0)(void *p);

u8 *func_003bcbe0(u8 *arg0, s32 arg1)
{
    u8 *hdr;
    s32 i;

    hdr = arg0 + arg1;
    if (*(u32 *)(hdr + 4) != 0) {
        if (*(s32 *)hdr > 0) {
            i = 0;
            do {
                p4c_003bbea0_003bcbe0(*(u8 **)(hdr + 4) + 16 * i);
                i += 1;
            } while (i < *(s32 *)hdr);
        }
        (*(P4FreeFn_003bcbe0 *)0x008873EC)(*(void **)(hdr + 4));
    }
    *(u32 *)(hdr + 4) = 0;
    *(u32 *)(hdr + 0) = 0;
    return arg0;
}
