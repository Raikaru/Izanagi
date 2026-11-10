/* func_003d68b0 (rt2d_grouped, 68 bytes): the byte size of a UV-animation
 * record. Calls _rpUVAnimCustomDataStreamGetSize(*(u32 *)(arg0 + 20)) (retail
 * 0x003D7B30, returns 0x40), adds the fixed 4-byte header, then adds
 * (*(s32 *)(arg0 + 4)) << 5, the per-key stride. */

u32 _rpUVAnimCustomDataStreamGetSize(u32 value);

s32 func_003d68b0(u8 *arg0)
{
    s32 size;

    size = (s32)_rpUVAnimCustomDataStreamGetSize(*(u32 *)(arg0 + 20));
    return (4 + size) + (*(s32 *)(arg0 + 4) << 5);
}
