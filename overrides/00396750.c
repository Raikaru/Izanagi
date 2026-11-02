/* func_00396750 (HAnim module 0x11E open callback, registered by func_00396f00 through
 * func_003e8930; HAnimClose is the matching close): creates the module's 0x24-byte-entry free
 * list (entries per block and preallocated blocks from the retail globals gp-22016 and gp-22012,
 * in-place free-list header D_00884A50, hint 0x0004011E) into the retail gp global
 * gp-18980 (0x007646CC), then registers the HAnim keyframe interpolation scheme with
 * func_003d5000 (RtAnimRegisterInterpolationScheme): type 1, 0x24-byte keyframes, and the eight
 * HAnim keyframe callbacks. The registration also runs when the free list failed; that
 * failure only makes the return value 0. Returns arg0 on success.
 *
 * Direct transliteration of the retail code. The callbacks are passed by symbol (RenderWare
 * calls them on the host). */

extern u8 p4cb_003954b0[] __asm__("func_003954b0");
extern u8 p4cb_00395bd0[] __asm__("func_00395bd0");
extern u8 p4cb_003955a0[] __asm__("func_003955a0");
extern u8 p4cb_00396520[] __asm__("func_00396520");
extern u8 p4cb_003963c0[] __asm__("func_003963c0");
extern u8 p4cb_003961f0[] __asm__("func_003961f0");
extern u8 p4cb_003962e0[] __asm__("func_003962e0");
extern u8 p4cb_003963b0[] __asm__("func_003963b0");
extern s32 p4_rtanim_register_scheme(void *info) __asm__("func_003d5000");

s32 func_00396750(s32 arg0)
{
    u32 info[12];
    s32 list;
    s32 result = arg0;

    list = func_003e1220(0x24, *(s32 *)(0x007690F0 - 22016), 4, *(s32 *)(0x007690F0 - 22012), (void *)0x00884A50, 0x4011E);
    *(s32 *)(0x007690F0 - 18980) = list;
    if (list == 0)
        result = 0;

    info[0] = 1;
    info[1] = 0x24;
    info[2] = 0x24;
    info[3] = (u32)p4cb_003954b0;
    info[4] = (u32)p4cb_00395bd0;
    info[5] = (u32)p4cb_003955a0;
    info[6] = (u32)p4cb_00396520;
    info[7] = (u32)p4cb_003963c0;
    info[8] = (u32)p4cb_003961f0;
    info[9] = (u32)p4cb_003962e0;
    info[10] = (u32)p4cb_003963b0;
    info[11] = 0;
    p4_rtanim_register_scheme(info);
    return result;
}
