/* func_00416df0: stream size of a type 2 animation: 22 bytes per keyframe (4 time, 14 key data,
 * 4 previous-frame offset) plus the 24 custom bytes. */
s32 func_00416df0(u8 *anim)
{
    return *(s32 *)(anim + 4) * 22 + 24;
}
