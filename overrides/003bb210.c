/* func_003bb210 (288 bytes): reads a table object from a stream. arg0 is the stream (func_003e2910
 * reads a block and returns the byte count, func_003df300 reads into a buffer and returns 0 on
 * failure). Returns the table, or 0 on a short read or allocation failure.
 *
 * A 0x28-byte header is read (it must come back whole). Header word 8 (+0x20) is the entry count n
 * and word 9 (+0x24) the kind passed on; n*12 bytes are allocated from the slot 0x008873E8 (tag
 * 0x10102) and read from the stream; func_003bbbe0(n, kind, buffer) builds the table; the header's
 * first 0x20 bytes are copied to table+0x18 with the slot 0x008873B0 (memcpy), the table's first byte
 * is set to 0xC and the temporary buffer is freed through the slot 0x008873EC. A failed allocation
 * reports error {0x102, func_003df590(0x80000013, n*12)} through func_003df4d0 and returns 0. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);
typedef void (*P4FreeFn)(void *ptr);
typedef void *(*P4CopyFn)(void *dst, const void *src, u32 n);

u8 *func_003bbbe0(s32 arg0, s32 arg1, u8 *arg2);

u8 *func_003bb210(s32 arg0)
{
    u32 hdr[10];
    s32 n;
    s32 size;
    u8 *buf;
    u8 *table;

    if (func_003e2910(arg0, hdr, 0x28) != 0x28) {
        return (u8 *)0;
    }
    n = (s32)hdr[8];
    size = n * 12;
    buf = (u8 *)(*(P4AllocFn *)0x008873E8)((u32)size, 0x10102);
    if (buf == (u8 *)0) {
        s32 err[2];

        err[0] = 0x102;
        err[1] = func_003df590(0x80000013, size);
        func_003df4d0(err);
        return (u8 *)0;
    }
    if (func_003df300(arg0, buf, size) == 0) {
        return (u8 *)0;
    }
    table = (u8 *)func_003bbbe0(n, hdr[9], buf);
    (*(P4CopyFn *)0x008873B0)(table + 0x18, hdr, 0x20);
    *(u8 *)table = 0xC;
    (*(P4FreeFn *)0x008873EC)(buf);
    return table;
}
