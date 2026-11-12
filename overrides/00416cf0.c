/* func_00416cf0: stream write of a type 2 animation, the inverse of func_00416bc0: per frame the
 * time (func_003df1a0), the fourteen key bytes (func_003df270), the previous frame as a byte offset
 * from the frame array (func_003df240); then the 24 custom bytes. Returns 1 on success, else 0. */
extern s32 p4c_003df1a0_00416cf0(s32 stream, void *src, s32 n) __asm__("func_003df1a0");
extern s32 p4c_003df270_00416cf0(s32 stream, void *src, s32 n) __asm__("func_003df270");
extern s32 p4c_003df240_00416cf0(s32 stream, void *src, s32 n) __asm__("func_003df240");

s32 func_00416cf0(u8 *anim, s32 stream)
{
    u8 *frames = *(u8 **)(anim + 0x10);
    u8 *custom = *(u8 **)(anim + 0x14);
    s32 i;
    u8 *kf = frames;
    s32 offset;

    for (i = 0; i < *(s32 *)(anim + 4); i++, kf += 24) {
        if (!p4c_003df1a0_00416cf0(stream, kf + 4, 4)) {
            return 0;
        }
        if (!p4c_003df270_00416cf0(stream, kf + 8, 14)) {
            return 0;
        }
        offset = (s32)(*(u8 **)kf - frames);
        if (!p4c_003df240_00416cf0(stream, &offset, 4)) {
            return 0;
        }
    }
    return p4c_003df1a0_00416cf0(stream, custom, 24) != 0;
}
