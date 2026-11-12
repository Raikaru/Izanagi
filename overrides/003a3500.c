/* func_003a3500 (368 bytes): byte size of a particle-tank instance buffer. arg0 is the particle
 * count n and arg1 the component flags. Each component adds (n * stride + pad) & ~15, with the stride in
 * the retail table words and pad 15 (0x4F for the 0x4 and 0x2 components):
 *
 *   0x01: stride 16        0x08: *(u32 *)0x00752D24   0x10: *(u32 *)0x00752D28
 *   0x04: *(u32 *)0x00752D2C (pad 0x4F)   0x02: *(u32 *)0x00752D30 (pad 0x4F)
 *   0x40: *(u32 *)0x00752D34   0x20: *(u32 *)0x00752D38   0x80: *(u32 *)0x00752D3C
 *   0x100: *(u32 *)0x00752D40
 *
 * Leaf function. */

s32 func_003a3500(s32 arg0, u32 arg1)
{
    s32 total;

    total = 0;
    if (arg1 & 0x1) {
        total += (arg0 * 16 + 0xF) & ~0xF;
    }
    if (arg1 & 0x8) {
        total += (arg0 * *(s32 *)0x00752D24 + 0xF) & ~0xF;
    }
    if (arg1 & 0x10) {
        total += (arg0 * *(s32 *)0x00752D28 + 0xF) & ~0xF;
    }
    if (arg1 & 0x4) {
        total += (arg0 * *(s32 *)0x00752D2C + 0x4F) & ~0xF;
    }
    if (arg1 & 0x2) {
        total += (arg0 * *(s32 *)0x00752D30 + 0x4F) & ~0xF;
    }
    if (arg1 & 0x40) {
        total += (arg0 * *(s32 *)0x00752D34 + 0xF) & ~0xF;
    }
    if (arg1 & 0x20) {
        total += (arg0 * *(s32 *)0x00752D38 + 0xF) & ~0xF;
    }
    if (arg1 & 0x80) {
        total += (arg0 * *(s32 *)0x00752D3C + 0xF) & ~0xF;
    }
    if (arg1 & 0x100) {
        total += (arg0 * *(s32 *)0x00752D40 + 0xF) & ~0xF;
    }
    return total;
}
