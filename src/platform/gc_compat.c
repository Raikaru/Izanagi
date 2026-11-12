/* GameCube SDK and PowerPC intrinsics that the CRI sources reconstructed from the GameCube
 * build (src/cri/re4) call. The PS2 retail image has no code for them, so they have no retail
 * address; they are defined here with their documented behaviour so those units link without
 * traps.
 *
 *  __cvt_fp2unsigned  float -> unsigned, truncating, clamped to 0 .. 0xFFFFFFFF
 *  __dcbt             data-cache touch: a no-op on the host
 *  __dcbz             zero the 32-byte cache block that contains ptr + offset
 *  __lwbrx            load word byte-reversed from ptr + offset
 *  __rlwinm           rotate left, then AND with the mask of bits mb..me (PowerPC big-endian
 *                     bit numbering, bit 0 = most significant)
 *  __rlwimi           rotate left, then insert under that mask into the first operand
 *  (DCInvalidateRange, OSDisableInterrupts and OSRestoreInterrupts are in src/cri/cri_glue.c)
 *  ARAlloc / ARFree   auxiliary-RAM stack allocator: ARAlloc returns the address of a new block
 *                     above the previous ones, ARFree frees the newest block and stores its
 *                     size in *length */
#include <stdint.h>
#include <string.h>

uint32_t __cvt_fp2unsigned(float f)
{
    if (!(f > 0.0f)) {
        return 0;
    }
    if (f >= 4294967296.0f) {
        return 0xFFFFFFFFu;
    }
    return (uint32_t)f;
}

void __dcbt(const void *ptr, int offset)
{
    (void)ptr;
    (void)offset;
}

void __dcbz(void *ptr, int offset)
{
    uintptr_t block = ((uintptr_t)ptr + (uintptr_t)(intptr_t)offset) & ~(uintptr_t)31;

    memset((void *)block, 0, 32);
}

uint32_t __lwbrx(const void *ptr, int offset)
{
    uint32_t v;

    memcpy(&v, (const uint8_t *)ptr + offset, sizeof v);
    return __builtin_bswap32(v);
}

static uint32_t ppc_mask(int mb, int me)
{
    uint32_t hi = 0xFFFFFFFFu >> mb;
    uint32_t lo = 0xFFFFFFFFu << (31 - me);

    return mb <= me ? (hi & lo) : (hi | lo);
}

static uint32_t rotl32(uint32_t v, int sh)
{
    sh &= 31;
    return sh ? (v << sh) | (v >> (32 - sh)) : v;
}

uint32_t __rlwinm(uint32_t value, int shift, int mb, int me)
{
    return rotl32(value, shift) & ppc_mask(mb, me);
}

uint32_t __rlwimi(uint32_t dest, uint32_t source, int shift, int mb, int me)
{
    uint32_t mask = ppc_mask(mb, me);

    return (rotl32(source, shift) & mask) | (dest & ~mask);
}

#define AR_BASE 0x4000u
#define AR_SIZE (16u * 1024u * 1024u)
#define AR_BLOCKS 32

static uint32_t ar_top = AR_BASE;
static uint32_t ar_len[AR_BLOCKS];
static int ar_count;

uint32_t ARAlloc(uint32_t length)
{
    uint32_t at = ar_top;

    length = (length + 31u) & ~31u;
    if (ar_count >= AR_BLOCKS || at + length > AR_BASE + AR_SIZE) {
        return 0;
    }
    ar_len[ar_count++] = length;
    ar_top += length;
    return at;
}

uint32_t ARFree(uint32_t *length)
{
    if (ar_count == 0) {
        if (length != 0) {
            *length = 0;
        }
        return ar_top;
    }
    ar_count--;
    ar_top -= ar_len[ar_count];
    if (length != 0) {
        *length = ar_len[ar_count];
    }
    return ar_top;
}
