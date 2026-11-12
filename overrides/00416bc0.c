/* func_00416bc0: stream read of an RtAnimInterpolatorInfo type 2 animation (registered by
 * func_00417470). anim: +4 keyframe count, +0x10 keyframe array (24-byte frames), +0x14 custom data
 * (24 bytes). Per frame: +4 the time (func_003df300), +8 fourteen bytes of packed key data
 * (func_003df2a0), then a word that is the byte offset of the previous frame (func_003df360), stored
 * back as a pointer at +0. The custom data follows. Returns anim, or 0 when a read fails. */
extern s32 p4c_003df300_00416bc0(s32 stream, void *dst, s32 n) __asm__("func_003df300");
extern s32 p4c_003df2a0_00416bc0(s32 stream, void *dst, s32 n) __asm__("func_003df2a0");
extern s32 p4c_003df360_00416bc0(s32 stream, void *dst, s32 n) __asm__("func_003df360");

u8 *func_00416bc0(s32 stream, u8 *anim)
{
    u8 *frames = *(u8 **)(anim + 0x10);
    u8 *custom = *(u8 **)(anim + 0x14);
    s32 i;
    u8 *kf = frames;
    s32 offset;

    for (i = 0; i < *(s32 *)(anim + 4); i++, kf += 24) {
        if (!p4c_003df300_00416bc0(stream, kf + 4, 4)) {
            return (u8 *)0;
        }
        if (!p4c_003df2a0_00416bc0(stream, kf + 8, 14)) {
            return (u8 *)0;
        }
        if (!p4c_003df360_00416bc0(stream, &offset, 4)) {
            return (u8 *)0;
        }
        *(u8 **)kf = frames + (u32)offset / 24u * 24u;
    }
    return p4c_003df300_00416bc0(stream, custom, 24) ? anim : (u8 *)0;
}
