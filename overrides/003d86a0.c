/* func_003d86a0 (192 bytes): (arg0, arg1, arg2 = record {+4 = element size n, +8 = count c}): gets a scratch block of 3 * n bytes from the allocator slot 0x008873E8 (tag 0x10000), calls
 * func_003d8500(arg0, arg2, i, arg1, scratch) for i = c - 1 down to 0 and sums the results, frees the scratch block with the slot 0x008873EC and returns the sum. */

extern s32 p4c_003d8500_003d86a0(u8 *a0, u8 *rec, s32 i, u32 a1, void *scratch) __asm__("func_003d8500");

typedef void *(*P4AllocFn_003d86a0)(u32 size, u32 tag);
typedef void (*P4FreeFn_003d86a0)(void *p);

s32 func_003d86a0(u8 *arg0, u32 arg1, u8 *arg2)
{
    s32 sum = 0;
    s32 i;
    void *scratch;

    scratch = (*(P4AllocFn_003d86a0 *)0x008873E8)(*(u32 *)(arg2 + 4) * 3, 0x10000);
    for (i = *(s32 *)(arg2 + 8) - 1; i >= 0; i--) {
        sum += p4c_003d8500_003d86a0(arg0, arg2, i, arg1, scratch);
    }
    (*(P4FreeFn_003d86a0 *)0x008873EC)(scratch);
    return sum;
}
