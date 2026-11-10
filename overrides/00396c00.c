/* func_00396c00 (640 bytes): reads a frame's hierarchy link from a stream (the mirror of
 * func_00396a40). arg0 is the stream (the reader is func_003df360(stream, pointer, 4), which
 * returns 0 on failure), arg2 the frame; arg1 is not used. Returns the stream, or 0 on a short read
 * or a stream that does not start with 0x100.
 *
 * After the magic word the link id is read into frame + *(s32 *)0x007646C8 (gp-0x4A28) +0, then
 * the node count. A count of 0 or less ends the read (link untouched beyond the id). Otherwise a
 * flags word and a second word are read, and a record is built as in func_00396fb0 (freelist block
 * from 0x008873F8 with pool 0x007646CC, tag 0x3011E, zero-filled 0x24 bytes): +0x20 =
 * func_003d5790(count, second word), +0 = flags, +0x14 = the frame, +4 = count, +0x18 = itself; with
 * flags bit 1 clear +8/+0xC hold a 16-aligned block of count*64 bytes (allocator 0x008873E8, tag
 * 0x3011E, size count*64+15), else both are 0; +0x10 is count*16 bytes from the same allocator. Each
 * node's +0, +4 and +8 words are read from the stream and +0xC cleared. The record becomes the
 * link's +4. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);
typedef void *(*P4PoolAllocFn)(u32 pool, u32 tag);

s32 func_00396c00(s32 arg0, s32 arg1, u8 *arg2)
{
    s32 magic;
    s32 count;
    s32 flags;
    s32 extra;
    u8 *link;
    u8 *rec;
    u8 *node;
    s32 i;

    (void)arg1;
    link = arg2 + *(s32 *)0x007646C8;
    if (func_003df360(arg0, &magic, 4) == 0) {
        return 0;
    }
    if (magic != 0x100) {
        return 0;
    }
    if (func_003df360(arg0, (s32 *)link, 4) == 0) {
        return 0;
    }
    if (func_003df360(arg0, &count, 4) == 0) {
        return 0;
    }
    if (count <= 0) {
        return arg0;
    }
    if (func_003df360(arg0, &flags, 4) == 0) {
        return 0;
    }
    if (func_003df360(arg0, &extra, 4) == 0) {
        return 0;
    }
    rec = (u8 *)(*(P4PoolAllocFn *)0x008873F8)(*(u32 *)0x007646CC, 0x3011E);
    func_0043f9c8(rec, 0, 0x24);
    if (rec != (u8 *)0) {
        *(void **)(rec + 0x20) = func_003d5790(count, extra);
        *(s32 *)(rec + 0) = flags;
        *(u8 **)(rec + 0x14) = arg2;
        *(s32 *)(rec + 4) = count;
        *(u8 **)(rec + 0x18) = rec;
        if (*(u32 *)rec & 2) {
            *(u32 *)(rec + 8) = 0;
            *(u32 *)(rec + 0xC) = 0;
        } else {
            u8 *raw;

            raw = (u8 *)(*(P4AllocFn *)0x008873E8)((u32)(count * 64 + 15), 0x3011E);
            *(u32 *)(rec + 8) = (u32)(raw + 0xF) & 0xFFFFFFF0u;
            *(u8 **)(rec + 0xC) = raw;
        }
        *(u8 **)(rec + 0x10) = (u8 *)(*(P4AllocFn *)0x008873E8)((u32)(count << 4), 0x3011E);
        node = *(u8 **)(rec + 0x10);
        i = 0;
        if (0 < *(s32 *)(rec + 4)) {
            do {
                if (func_003df360(arg0, (s32 *)node, 4) == 0) {
                    return 0;
                }
                if (func_003df360(arg0, (s32 *)(node + 4), 4) == 0) {
                    return 0;
                }
                if (func_003df360(arg0, (s32 *)(node + 8), 4) == 0) {
                    return 0;
                }
                *(u32 *)(node + 0xC) = 0;
                i += 1;
                node += 0x10;
            } while (i < *(s32 *)(rec + 4));
        }
    }
    *(u8 **)(link + 4) = rec;
    return arg0;
}
