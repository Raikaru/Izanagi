/* func_0039b2c0 (192 bytes): (re)allocates a block of arg1 five-word entries. arg0 is the owner;
 * its block header is at arg0 + *(s32 *)0x007646E0 (gp-0x4A10; gp is 0x007690F0): +0 entry count,
 * +8 the entry array. A previous array is freed through the slot at 0x008873EC (the count and
 * array are cleared), then 20*arg1 bytes are allocated from 0x008873E8 with tag 0x30105. Returns
 * 0 when that fails (the header keeps +8 = 0). Otherwise the count is stored, the array is
 * zero-filled (func_0043f9c8 is the port's memset) and arg0 is returned. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);
typedef void (*P4FreeFn)(void *ptr);

s32 func_0039b2c0(u8 *arg0, s32 arg1)
{
    u8 *hdr;
    s32 bytes;

    hdr = arg0 + *(s32 *)0x007646E0;
    if (*(u8 **)(hdr + 8) != (u8 *)0) {
        (*(P4FreeFn *)0x008873EC)(*(u8 **)(hdr + 8));
        *(u32 *)(hdr + 0) = 0;
        *(u32 *)(hdr + 8) = 0;
    }
    bytes = arg1 * 20;
    *(void **)(hdr + 8) = (*(P4AllocFn *)0x008873E8)((u32)bytes, 0x30105);
    if (*(u8 **)(hdr + 8) == (u8 *)0) {
        return 0;
    }
    *(s32 *)(hdr + 0) = arg1;
    func_0043f9c8(*(u8 **)(hdr + 8), 0, bytes);
    return (s32)arg0;
}
