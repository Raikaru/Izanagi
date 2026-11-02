/* func_003bcc80 (208 bytes): copies a list of 16-byte entries between two owners. The headers are
 * arg0 + arg2 (destination) and arg1 + arg2 (source): +0 count, +4 entries. func_003bc740(destination)
 * clears the destination first and the count is copied. With a count above 0 the destination's entries
 * are allocated (count * 16 bytes from the slot 0x008873E8, tag 0x3011F) and entry i is copied with
 * func_003bbf80(dst entry, src entry). Returns arg0. */

extern void p4c_003bbf80_003bcc80(u8 *d, u8 *s) __asm__("func_003bbf80");
extern void p4c_003bc740_003bcc80(u8 *p) __asm__("func_003bc740");

typedef void *(*P4AllocFn_003bcc80)(u32 size, u32 tag);

u8 *func_003bcc80(u8 *arg0, u8 *arg1, s32 arg2)
{
    u8 *dst;
    u8 *src;
    s32 i;

    dst = arg0 + arg2;
    src = arg1 + arg2;
    p4c_003bc740_003bcc80(dst);
    *(s32 *)dst = *(s32 *)src;
    if (*(s32 *)dst <= 0) {
        return arg0;
    }
    *(void **)(dst + 4) = (*(P4AllocFn_003bcc80 *)0x008873E8)((u32)(*(s32 *)dst << 4), 0x3011F);
    for (i = 0; i < *(s32 *)dst; i++) {
        p4c_003bbf80_003bcc80(*(u8 **)(dst + 4) + 16 * i, *(u8 **)(src + 4) + 16 * i);
    }
    return arg0;
}
