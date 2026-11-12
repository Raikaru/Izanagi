/* func_003c55f0 (272 bytes): allocates a list record of arg0 entries and returns it, or 0 on failure. The record (12 bytes,
 * from the pool *(u32 *)0x007647D8, tag 0x30502, slot 0x008873F8) gets +4 = 0 and, for arg0 = 0, +8 = +0 = 0;
 * otherwise a block of arg0 * 20 bytes (tag 0x1030502, slot 0x008873E8) goes to +8 and the count to +0; when the block
 * cannot be had the record is returned to the pool (slot 0x008873FC). Failures report {2, func_003df590(0x80000013,
 * size)} through func_003df4d0 (size 12 for the record, arg0 * 20 for the block). */

extern void p4c_003df4d0_003c55f0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c55f0(s32 code, ...) __asm__("func_003df590");

typedef void *(*P4PoolAllocFn_003c55f0)(u32 pool, u32 tag);
typedef void *(*P4AllocFn_003c55f0)(u32 size, u32 tag);
typedef void (*P4PoolFreeFn_003c55f0)(u32 pool, void *obj);

u32 *func_003c55f0(s32 arg0)
{
    u32 *rec;
    s32 err[2];
    u32 bytes;

    rec = (u32 *)(*(P4PoolAllocFn_003c55f0 *)0x008873F8)(*(u32 *)0x007647D8, 0x30502);
    if (rec == (u32 *)0) {
        err[0] = 2;
        err[1] = p4c_003df590_003c55f0(0x80000013, 12);
        p4c_003df4d0_003c55f0(err);
        return (u32 *)0;
    }
    rec[1] = 0;
    if (arg0 == 0) {
        rec[2] = 0;
        rec[0] = 0;
        return rec;
    }
    bytes = (u32)arg0 * 20;
    rec[2] = (u32)(*(P4AllocFn_003c55f0 *)0x008873E8)(bytes, 0x1030502);
    if (rec[2] != 0) {
        rec[0] = (u32)arg0;
        return rec;
    }
    (*(P4PoolFreeFn_003c55f0 *)0x008873FC)(*(u32 *)0x007647D8, rec);
    err[0] = 2;
    err[1] = p4c_003df590_003c55f0(0x80000013, bytes);
    p4c_003df4d0_003c55f0(err);
    return (u32 *)0;
}
