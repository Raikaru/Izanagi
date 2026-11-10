/* func_003ca5a0 (module 0x509 engine-open callback, registered by func_003cb900 through
 * func_003e8930(0xC, 0x509, open, close)): `offset` is the module's offset in the RenderWare
 * engine block at 0x008872E0 (kept in the retail global gp-18664 = 0x00764808). Creates the
 * module's two 24-byte-entry free lists into block words 0 and 1
 * (entries per block and preallocated blocks come from the retail globals at gp-21848/-21844
 * and gp-21840/-21836; the free-list headers are the in-place structs at 0x00886610 and
 * 0x008865E0; hint 0x00040507), sets the engine-block halfword at 0x008872E8 to 1, clears block
 * word 2 and counts the instance in gp-18660. A failed second free list destroys the first and
 * returns 0. Returns the engine pointer on success.
 *
 * Direct transliteration of the retail code. */

extern s32 func_003e1220(s32 size, s32 blockSize, s32 alignment, s32 prealloc, void *space, s32 hint);

#define P4_GP_WORD(off) (*(s32 *)(0x007690F0 + (off)))

void *func_003ca5a0(void *engine, s32 offset)
{
    u32 *block;

    P4_GP_WORD(-18664) = offset;
    block = (u32 *)(0x008872E0 + P4_GP_WORD(-18664));
    block[0] = func_003e1220(24, P4_GP_WORD(-21848), 4, P4_GP_WORD(-21844), (void *)0x00886610, 0x00040507);
    if (block[0] == 0)
        return NULL;

    block = (u32 *)(0x008872E0 + P4_GP_WORD(-18664));
    block[1] = func_003e1220(24, P4_GP_WORD(-21840), 4, P4_GP_WORD(-21836), (void *)0x008865E0, 0x00040507);
    if (block[1] == 0) {
        func_003e12f0((u8 *)block[0]);
        ((u32 *)(0x008872E0 + P4_GP_WORD(-18664)))[0] = 0;
        return NULL;
    }

    *(u16 *)0x008872E8 = 1;
    ((u32 *)(0x008872E0 + P4_GP_WORD(-18664)))[2] = 0;
    P4_GP_WORD(-18660) = P4_GP_WORD(-18660) + 1;
    return engine;
}
