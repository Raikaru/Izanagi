/* func_003c8920 (720 bytes): builds the grouped index block of a mesh from the triangle list arg0 ({+4 = count n, +8 = array of 20-byte triangles})
 * and returns it (0 when an allocation fails: {2, func_003df590(0x80000013, size)} goes to func_003df4d0). A pointer table over the triangles
 * (allocator slot 0x008873E8, tag 0x10502, 4 * n bytes) is sorted with func_00440bb8(table, n, 4, func_003c6080), which orders them by
 * material. The block (func_003c54a0(0x10 + 12 * groups + 6 * n), groups = number of runs of equal material (word +8 of the triangle)) gets +0 = 0, halfword
 * +4 = number of groups, halfword +6 = the halfword at 0x008872E0 + *(s32 *)0x007647E0 (gp-0x4910; gp is 0x007690F0) which is then incremented, +0xC = 0, +8 = 3 * n; the
 * 12-byte group records start at +0x10 ({data pointer, number of halfwords, material}) and the data (three halfwords per triangle copied from
 * the triangle's first three halfwords) follows them. func_003c59f0(block, func_003c6280, 0) is called and the table freed (slot 0x008873EC). With no triangles the first record's material is 0 (retail read a stale table word). */

extern void *p4c_003c54a0_003c8920(u32 size) __asm__("func_003c54a0");
extern void p4c_003c59f0_003c8920(void *blk, void (*cb)(void), s32 z) __asm__("func_003c59f0");
extern void p4c_003df4d0_003c8920(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c8920(s32 code, ...) __asm__("func_003df590");
extern void p4c_00440bb8_003c8920(void *base, u32 n, u32 sz, void (*cmp)(void)) __asm__("func_00440bb8");

extern void p4fn_003c6080_003c8920(void) __asm__("func_003c6080");
extern void p4fn_003c6280_003c8920(void) __asm__("func_003c6280");

typedef void *(*P4AllocFn_003c8920)(u32 size, u32 tag);
typedef void (*P4FreeFn_003c8920)(void *p);

u8 *func_003c8920(u32 *arg0)
{
    u32 n = arg0[1];
    u32 **table = (u32 **)(*(P4AllocFn_003c8920 *)0x008873E8)(n << 2, 0x10502);
    u32 i;
    u32 groups;
    u32 prev;
    u32 size;
    u8 *block;
    s32 err[2];
    u32 *rec;
    u8 *cur;
    u32 *e;

    if (table == (u32 **)0) {
        err[0] = 2;
        err[1] = p4c_003df590_003c8920(0x80000013, arg0[1] << 2);
        p4c_003df4d0_003c8920(err);
        return (u8 *)0;
    }
    for (i = 0; i < arg0[1]; i++) {
        table[i] = (u32 *)(arg0[2] + i * 0x14);
    }
    p4c_00440bb8_003c8920(table, n, 4, p4fn_003c6080_003c8920);
    n = arg0[1];
    groups = 1;
    if (n >= 2) {
        prev = table[0][2];
        for (i = 1; i < n; i++) {
            if (table[i][2] != prev) {
                prev = table[i][2];
                groups++;
            }
        }
    }
    size = 0x10 + groups * 12 + n * 6;
    block = (u8 *)p4c_003c54a0_003c8920(size);
    if (block == (u8 *)0) {
        (*(P4FreeFn_003c8920 *)0x008873EC)(table);
        err[0] = 2;
        err[1] = p4c_003df590_003c8920(0x80000013, size);
        p4c_003df4d0_003c8920(err);
        return (u8 *)0;
    }
    *(u32 *)block = 0;
    *(u16 *)(block + 4) = 1;
    *(s16 *)(block + 6) = *(s16 *)(0x008872E0 + *(s32 *)0x007647E0);
    *(u32 *)(block + 0xC) = 0;
    *(u32 *)(block + 8) = arg0[1] * 3;
    *(s16 *)(0x008872E0 + *(s32 *)0x007647E0) = (s16)(*(s16 *)(0x008872E0 + *(s32 *)0x007647E0) + 1);
    *(u32 *)(block + 0x14) = 0;
    *(u32 *)(block + 0x18) = n != 0 ? table[0][2] : 0;
    cur = block + 0x10 + groups * 12;
    *(u32 *)(block + 0x10) = (u32)cur;
    rec = (u32 *)(block + 0x10);
    for (i = 0; i < arg0[1]; i++) {
        e = table[i];
        if (e[2] != rec[2]) {
            rec += 3;
            rec[1] = 0;
            rec[2] = e[2];
            rec[0] = (u32)cur;
            *(u16 *)(block + 4) = (u16)(*(u16 *)(block + 4) + 1);
        }
        *(u16 *)(cur + 0) = *(u16 *)((u8 *)e + 0);
        *(u16 *)(cur + 2) = *(u16 *)((u8 *)e + 2);
        *(u16 *)(cur + 4) = *(u16 *)((u8 *)e + 4);
        rec[1] = rec[1] + 3;
        cur += 6;
    }
    p4c_003c59f0_003c8920(block, p4fn_003c6280_003c8920, 0);
    (*(P4FreeFn_003c8920 *)0x008873EC)(table);
    return block;
}
